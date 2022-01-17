#include "myos.h"

int sys_getkeyblock()
{
    int val = 0;
    do
    {
        val = getkey();
    }
    while(val == 0);
    return val;
}

//get an entire string of input of length max from the user keyboard, it will block until the max number of input is met
//it will also print out the char as the key is pressed
void sys_terminal_readline(char* out, int max, bool output_while_typing)
{
    int i = 0;
    for (i = 0; i < max -1; i++)
    {
        char key = sys_getkeyblock();

        // Carriage return means we have read the line
        if (key == 13)
        {
            break;
        }

        if (output_while_typing)
        {
            sys_putchar(key);
        }

        // Backspace
        if (key == 0x08 && i >= 1)
        {
            out[i-1] = 0x00;
            // -2 because we will +1 when we go to the continue
            i -= 2;
            continue;
        }

        out[i] = key;
    }

    // Add the null terminator
    out[i] = 0x00;
} 