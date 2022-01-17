#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "std/string.h"
#include "status.h"
#include "config.h"

//syscall handler for loading a process
void* isr80h_command6_process_load_start(struct interrupt_frame* frame)
{
    //get the filename of the program to load
    void* filename_user_ptr = task_get_stack_item(task_current(), 0);
    char filename[MYOS_MAX_PATH];
    int res = copy_string_from_task(task_current(), filename_user_ptr, filename, sizeof(filename));
    if (res < 0)
    {
        goto out;
    }

    char path[MYOS_MAX_PATH];
    strcpy(path, "0:/");
    strcpy(path+3, filename);

    struct process* process = 0;
    res = process_load_switch(path, &process);
    if (res < 0)
    {
        goto out;
    }

    task_switch(process->task);
    //call the iret routine to enter user space again, with the new process registers and page map
    task_return(&process->task->registers);

out:
    return 0;
} 