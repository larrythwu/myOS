#include "shell.h"
#include "stdio.h"
#include "stdlib.h"
#include "myos.h"

int main(int argc, char** argv)
{
    print("Welcome to Larry's OS v1.0.0\n");
    while(1) 
    {
        print("> ");
        char buf[1024];
        sys_terminal_readline(buf, sizeof(buf), true);
        print("\n");
        print(buf);
        print("\n");

    }
    return 0;
} 