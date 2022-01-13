#ifndef IDT_H
#define IDT_H

#include <stdint.h>

struct interrupt_frame;

//function pointers declaration
typedef void*(*ISR80H_COMMAND)(struct interrupt_frame* frame);

//the interrupt descriptor
struct idt_desc
{
    uint16_t offset_1;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr; //O, DPL, S, TYPE refer to "OS dev interrupt descriptor table"
    uint16_t  offset_2;

} __attribute__((packed));

//descriptor for the location of the idt
struct idtr_desc
{
    uint16_t limit;
    uint32_t base;
}__attribute__((packed));


//the general purpise registers we want to store when a user process traps into the kernel
struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

typedef void(*INTERRUPT_CALLBACK_FUNCTION)();

void idt_init();
void enable_interrupts();
void disable_interrupts();
void isr80h_register_command(int command_id, ISR80H_COMMAND command);
int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback);

#endif