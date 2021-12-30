#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "std/stdio.h"
#include "memory/paging/paging.h"
#include "disk/disk.h"
#include "filesystem/path_parser.h"

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

    //-------search and init the disks-------//
    disk_search_and_init();

    //-------load our interrupt descriptor table-------//
    idt_init();
    

    //-------enable paging-------//
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    paging_switch(paging_4gb_chunk_get_directory(kernel_chunk));
    
    enable_paging();

    //-------testing reading from HD-------//
    // char buf[512];
    // disk_read_sector(0, 1, buf);


    //-------enable te interrupts-------//
    enable_interrupts();

    //-------testing our path parser-------//
    struct path_root* root_path = pathparser_parse("0:/bin/shell.exe", NULL);

    if(root_path)
    {
        
    }
}