#include "task.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "process.h"
#include "std/stdio.h"
#include "idt/idt.h"
#include "memory/paging/paging.h"
#include "std/string.h"

// the current task that is running
struct task* current_task = 0;

// fpr task linked list
struct task* task_tail = 0;
struct task* task_head = 0;

int task_init(struct task* task, struct process* process);

//return the current task
struct task* task_current()
{
    return current_task;
}

//create and inintialize a new task and add it to the end of the linked list 
struct task* task_new(struct process* process)
{
    int res = 0;
    struct task* task = kzalloc(sizeof(struct task));
    if (!task)
    {
        res = -ENOMEM;
        goto out;
    }

    res = task_init(task, process);
    if (res != ALL_OK)
    {
        goto out;
    }

    if (task_head == 0)
    {
        task_head = task;
        task_tail = task;
        current_task = task;
        goto out;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

out:    
    if (ISERR(res))
    {
        task_free(task);
        return ERROR(res);
    }

    return task;
}

//get the next task in the linked list, note that we want the list to loop back 
//so if the next task is null then we got to the head
struct task* task_get_next()
{
    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task* task)
{
    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next();
    }
}

//free the task object and all the sub components 
int task_free(struct task* task)
{
    //free the paging direcotry memory
    paging_free_4gb(task->page_directory);
    //remove the task from the linked list
    task_list_remove(task);

    // Finally free the task data
    kfree(task);
    return 0;
}

//initialize the task 
int task_init(struct task* task, struct process* process)
{
    memset(task, 0, sizeof(struct task));
    //Create a new page directory, a 4GB read-only address space 
    task->page_directory = paging_new_4gb(PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    if (!task->page_directory)
    {
        return -EIO;
    }

    task->registers.ip = MYOS_PROGRAM_VIRTUAL_ADDRESS;
    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.esp = MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;
    task->registers.cs = USER_CODE_SEGMENT;
    task->process = process;
    return 0;
} 

//change the current task that is running and reload the page table
int task_switch(struct task* task)
{
    current_task = task;
    paging_switch(task->page_directory);
    return 0;
}

//switch from the kernel page direcotry into the task page directory
//note the kernel activity is not tasks
int task_page()
{
    user_registers();
    task_switch(current_task);
    return 0;
}

//runnig the first task: run the task link list head
void task_run_first_ever_task()
{
    //make sure a current_task is set
    if (!current_task)
    {
        panic("task_run_first_ever_task(): No current task exists!\n");
    }

    //switch the page directory to task's
    task_switch(task_head);
    //return to user space by reloading its registers
    //print("running task_return\n");
    task_return(&task_head->registers);
}

//store the register status of the user task into the task object
void task_save_state(struct task *task, struct interrupt_frame *frame)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ebp = frame->ebp;
    task->registers.ebx = frame->ebx;
    task->registers.ecx = frame->ecx;
    task->registers.edi = frame->edi;
    task->registers.edx = frame->edx;
    task->registers.esi = frame->esi;
}

void task_current_save_state(struct interrupt_frame *frame)
{
    if (!task_current())
    {
        panic("No current task to save\n");
    }

    struct task *task = task_current();
    task_save_state(task, frame);
}

//copy a string stored in task's virtual memory into a physical address in the kernel
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max)
{
    if (max >= PAGING_PAGE_SIZE)
    {
        return -EINVARG;
    }

    int res = 0;
    char* tmp = kzalloc(max);
    if (!tmp)
    {
        res = -ENOMEM;
        goto out;
    }

    uint32_t* task_directory = task->page_directory->directory_entry;
    //this returns the physical addess of the temp with the flags
    uint32_t old_entry = paging_get(task_directory, tmp);
    //notice that currently we are in the kernel page table, we set the mapping in the task page table 
    //to point to the same temp object that we created in kernel
    //this allows the task to see our temp variable
    paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    paging_switch(task->page_directory);
    //in task space we can see virtual now, and we copy the string stored in virutal address to the temp
    //so now the kernel can see the string stored in the task virutal memory
    strncpy(tmp, virtual, max);
    kernel_page();

    res = paging_set(task_directory, tmp, old_entry);
    if (res < 0)
    {
        res = -EIO;
        goto out_free;
    }

    strncpy(phys, tmp, max);

out_free:
    kfree(tmp);
out:
    return res;
}

//load the page table of the task given
int task_page_task(struct task* task)
{
    user_registers();
    paging_switch(task->page_directory);
    return 0;
}

//pull the items from the task stack
void* task_get_stack_item(struct task* task, int index)
{
    void* result = 0;
    //this is the stack the moment the interrupt is called
    uint32_t* sp_ptr = (uint32_t*) task->registers.esp;

    // Switch to the given tasks page, but the stack is still kernel stack
    task_page_task(task);
    //we are pushing this value onto the KERNEL stack, as we only loaded the task page table
    //nothing else
    result = (void*) sp_ptr[index];

    // Switch back to the kernel page
    kernel_page();

    return result;
} 