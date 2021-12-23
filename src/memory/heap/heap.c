#include "heap.h"
#include "kheap.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>
#include "std/stdio.h"

//check if our table allocation have enough entries to cover all the mem addr in the heap
static int heap_validate_heap_table(void* ptr, void*end, struct heap_table* table)
{
    int res = 0;
    size_t table_size = (size_t)(end - ptr);
    size_t total_pages = table_size / MYOS_HEAP_BLOCK_SIZE;
    if(table->total != total_pages)
    {
        res = -EINVARG;
        goto out;
    }
out:
    return res;
}

//check if our heap allocation is aligned properly
static int heap_validate_aligment(void* ptr)
{
    return ((unsigned int)ptr % MYOS_HEAP_BLOCK_SIZE)==0;
}

int heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    int res = 0;
    //make sure that starting point and the ending pooint are aligned to the heap block size
    if(!heap_validate_aligment(ptr) || !heap_validate_aligment(end))
    {
        res = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = ptr;
    heap->table = table;

    res = heap_validate_heap_table(ptr, end, table);
    if(res<0)
        goto out;

    //the total number of bytes our heap table needs
    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_TABLE_ENTRY_FREE, table_size);

out:
    return res;
}

static uint32_t heap_aligne_value_to_upper(uint32_t val)
{
    int rem = val % MYOS_HEAP_BLOCK_SIZE;

    //if not aligned we round up
    if(rem != 0)
    {
        val -= rem;
        val += MYOS_HEAP_BLOCK_SIZE;
    }
    return val;
}

//the type info is used to check if the correponding block is free or not 
//baiscally this just extract the lower four bites which s is the type info
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

//we pasrse through the entry table and find if there is 'total_blocks' of consevutive entries that are free
int heap_get_start_block(struct heap* heap, uint32_t total_blocks)
{
    struct heap_table* table = heap->table;
    //start and end block entry number
    //bs: starting block number
    //bc: number of free block collected, block count
    int bc = 0;
    int bs = -1;


    for(size_t i = 0; i < table->total; i++)
    {
        //here we check the type of every entry to see if the block is taken or not
        if(heap_get_entry_type(table->entries[i] != HEAP_BLOCK_TABLE_ENTRY_FREE))
        {
            bc = 0;
            bs = -1;
            continue;
        }    
        //now we know the current block is free
        //if this is the first block
        if(bs == -1)
        {
            bs = i;
        }
        bc++;
        //we found enough blocks
        if(bc == total_blocks)
        {
            break;
        }
    }

    //if no start point is found we return no memory error
    if(bs == -1)
    {
        return -ENOMEM;
    }

    return bs;

}

//translate the block number to the actual  
void* heap_block_to_adress(struct heap* heap, int block)
{
    return heap->saddr + (block * MYOS_HEAP_BLOCK_SIZE);

}

//mark the range of blocks as taken in the entry table
void heap_mark_blocks_taken(struct heap* heap, int start_block, int total_blocks)
{
    print("starting block = ");
    printn(start_block);

    print("total_blocks = ");
    printn(total_blocks);

    int end_block = (start_block + total_blocks - 1);
    //set the mask for our first block
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    if(total_blocks > 1)
    {
        entry |= HEAP_BLOCK_HAS_NEXT;

    }

    for(int i = start_block; i <= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_TABLE_ENTRY_TAKEN;
        
        //if we are not at the last block, we need to indicate that it still has next 
        if( i != end_block-1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void* heap_malloc_blocks(struct heap* heap, int total_blocks)
{
    void* address = 0;
    
    //see if we can find a consevutive of array of blocks that is 
    //of the size we need
    int start_block = heap_get_start_block(heap, total_blocks);


    
    if(start_block < 0)
    {
        //if we get an error, we will return address 0x0 (NULL)
        goto out;
    }

    //get the absolute memaddress 
    address = heap_block_to_adress(heap, start_block);
    
    //mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;

}

//allocate an aligned block of memory of the sepcified size
//if no memory is available then return a null address 
//heap: a struct that stores the starting addr of the heap memory
//      and the address of the heap entry table for keeping records
//size: the size of the memory block that we want to allocate 
void* heap_malloc(struct heap* heap, size_t size)
{
    //get how many aligned block we need to cover the requested size
    size_t aligned_size = heap_aligne_value_to_upper(size);
    int total_blocks = aligned_size / MYOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

int heap_address_to_block(struct heap* heap, void* address)
{
    return ((int)(address) - (int)heap->saddr)/MYOS_HEAP_BLOCK_SIZE;
}

void heap_mark_blocks_free(struct heap* heap, int starting_block)
{
    struct heap_table* table = heap->table;
    
    for(int i = starting_block; i < (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if(!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

//mark the all the entries starting at ptr to FREE
//heap: the pointer to the heap struct which stores the table
//ptr: starting address to be freed
void heap_free(struct heap* heap, void* ptr)
{
    //we don't need the number of block since we have the "NEXT" flag
    //we just free all that is "next" of the starting block
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}