#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "std/stdio.h"

struct idt_desc idt_descriptors[MYOS_TOTAL_INTERRUPTS];
struct idtr_desc idt_descriptor;

//the function defined in idt.asm which load the idt using lidt asm function
//which uses the address of the idtr_drcriptor in the memory
extern void idt_load(struct idtr_desc* ptr);

extern void int21h();
extern void no_interrupt();

//our keyboared interrupt handler
void int21h_handler()
{
    print("[INT] Pressed Key\n");
    //End of Interrupt signal that we need to send to the PIC at the end of the Interrrupt Service Routine
    //so that PIC canb reset the service register, EOI is sent by wrting ox20 to port 0x20 or 0x20 to 0xA0
    outb(0x20, 0x20);
}

//this is just a placeholder for all the IRQ interrupts that we don't have an routine created yet
void no_interrupt_handler()
{
    //print("[INT] No service routine defined");
    outb(0x20, 0x20);
}

//our handler function ofr int 0
//traditional int 0 is for divide by 0 exceptions
void idt_zero()
{
    print("[INT] Divide by 0 exception\n");
}

//set an entry of the interrupt table
//interrupt_no: the interrupt number we call ie. int 0x13
//address: the mem address of the handler function
void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    //lower two byte of the address
    desc->offset_1 = (uint32_t) address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t) address >> 16;

}

//set up the intterupt descriptor table 
void idt_init()
{
    //zero out our descriptors
    memset(idt_descriptors, 0 , sizeof(idt_descriptors));
    idt_descriptor.limit = sizeof(idt_descriptors)-1;
    idt_descriptor.base = (uint32_t)idt_descriptors;
    
    //herer we set all the IRQ and software interrrupts to the no_interrupt_handler as default value
    for(int i = 0; i < MYOS_TOTAL_INTERRUPTS; i++){
        idt_set(i, no_interrupt);
    }
    
    idt_set(0, idt_zero);
    idt_set(0x21, int21h);


    //load the idt through the asm code
    idt_load(&idt_descriptor);
    print("Loaded IDT\n");
}
