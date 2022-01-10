#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "task.h"
#include "config.h"

struct process
{
    // The process id
    uint16_t id;

    char filename[MYOS_MAX_PATH];

    // The main process task
    struct task* task;

    // The memory (malloc) allocations of the process
    //to keep track of all the allocated memories 
    void* allocations[MYOS_MAX_PROGRAM_ALLOCATIONS];

    // The physical pointer to the process memory.
    //used to point to any data we need
    void* ptr;

    // The physical pointer to the stack memory
    void* stack;

    // The size of the data pointed to by "ptr"
    uint32_t size;
};

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1

int process_load(const char* filename, struct process** process);
int process_load_for_slot(const char* filename, struct process** process, int process_slot);

#endif