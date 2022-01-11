#ifndef CONFIG_H
#define CONFIG_H

#define KERNEL_CODE_SELECTOR 0X08
#define KERNEL_DATA_SELECTOR 0x10
//the numebr of interrupt we support in protected mode
#define MYOS_TOTAL_INTERRUPTS 512

//we set our heap to 100MB for now, that is assuming 
//that the computer has 100MB free memory 
#define MYOS_HEAP_SIZE_BYTE 104857600
//and we require that all the heap allocations are 4KB aligned 
#define MYOS_HEAP_BLOCK_SIZE 4096

//refer to https://wiki.osdev.org/Memory_Map_(x86) Exteneded Memory section
//for addresses of free to use mem addr
#define MYOS_HEAP_ADDRESS 0x01000000
//another piece of free mem where we can store our heap table
//0x00007E00 ~ 0x0007FFFF 
#define MYOS_HEAP_TABLE_ADRESS 0x00007E00 

#define HEAP_BLOCK_TABLE_ENTRY_FREE 0

#define MYOS_SECTOR_SIZE 512

#define MYOS_MAX_PATH 108

#define MYOS_MAX_FILESYSTEMS 12

#define MYOS_MAX_FILE_DESCRIPTORS 512

#define MYOS_TOTAL_GDT_SEGMENTS 6

#define MYOS_PROGRAM_VIRTUAL_ADDRESS 0x400000
//16KB user program stack size
#define MYOS_USER_PROGRAM_STACK_SIZE 1024 * 16
#define MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START 0x3FF000
//note that the stack grows downward so the end address is always less the the start
#define MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_END MYOS_PROGRAM_VIRTUAL_STACK_ADDRESS_START - MYOS_USER_PROGRAM_STACK_SIZE

//the offset in the gdt_real table defined in kernel.c
#define USER_DATA_SEGMENT 0x23
#define USER_CODE_SEGMENT 0x1b
#define MYOS_MAX_PROGRAM_ALLOCATIONS 1024
#define MYOS_MAX_PROCESSES 12

#define MYOS_KEYBOARD_BUFFER_SIZE 1024
#define MYOS_MAX_ISR80H_COMMANDS 1024
#endif