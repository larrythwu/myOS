#include "classic.h"
#include "keyboard.h"
#include "io/io.h"
#include <stdint.h>
#include <stddef.h>

int classic_keyboard_init();

//our scan code array, for translating scan code to ascii chars
//basically the keybaord will map every key to a number, and the number can be mapped to a ascii char
static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', 
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

//this is our acutal keyboard implementation, we set the function pointer to the implementation
struct keyboard classic_keyboard = {
    .name = {"Classic"},
    .init = classic_keyboard_init
};

//enable the keyboard through its command register port
int classic_keyboard_init()
{
    //PS_PORT is at 0x64, we write to it to enable the port, 0x64 is the address of the command register
    //The command to write to 0x64 to enable the keyboard is 0xAE, refer to OS Dev PS21 port for reference
    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT);
    return 0;
}

//translate scan code to char
uint8_t classic_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    if (scancode > size_of_keyboard_set_one)
    {
        return 0;
    }

    char c = keyboard_scan_set_one[scancode];
    return c;
}


void clasic_keyboard_handle_interrupt()
{

}

//return the our classic keyboard struct
struct keyboard* classic_init()
{
    return &classic_keyboard;
} 