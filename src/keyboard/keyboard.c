#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"
#include "classic.h"

//the head and the tail of the keyboard linked list 
static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_last = 0;

void keyboard_init()
{
    //insert our ps2 classic keyboard 
    keyboard_insert(classic_init());
}

//add the keyboard to the linked list and call its init function
int keyboard_insert(struct keyboard* keyboard)
{
    int res = 0;
    //if the init fucntion does not exist 
    if (keyboard->init == 0)
    {
        res = -EINVARG;
        goto out;
    }

    //appending the new keyboard to the end of the linked list
    if (keyboard_list_last)
    {
        keyboard_list_last->next = keyboard;
        keyboard_list_last = keyboard;
    }
    else //the first keyboard 
    {
        keyboard_list_head = keyboard;
        keyboard_list_last = keyboard;
    }

    res = keyboard->init();

out:
    return res;
}

//return the tail index of the buffer 
static int keyboard_get_tail_index(struct process* process)
{
    return process->keyboard.tail % sizeof(process->keyboard.buffer);
}

//deleting the tail of the buffer
void keyboard_backspace(struct process* process)
{
    process->keyboard.tail--;
    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = 0x00;
}

//push a new character into the keybaord buffer
void keyboard_push(char c)
{
    struct process* process = process_current();
    if (!process)
    {
        return;
    }
    //if the character is null 
    if(c == 0)
    {
        return;
    }
    //get the tail index from the keyboard buffer 
    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = c;
    //increment the buffer tail
    process->keyboard.tail++;
}


char keyboard_pop()
{
    if (!task_current())
    {
        return 0;
    }

    struct process* process = task_current()->process;
    int real_index = process->keyboard.head % sizeof(process->keyboard.buffer);
    char c = process->keyboard.buffer[real_index];
    if (c == 0x00)
    {
        // Nothing to pop return zero.
        return 0;
    }

    process->keyboard.buffer[real_index] = 0;
    process->keyboard.head++;
    return c;
} 

void keyboard_set_capslock(struct keyboard* keyboard, KEYBOARD_CAPS_LOCK_STATE state)
{
    keyboard->capslock_state = state;
}

KEYBOARD_CAPS_LOCK_STATE keyboard_get_capslock(struct keyboard* keyboard)
{
    return keyboard->capslock_state;
}

