#ifndef KEYBOARD_H
#define KEYBOARD_H

struct process;

typedef int (*KEYBOARD_INIT_FUNCTION)();

//represent a keybaord abstraction 
struct keyboard
{
    //function pointer to the init 
    KEYBOARD_INIT_FUNCTION init;
    char name[20];
    //linked list of keyboard 
    struct keyboard* next;
};

void keyboard_init();
void keyboard_backspace(struct process* process);
void keyboard_push(char c);
char keyboard_pop();
int keyboard_insert(struct keyboard* keyboard);

#endif 