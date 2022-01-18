#ifndef PEACHOS_H
#define PEACHOS_H
#include <stddef.h>
#include <stdbool.h>

//command linked list items
struct command_argument
{
    char argument[512];
    struct command_argument* next;
};

struct process_arguments
{
    int argc;
    char** argv;
};

void print(const char* filename);
int getkey();  
void* sys_malloc(size_t size);
void sys_free(void* ptr);
void sys_putchar(char c);
int sys_getkeyblock();
void sys_terminal_readline(char* out, int max, bool output_while_typing);
void sys_process_load_start(const char* filename);
struct command_argument* sys_parse_command(const char* command, int max);
void sys_process_get_arguments(struct process_arguments* arguments);
int system_run(const char* command);
int system(struct command_argument* arguments);
void exit();
#endif 