#ifndef ISR80H_H
#define ISR80H_H

//enum is just auto assigning numeric value from 0
enum SystemCommands
{
    SYSTEM_COMMAND0_SUM,
};

void isr80h_register_commands();

#endif 