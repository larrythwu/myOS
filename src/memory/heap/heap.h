#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include <stdint.h>
#include <stddef.h>

//to indicate if a heap block is taken or free
#define HEAP_TABLE_ENTRY_TAKEN 0x01
#define HEAP_TABLE_ENTRY_FREE 0x00

//indicate if the block is the first and whetehr it have another adjacent block
#define HEAP_BLOCK_HAVE_NEXT 0x80
#define HEAP_BLOCK_IS_FIRST 0x40

//a char is one byte which is the size of our table entry
typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

struct heap_table
{
    //pointer to our array of entries
    HEAP_BLOCK_TABLE_ENTRY* entries;
    //number of blocks we have allocated
    size_t total;
};

struct heap
{
    //mem address of our table
    struct heap_table* table;
    //use to store the address of our heap starting addr
    void* saddr;
};

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);

void* heap_malloc(struct heap* heap, size_t size);

void heap_free(void* ptr);
#endif