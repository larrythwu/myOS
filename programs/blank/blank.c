#include "myos.h"

int main(int argc, char** argv)
{
    while(1) 
    {
        int c = getkey();
        if (c != 0)
        {
            print((char*)(&c));
        }
    }
} 