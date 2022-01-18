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
#include "disk/streamer.h"
#include "std/string.h"
#include "filesystem/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"

void panic(const char* msg)
{
    //print(msg);
    while(1) {}
}

//our asm divide by zero code
extern void problem();

static struct paging_4gb_chunk* kernel_chunk;

struct tss tss;
struct gdt gdt_real[MYOS_TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured[MYOS_TOTAL_GDT_SEGMENTS] = {
    {.base = 0x00, .limit = 0x00, .type = 0x00},                // NULL Segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x9a},           // Kernel code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0x92},            // Kernel data segment
    //note that the user code and data are in the same memory range as the kernel, this is fine since we uses paging to set the permissions
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf8},              // User code segment
    {.base = 0x00, .limit = 0xffffffff, .type = 0xf2},             // User data segment
    {.base = (uint32_t)&tss, .limit=sizeof(tss), .type = 0xE9}      // TSS Segment
};

//switch to the kernel page directory
void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

//our main function
void kernel_main()
{
    terminal_initialize();
    //print("Welcome Larry!\n");
    //print("myOS\n");

    //---------loading the GDT-----------//
    //zero of the gdt_real structure
    memset(gdt_real, 0x00, sizeof(gdt_real));
    //convert our kernel construct into the acutal gdt encoding
    gdt_structured_to_gdt(gdt_real, gdt_structured, MYOS_TOTAL_GDT_SEGMENTS);
    // Load the gdt 
    gdt_load(gdt_real, sizeof(gdt_real));

    //----------setting up the task state segment-----------//
    memset(&tss, 0x00, sizeof(tss));
    tss.esp0 = 0x600000;
    tss.ss0 = KERNEL_DATA_SELECTOR;
    //0x28 is the offset in the gdt_real structure 
    tss_load(0x28);

    //-------initailize our kernel space heap-------//
    kheap_init();

    //-------initialize the file system----------//
    //insert FAT16 into our list of fs
    fs_init();

    //-------search and init the disks-------//
    disk_search_and_init();

    //-------load our interrupt descriptor table-------//
    idt_init();
    

    //-------enable paging-------//
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL);
    
    paging_switch(kernel_chunk);
    
    enable_paging();

    //---------init the syscall handlers--------//
    isr80h_register_commands();
    
    //-----------init keyboard-----------//
    keyboard_init();

    //-----------testing our new interrupt handler functions----------//
    //idt_register_interrupt_callback(32, pic_timer_callback);

    //-------load our user program------//
    struct process* process = 0;
    //print("loading process\n");

    int res = process_load_switch("0:/blank.elf", &process);
    //print("loaded process\n");

    if (res != 0)
    {
        panic("Failed to load the user program");
    }
    
    struct command_argument argument;
    strcpy(argument.argument, "Testing!");
    argument.next = 0x00; 

    process_inject_arguments(process, &argument);

    task_run_first_ever_task();

    //-------enable te interrupts-------//
    //enable_interrupts();

    
    while(1){}
}