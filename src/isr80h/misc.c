#include "misc.h"
#include "idt/idt.h"

//this is just a trivial kernel function for summing two number
//just a hello world example
void* isr80h_command0_sum(struct interrupt_frame* frame)
{
    return 0;
} 