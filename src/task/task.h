#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"

struct interrupt_frame;
//for holding the CPU registers, used to switch/restore between different tasks
//this struct holds all the information to return to a preivous CPU state
struct registers
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
};

struct process;
//think of this as a linked list of processes for now
struct task
{
    //each task has its own paging directory
    struct paging_4gb_chunk* page_directory;

    // The registers of the task when the task is not running
    struct registers registers;

    //process of the task
    struct process* process;
    // The next task in the linked list
    struct task* next;
    // Previous task in the linked list
    struct task* prev;
};

struct task* task_new(struct process* process);
struct task* task_current();
struct task* task_get_next();
int task_free(struct task* task);
int task_switch(struct task* task);
int task_page();

void task_run_first_ever_task();

void task_return(struct registers* regs);
void restore_general_purpose_registers(struct registers* regs);
void user_registers();
void task_current_save_state(struct interrupt_frame *frame);
int copy_string_from_task(struct task* task, void* virtual, void* phys, int max);
void* task_get_stack_item(struct task* task, int index);


#endif 