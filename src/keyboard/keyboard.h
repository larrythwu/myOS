#ifndef KEYBOARD_H
#define KEYBOARD_H

struct process;

typedef int (*KEYBOARD_INIT_FUNCTION)();
#define KEYBOARD_CAPS_LOCK_ON 1
#define KEYBOARD_CAPS_LOCK_OFF 0

typedef int KEYBOARD_CAPS_LOCK_STATE;

//represent a keybaord abstraction 
struct keyboard
{
    //function pointer to the init 
    KEYBOARD_INIT_FUNCTION init;
    char name[20];
    //linked list of keyboard 
    struct keyboard* next;
    KEYBOARD_CAPS_LOCK_STATE capslock_state;
};

void keyboard_init();
void keyboard_backspace(struct process* process);
void keyboard_push(char c);
char keyboard_pop();
int keyboard_insert(struct keyboard* keyboard);
void keyboard_set_capslock(struct keyboard* keyboard, KEYBOARD_CAPS_LOCK_STATE state);
KEYBOARD_CAPS_LOCK_STATE keyboard_get_capslock(struct keyboard* keyboard);

#endif 