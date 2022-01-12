#include "isr80h.h"
#include "idt/idt.h"
#include "misc.h"
#include "io.h"

//we register all the int 80h handlers here
//the acutal implementation is in misc.c
void isr80h_register_commands()
{
    //calling the interrupt descriptor table function
    isr80h_register_command(SYSTEM_COMMAND0_SUM, isr80h_command0_sum);
    isr80h_register_command(SYSTEM_COMMAND1_PRINT, isr80h_command1_print);
} 