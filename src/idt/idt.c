#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "std/stdio.h"
#include "task/task.h"
#include "status.h"
#include "task/process.h"

struct idt_desc idt_descriptors[MYOS_TOTAL_INTERRUPTS];
struct idtr_desc idt_descriptor;

//this is an array of function pointers that point to the syscall handlers
static ISR80H_COMMAND isr80h_commands[MYOS_MAX_ISR80H_COMMANDS];
//contain the pointer to the interrupt handler in asm
extern void* interrupt_pointer_table[MYOS_TOTAL_INTERRUPTS];

//the function defined in idt.asm which load the idt using lidt asm function
//which uses the address of the idtr_drcriptor in the memory
extern void idt_load(struct idtr_desc* ptr);

extern void int21h();
extern void no_interrupt();
extern void isr80h_wrapper();

//this array hold 512 interrupt handler function pointers, used by the general interrupt handler function
static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[MYOS_TOTAL_INTERRUPTS];
void idt_handle_exception();

//our keyboared interrupt handler
// void int21h_handler()
// {
//     print("[INT] Pressed Key\n");
//     //End of Interrupt signal that we need to send to the PIC at the end of the Interrrupt Service Routine
//     //so that PIC canb reset the service register, EOI is sent by wrting ox20 to port 0x20 or 0x20 to 0xA0
//     outb(0x20, 0x20);
// }

//this is just a placeholder for all the IRQ interrupts that we don't have an routine created yet
void no_interrupt_handler()
{
    //print("[INT] No service routine defined");
    outb(0x20, 0x20);
}

//our handler function ofr int 0
//traditional int 0 is for divide by 0 exceptions
void idt_zero()
{
    print("[INT] Divide by 0 exception\n");
}

//set an entry of the interrupt table
//interrupt_no: the interrupt number we call ie. int 0x13
//address: the mem address of the handler function
void idt_set(int interrupt_no, void* address)
{
    struct idt_desc* desc = &idt_descriptors[interrupt_no];
    //lower two byte of the address
    desc->offset_1 = (uint32_t) address & 0x0000ffff;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0x00;
    desc->type_attr = 0xEE;
    desc->offset_2 = (uint32_t) address >> 16;

}

void idt_clock()
{
    outb(0x20, 0x20);
    // Switch to the next task
    task_next();
}


//set up the intterupt descriptor table 
void idt_init()
{
    //zero out our descriptors
    memset(idt_descriptors, 0 , sizeof(idt_descriptors));
    idt_descriptor.limit = sizeof(idt_descriptors)-1;
    idt_descriptor.base = (uint32_t)idt_descriptors;
    
    //herer we set all the IRQ and software interrrupts to the macro defined general handler
    for(int i = 0; i < MYOS_TOTAL_INTERRUPTS; i++){
        idt_set(i, interrupt_pointer_table[i]);
    }
    
    //idt_set(0, idt_zero);
    idt_set(0x80, isr80h_wrapper);
    //timer
    //idt_set(0x20, idt_clock);

    for (int i = 0; i < 0x20; i++)
    {
        idt_register_interrupt_callback(i, idt_handle_exception);
    }
    idt_register_interrupt_callback(0x20, idt_clock);
    //load the idt through the asm code
    idt_load(&idt_descriptor);
    //print("Loaded IDT\n");
}

void* isr80h_handle_command(int command, struct interrupt_frame* frame)
{
    void* result = 0;

    //check if the command is within range
    if(command < 0 || command >= MYOS_MAX_ISR80H_COMMANDS)
    {
        return 0;
    }

    //get the function pointer
    ISR80H_COMMAND command_func = isr80h_commands[command];
    //the syscall does not exist
    if (!command_func)
    {
        return 0;
    }
    //calling the function pointer
    result = command_func(frame);
    return result;
}


//we set the function pointer array osr80h_commands to the right functiopn pointer
void isr80h_register_command(int command_id, ISR80H_COMMAND command)
{
    if (command_id < 0 || command_id >= MYOS_MAX_ISR80H_COMMANDS)
    {
        panic("The command is out of bounds\n");
    }

    if (isr80h_commands[command_id])
    {
        panic("Your attempting to overwrite an existing command\n");
    }

    isr80h_commands[command_id] = command;
}

void* isr80h_handler(int command, struct interrupt_frame* frame)
{
    void* res = 0;
    //switch to kernel paging 
    kernel_page();
    //save the current register info into the frame object
    task_current_save_state(frame);
    //execute the command 
    res = isr80h_handle_command(command, frame);
    //switch back to the task page
    task_page();
    //we return back to the int80h wrapper which will send us back to the user land
    return res;
} 

//set one entry of the interrupt_callbacks func pointer array
int idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK_FUNCTION interrupt_callback)
{
    if (interrupt < 0 || interrupt >= MYOS_TOTAL_INTERRUPTS)
    {
        return -EINVARG;
    }

    interrupt_callbacks[interrupt] = interrupt_callback;
    return 0;
}

//the function to direct all interrupt to the right handler base on their int number
void interrupt_handler(int interrupt, struct interrupt_frame* frame)
{
    kernel_page();
    if (interrupt_callbacks[interrupt] != 0)
    {
        task_current_save_state(frame);
        interrupt_callbacks[interrupt](frame);
    }

    task_page();
    outb(0x20, 0x20);
}

void idt_handle_exception()
{
    print("Exception recieved: terminating the process...\n");
    process_terminate(task_current()->process);
    task_next();
}