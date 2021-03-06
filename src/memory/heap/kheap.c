#include "kheap.h"
#include "heap.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "std/stdio.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init()
{
    //calculate how many table entries we need 
    int total_table_entries = MYOS_HEAP_SIZE_BYTE / MYOS_HEAP_BLOCK_SIZE;
    //specify the starting address of our table
    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)MYOS_HEAP_TABLE_ADRESS;
    kernel_heap_table.total = total_table_entries;

    void* end = (void*) (MYOS_HEAP_ADDRESS + MYOS_HEAP_SIZE_BYTE);
    int res = heap_create(&kernel_heap, (void*)(MYOS_HEAP_ADDRESS), end, &kernel_heap_table);
    if(res < 0)
    {
        //error
        print("FAILED to create a heap");
    }

}

void* kmalloc(size_t size)
{
    return heap_malloc(&kernel_heap, size);
}

void kfree(void* ptr)
{
    heap_free(&kernel_heap, ptr);
}

//allocate a chunk in the heap and then zero them out
void* kzalloc(size_t size){
    void* ptr = kmalloc(size);
    if(ptr==0)
        return 0;
    memset(ptr, 0, size);
    return ptr;
}