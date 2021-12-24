#include "paging.h"
#include "memory/heap/kheap.h"

static uint32_t* current_directory = 0;

//we are creating and allocating the entire page table for the 32 bit space
//this is only possible in 32 bit kernel space, this implementation is not feasible for 64 bit system
//and note that the below initialization have no useful data, we are simply putting mappings like 0x12345 => 0x12345 and the flags 
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags)
{
    uint32_t* directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    int offset = 0;
    //initialize the paging direcory
    for(int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        for(int b = 0; b < PAGING_TOTAL_ENTRIES_PER_TABLE; b++)
        {
            entry[b] = (offset + b*PAGING_PAGE_SIZE) | flags;
        }
        //done with the first table 
        offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
        //the direcotry should be really a double pointer, but converting the address to uint also works
        directory[i] = (uint32_t) entry | flags | PAGING_IS_WRITEABLE;
    }
    struct paging_4gb_chunk* chunk_4gb = kzalloc(sizeof(struct paging_4gb_chunk));
    chunk_4gb->directory_entry = directory;
    return chunk_4gb;
}

//for context switch? switching between different page mapping
void paging_switch(uint32_t* directory)
{
    paging_load_directory(directory);
    current_directory = directory;
}


uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk)
{
    return chunk->directory_entry;
}