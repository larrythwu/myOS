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
 void paging_switch(struct paging_4gb_chunk *directory)
 {
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
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

void paging_free_4gb(struct paging_4gb_chunk* chunk)
{
    for (int i = 0; i < 1024; i++)
    {
        uint32_t entry = chunk->directory_entry[i];
        //the lowest 12 bits are flags and not the address
        //FIXME: why not entry >> 12?
        uint32_t* table = (uint32_t*)(entry & 0xfffff000);
        kfree(table);
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}

//return the page size aligned address, roudning up
void* paging_align_address(void* ptr)
{
    if ((uint32_t)ptr % PAGING_PAGE_SIZE)
    {
        return (void*)((uint32_t)ptr + PAGING_PAGE_SIZE - ((uint32_t)ptr % PAGING_PAGE_SIZE));
    }

    return ptr;
}

int paging_map(struct paging_4gb_chunk* directory, void* virt, void* phys, int flags)
{
    //if not aligned then the address are erroreous
    if (((unsigned int)virt % PAGING_PAGE_SIZE) || ((unsigned int) phys % PAGING_PAGE_SIZE))
    {
        return -EINVARG;
    }

    return paging_set(directory->directory_entry, virt, (uint32_t) phys | flags);
}

//map the entire phsical address specified linear to the virutal address start point
int paging_map_range(struct paging_4gb_chunk* directory, void* virt, void* phys, int count, int flags)
{
    int res = 0;
    //count is the number of pages we need to map
    for (int i = 0; i < count; i++)
    {
        res = paging_map(directory, virt, phys, flags);
        if (res == 0)
            break;
        virt += PAGING_PAGE_SIZE;
        phys += PAGING_PAGE_SIZE;
    }

    return res;
}
//provide mapping between the viruta and the physical address provided here
 int paging_map_to(struct paging_4gb_chunk *directory, void *virt, void *phys, void *phys_end, int flags)
 {
    int res = 0;
    //if the virutal address is not aligned to the page size 
    if ((uint32_t)virt % PAGING_PAGE_SIZE)
    {
        res = -EINVARG;
        goto out;
    }
    if ((uint32_t)phys % PAGING_PAGE_SIZE)
    {
        res = -EINVARG;
        goto out;
    }
    if ((uint32_t)phys_end % PAGING_PAGE_SIZE)
    {
        res = -EINVARG;
        goto out;
    }

    if ((uint32_t)phys_end < (uint32_t)phys)
    {
        res = -EINVARG;
        goto out;
    }

    uint32_t total_bytes = phys_end - phys;
    int total_pages = total_bytes / PAGING_PAGE_SIZE;
    res = paging_map_range(directory, virt, phys, total_pages, flags);
out:
    return res;
}
