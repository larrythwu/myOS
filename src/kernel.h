#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

void kernel_main();


#define ERROR(value) (void*)(value)
#define ERROR_I(value) (int)(value)
#define ISERR(value) ((int)value < 0)
void panic(const char* msg);
void kernel_page();
void kernel_registers();

#endif