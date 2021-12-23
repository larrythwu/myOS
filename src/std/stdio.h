#ifndef STDIO_H
#define STDIO_H

#define VGA_WIDTH 80
#define VGA_HEIGHT 20

void print(const char* str);
void terminal_initialize();
void printn(int number);

#endif