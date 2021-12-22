#include "heap.h"
#include "kheap.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>


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
    return ((uint32_t)ptr % MYOS_HEAP_BLOCK_SIZE)==0;
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
    memset(table->entries, 0, table_size);

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
int heap_get_start_block(struct heap* heap, int total_blocks)
{
    struct heap_table* table = heap->table;
    int bc = 0;
    int bs = -1;


    for(size_t i = 0; i < table->total; i++)
    {
        //here we check the type of every entry to see if the block is taken or not
    }
}

void* heap_malloc_blocks(struct heap* heap, uint32_t total_blocks)
{
    void* address = 0;
    
    //see if we can find a consevutive of array of blocks that is 
    //of the size we need
    int start_block = heap_get_start_block(heap, total_blocks);
    
    if(start_block < 0)
    {
        goto out;
    }

    //get the absolute memaddress 
    address = heap_block_to_adress(heap, start_block);
    //mark the blocks as taken
    heap_mark_blocks_taken(heap, start_block, total_blocks);


    return address;

}

//allocate an aligned block of memory of size 
void* heap_malloc(struct heap* heap, size_t size)
{
    size_t aligned_size = heap_aligne_value_to_upper(size);
    uint32_t total_blocks = aligned_size / MYOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(void* ptr)
{
    return;
}