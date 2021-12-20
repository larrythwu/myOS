#include "kernel.h"
#include <stdint.h>
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"

uint16_t* video_mem = 0;
uint16_t terminal_row = 0;
uint16_t terminal_col=0;


uint16_t terminal_make_char(char c, char color)
{
    return (color << 8) | c;

}

//put a char at the position specified 
void terminal_putchar(int x, int y, char c, char color)
{
    video_mem[(y*VGA_WIDTH) + x] = terminal_make_char(c, color);
}

//write a char to the screen and move the position
void terminal_writechar(char c, char color)
{
    if(c=='\n')
    {
        terminal_row++;
        terminal_col = 0;
        return;
    }
    terminal_putchar(terminal_col, terminal_row, c, color);
    terminal_col ++;
    if(terminal_col >= VGA_WIDTH){
        terminal_col = 0;
        terminal_row ++;
    }

}

//clear the display to all black
void terminal_initialize()
{
    video_mem = (uint16_t*) 0xb8000;
    for(int y=0; y<VGA_HEIGHT; y++){
        for(int x=0; x<VGA_WIDTH; x++){
            terminal_putchar(x, y, ' ', 0);
        }

    }

}

//return the size of a string
size_t strlen(const char* str)
{
    size_t len = 0;
    
    while(str[len])
        len ++;

    return len;
}

//print a string to screen
void print(const char* str){
    size_t len = strlen(str);
    for(int i = 0; i< len; i++){
        terminal_writechar(str[i], 15);
    }
}

//our asm divide by zero code
extern void problem();

//our main function
void kernel_main()
{
    terminal_initialize();
    print("Welcome Larry!\n");
    print("myOS\n");

        
    //load our interrupt descriptor table
    idt_init();
    
    //test out our interrupt
    //problem();
    //outb(0x60, 0xff);
    
}