#include "task.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "process.h"

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

    return 0;
} 