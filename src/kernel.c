#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "std/stdio.h"


//our asm divide by zero code
extern void problem();

//our main function
void kernel_main()
{
    terminal_initialize();
    print("Welcome Larry!\n");
    print("myOS\n");

    //initailize our kernel space heap
    kheap_init();
        
    //load our interrupt descriptor table
    idt_init();
    
    void* ptr = kmalloc(50);
    void* ptr2 = kmalloc(5000);
    kfree(ptr);
    void* ptr3 = kmalloc(4000);
    //test out our interrupt
    //problem();
    //outb(0x60, 0xff);
    if(ptr || ptr2 || ptr3)
    {

    }
}