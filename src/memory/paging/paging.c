#include "paging.h"
#include "memory/heap/kheap.h"
#include <stdbool.h>
#include "status.h"

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

bool paging_is_aligned(void* address)
{
    return ((uint32_t)address % PAGING_PAGE_SIZE) == 0;
}

//input virutal address
//output direct index and table index
int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    int res = 0;
    if(!paging_is_aligned(virtual_address))
    {
        res = - EINVARG;
        goto out;
    }
    //4KB page size: 12 bit offset
    //1024 page entry: 10 bit index
    //1024 direcotry::10 bit index
    //the below operation is a bit wise shift, taking the upper 10 bit of the virtual address
    *directory_index_out = ((uint32_t)virtual_address) / (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);

    *table_index_out = ((uint32_t)virtual_address / PAGING_PAGE_SIZE)  % PAGING_TOTAL_ENTRIES_PER_TABLE;

out:
    return res;
}

//set the entry corresponding to the virtual_addr to val
int paging_set(uint32_t* directory, void* virtual_addr, uint32_t val)
{
    if(!paging_is_aligned(virtual_addr))
    {
        return -EINVARG;
    }

    uint32_t direcotry_index = 0;
    uint32_t table_index = 0;
    
    int res = paging_get_indexes(virtual_addr, &direcotry_index, &table_index);
    if(res < 0)
        return res;
    
    uint32_t entry = directory[direcotry_index];
    //note that the lower 12 bit of the direcotry entry is protection info and other things
    //only the upper 20 bits are the address of the page table
    uint32_t* table = (uint32_t*)(entry & 0xfffff000);
    table[table_index] = val;
    return 0;
}