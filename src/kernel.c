#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "std/stdio.h"
#include "memory/paging/paging.h"

//our asm divide by zero code
extern void problem();

static struct paging_4gb_chunk* kernel_chunk;

//our main function
void kernel_main()
{
    terminal_initialize();
    print("Welcome Larry!\n");
    print("myOS\n");

    //-------initailize our kernel space heap-------//
    kheap_init();
        
    //-------load our interrupt descriptor table-------//
    idt_init();
    
    

    //-------enable paging-------//
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));
    
    enable_paging();

    //-------testing our paging system-------------//
    //we allocated one page of memory 
    char* ptr = kzalloc(4096);
    //we can change the mapping of one entry in the table to point this physcial mem
    //we point the virtual address 0x1000 to our newly allocated memory space
    paging_set(paging_4gb_chunk_get_directory(kernel_chunk), (void*)0x1000, (uint32_t)ptr | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE | PAGING_IS_PRESENT);

    char* ptr2 = (char*)0x1000;
    ptr2[0] = 'A';
    ptr2[1] = 'B';
    //so basically there is now two virtual memory pointing to the same physical address?
    //we set 0x1000 => whatever kzalloc returns (say 0xAAA)
    //ptr (from zalloc) => linear mapping by default (0xAAA)
    //ptr2(ox1000 virutal)  => 0xAAA
    print(ptr2);
    print(ptr);

    //-------enable te interrupts-------//
    enable_interrupts();
}