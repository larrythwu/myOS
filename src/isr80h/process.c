#include "process.h"
#include "task/task.h"
#include "task/process.h"
#include "std/string.h"
#include "status.h"
#include "config.h"
#include "kernel.h"
#include "std/stdio.h"

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

//switch the task and also pass in the arguments
void* isr80h_command7_invoke_system_command(struct interrupt_frame* frame)
{
    //get the physcial address
    struct command_argument* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    
    if (!arguments || strlen(arguments[0].argument) == 0)
    {
        return ERROR(-EINVARG);
    }

    struct command_argument* root_command_argument = &arguments[0];

    const char* program_name = root_command_argument->argument;

    char path[MYOS_MAX_PATH];
    strcpy(path, "0:/");
    strncpy(path+3, program_name, sizeof(path));

    struct process* process = 0;
    int res = process_load_switch(path, &process);
    if (res < 0)
    {
        return ERROR(res);
    }

    res = process_inject_arguments(process, root_command_argument);
    if (res < 0)
    {
        return ERROR(res);
    }

    task_switch(process->task);
    task_return(&process->task->registers);
    return 0;
}

void* isr80h_command8_get_program_arguments(struct interrupt_frame* frame)
{
    struct process* process = task_current()->process;
    //we get the physical address of the variable in the task where we want to store the arguments 
    struct process_arguments* arguments = task_virtual_address_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    //get the process arguments into the task argument variables
    process_get_arguments(process, &arguments->argc, &arguments->argv);
    return 0;
} 

void* isr80h_command9_exit(struct interrupt_frame* frame)
{
    print("\nExiting current program...\n");
    struct process* process = task_current()->process;
    process_terminate(process);
    task_next();
    return 0;
} 