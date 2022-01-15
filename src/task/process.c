#include "process.h"
#include "config.h"
#include "status.h"
#include "task/task.h"
#include "memory/memory.h"
#include "std/string.h"
#include "std/stdio.h"

#include "filesystem/file.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "loader/formats/elfloader.h"

// The current process that is running
struct process* current_process = 0;

//hold all the processes we have running now
static struct process* processes[MYOS_MAX_PROCESSES] = {};

//inititaze the process memory
static void process_init(struct process* process)
{
    memset(process, 0, sizeof(struct process));
}

struct process* process_current()
{
    return current_process;
}

//get the process from processes array at index process_id
struct process* process_get(int process_id)
{
    if (process_id < 0 || process_id >= MYOS_MAX_PROCESSES)
    {
        return NULL;
    }

    return processes[process_id];
}

//load the binary file into process->ptr
static int process_load_binary(const char* filename, struct process* process)
{
    //print("in process_load_binary\n");
    int res = 0;

    int fd = fopen(filename, "r");
    if (!fd)
    {
        //print("Error!!!\n");
        res = -EIO;
        goto out;
    }

    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res != ALL_OK)
    {
        //print("Error!!!\n");
        goto out;
    }

    void* program_data_ptr = kzalloc(stat.filesize);
    if (!program_data_ptr)
    {
        //print("Error!!!\n");
        res = -ENOMEM;
        goto out;
    }

    if (fread(program_data_ptr, stat.filesize, 1, fd) != 1)
    {
        //print("Error!!!\n");
        res = -EIO;
        goto out;
    }

    process->ptr = program_data_ptr;
    process->size = stat.filesize;
    process->filetype = PROCESS_FILETYPE_BINARY;

out:
    fclose(fd);
    //print("exit process_load_binary\n");

    return res;
}

//calling elf_load to load the elfile 
static int process_load_elf(const char* filename, struct process* process)
{
    int res = 0;
    struct elf_file* elf_file = 0;
    res = elf_load(filename, &elf_file);
    if (ISERR(res))
    {
        goto out;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;
out:
    return res;
}

static int process_load_data(const char* filename, struct process* process)
{
    int res = 0;    
    //res = process_load_binary(filename, process);
    //load the elf into the process
    res = process_load_elf(filename, process);
    //if we cannot load the elf file headers, then this file is a binary 
    if (res == -EINFORMAT)
    {
        res = process_load_binary(filename, process);
    }

    return res;
}

//mapping the bin file to process page
int process_map_binary(struct process* process)
{
    int res = 0;
    paging_map_to(process->task->page_directory, (void*) MYOS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address(process->ptr + process->size), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

//mapping the elf file to the process page
static int process_map_elf(struct process* process)
{
    int res = 0;

    struct elf_file* elf_file = process->elf_file;
    //map the page table using the virtual addr and the physical addr inside the elfile object which we loaded 
    //note we round up the start addr and round down the ending addr, so to make sure no matter what the data will always fit
    res = paging_map_to(process->task->page_directory, paging_align_to_lower_page(elf_virtual_base(elf_file)), elf_phys_base(elf_file), paging_align_address(elf_phys_end(elf_file)), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);
    return res;
}

int process_map_memory(struct process* process)
{
    int res = 0;

    //different way of mapping base on the file type
    switch(process->filetype)
    {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf(process);
        break;

        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary(process);
        break;

        default://invalid file type
            panic("process_map_memory: Invalid filetype\n");
    }

    if (res < 0)
    {
        goto out;
    }

    //map the stack of the task
    paging_map_to(process->task->page_directory, (void*)MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address(process->stack+MYOS_USER_PROGRAM_STACK_SIZE), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE);

out:
    return res;
}

//return the first free index of in the processes array
int process_get_free_slot()
{
    for (int i = 0; i < MYOS_MAX_PROCESSES; i++)
    {
        if (processes[i] == 0)
            return i;
    }

    return -EISTKN;
}

//loading an existing process into our processes array at the index process_slot
int process_load_for_slot(const char* filename, struct process** process, int process_slot)
{
    //print("in process_load_for_slot\n");
    int res = 0;
    struct task* task = 0;
    struct process* _process;
    //point to the stack of the new program 
    void* program_stack_ptr = 0;

    //if the slot already has a process loaded, then we return error
    if (process_get(process_slot) != 0)
    {
        res = -EISTKN;
        goto out;
    }

    _process = kzalloc(sizeof(struct process));
    if (!_process)
    {
        res = -ENOMEM;
        goto out;
    }
    
    //zero out the _process memory
    process_init(_process);

    //load the file into _process->ptr
    res = process_load_data(filename, _process);
    if (res < 0)
    {
        goto out;
    }

    //allocate the stack for the user process
    program_stack_ptr = kzalloc(MYOS_USER_PROGRAM_STACK_SIZE);
    if (!program_stack_ptr)
    {
        res = -ENOMEM;
        goto out;
    }

    //copy the filename and assigned the stack pointer and id
    strncpy(_process->filename, filename, sizeof(_process->filename));
    _process->stack = program_stack_ptr;
    _process->id = process_slot;

    // create a new task
    task = task_new(_process);
    if (ERROR_I(task) == 0)
    {
        res = ERROR_I(task);
        goto out;
    }

    _process->task = task;

    res = process_map_memory(_process);
    if (res < 0)
    {
        goto out;
    }

    *process = _process;

    // Add the process to the array
    processes[process_slot] = _process;

out:
    //print("exit process_load_for_slot\n");
    if (ISERR(res))
    {
        if (_process && _process->task)
        {
            task_free(_process->task);
        }

       //FIXME: create function for free all other memory
    }
    return res;
}

//find the first free slot in the processes array and call process_load_for_slot to load the process into that slot
int process_load(const char* filename, struct process** process)
{
    //print("in process_load\n");
    int res = 0;
    int process_slot = process_get_free_slot();
    if (process_slot < 0)
    {
        res = -EISTKN;
        goto out;
    }

    res = process_load_for_slot(filename, process, process_slot);
out:
    //print("exited process_load\n");
    return res;
}


int process_switch(struct process* process)
{
    current_process = process;
    return 0;
}

//load the process and switch the current process to it 
int process_load_switch(const char* filename, struct process** process)
{
    int res = process_load(filename, process);
    if (res == 0)
    {
        process_switch(*process);
    }

    return res;
}