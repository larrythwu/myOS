#ifndef IDT_H
#define IDT_H

#include <stdint.h>

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

void idt_init();
void enable_interrupts();
void disable_interrupts();
#endif