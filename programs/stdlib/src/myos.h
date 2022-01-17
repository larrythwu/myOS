#ifndef PEACHOS_H
#define PEACHOS_H
#include <stddef.h>
#include <stdbool.h>

void print(const char* filename);
int getkey();  
void* sys_malloc(size_t size);
void sys_free(void* ptr);
void sys_putchar(char c);
int sys_getkeyblock();
void sys_terminal_readline(char* out, int max, bool output_while_typing);

#endif 