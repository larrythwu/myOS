#include "myos.h"
#include "stdlib.h"
#include "stdio.h"

int main(int argc, char** argv)
{
    void* ptr = malloc(512);
    if (ptr)
    {

    }
    free(ptr);
    print(itoa(69));
    putchar('A');
    printf("The number is %i\n", 69);

    char buf[1024];
    sys_terminal_readline(buf, sizeof(buf), true);

    print(buf);

    while(1) 
    {
        // int c = getkey();
        // if (c != 0)
        // {
        //     print((char*)(&c));
        // }
    }
} 