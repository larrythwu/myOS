#ifndef PROCESS_H
#define PROCESS_H
#include <stdint.h>
#include "task.h"
#include "config.h"

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1
typedef unsigned char PROCESS_FILETYPE;

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
    //ptr is for binary file 
    //elf_file is for loading the elf file folder 
    //depending on the file type we are loading, one of them will be occuppied 
    union
    {
        // The physical pointer to the process memory.
        void* ptr;
        struct elf_file* elf_file;
    };
    // The physical pointer to the stack memory
    void* stack;

    // The size of the data pointed to by "ptr"
    uint32_t size;

    //the keyboard buffer
    struct keyboard_buffer
    {
        char buffer[MYOS_KEYBOARD_BUFFER_SIZE];
        int tail;
        int head;
    } keyboard;

    //----for loading in the elf file-----//
    PROCESS_FILETYPE filetype;




};

#define PROCESS_FILETYPE_ELF 0
#define PROCESS_FILETYPE_BINARY 1

int process_load(const char* filename, struct process** process);
int process_load_for_slot(const char* filename, struct process** process, int process_slot);
struct process* process_current();
struct process* process_get(int process_id);
int process_switch(struct process* process);
int process_load_switch(const char* filename, struct process** process);
void* process_malloc(struct process* process, size_t size);
void process_free(struct process* process, void* ptr);

#endif