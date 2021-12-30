#include <stdint.h>
#include <stddef.h>
#include "stdio.h"
#include "memory/memory.h"

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
size_t len(const char* str)
{
    size_t len = 0;
    
    while(str[len])
        len ++;

    return len;
}

char numberArray[100];

//convert an integer into printable char array
char * toArray(int number)
{
    memset(numberArray, 0, sizeof(numberArray));
    if(number == 0)
    {
        numberArray[0] = '0';
        return numberArray;
    }
    int i = 0;
    int temp = number;
    while (temp != 0)
    {
        temp = temp / 10;
        i++;
    }
    i--;

    while(i>=0)
    {
        numberArray[i] = number % 10 + '0';
        i--;
        number = number / 10;
    }

    return numberArray;
}


//print a string to screen
void print(const char* str){
    size_t length = len(str);
    for(int i = 0; i< length; i++){
        terminal_writechar(str[i], 15);
    }
}

void printn(int number)
{
    //print("Printing integer\n");
    print(toArray(number));
    print("\n");
}
