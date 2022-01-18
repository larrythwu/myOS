#include "myos.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main(int argc, char** argv)
{
    // void* ptr = malloc(512);
    // if (ptr)
    // {

    // }
    // free(ptr);
    // print(itoa(69));
    // putchar('A');
    // printf("The number is %i\n", 69);

    //char buf[1024];
    //sys_terminal_readline(buf, sizeof(buf), true);

    //print(buf);

    // char words[] = "hello how are you";
    // const char* token = strtok(words, " ");
    // while(token)
    // {
    //     printf("%s\n", token);
    //     token = strtok(NULL, " ");
    // }

    // char* ptr = malloc(20);
    // strcpy(ptr, "hello world");

    // print(ptr);

    // free(ptr);

    // ptr[0] = 'B';
    // print("abc\n");

    // char str[] = "rm -rf";
    // struct command_argument* root_command = sys_parse_command(str, sizeof(str));
    // printf("%s\n", root_command->argument);
    // printf("%s\n", root_command->next->argument);

    // struct process_arguments arguments;
    // sys_process_get_arguments(&arguments);

    // printf("%i %s\n", arguments.argc, arguments.argv[0]);
    print(argv[0]);

    while(1) 
    {
        // int c = getkey();
        // if (c != 0)
        // {
        //     print((char*)(&c));
        // }
    }
} 