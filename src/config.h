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

#endif