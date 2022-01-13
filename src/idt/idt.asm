section .asm

; C functions imports 
extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler
extern interrupt_handler

; asm function export to C
global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper
global interrupt_pointer_table


enable_interrupts:
    sti
    ret

disable_interrupts:
    cli
    ret

; load the idt 
idt_load:
    push ebp 
    mov ebp, esp
    mov ebx, [ebp+8]
    
    lidt [ebx]

    pop ebp
    ret

; wrapper function for interrupt handlers, since we need to disable interrupt and reenabled it again
; when the routine is executed, this cannot bne done in C, so we pass in the C handler as 
; int21_handler and do the rest in asm
; int21h:
;     pushad
;     call int21h_handler
;     popad
;     iret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

; Macro for handling all interrupts, 512 of them, all the interrupt will can the c function interrrupt_handler
; and it will direct us to the right handler using the int number we pushed onto the stack
; this bascially just repeat this function 512 times in compile time
%macro interrupt 1
    global int%1
    int%1:
        ; INTERRUPT FRAME START
        ; ALREADY PUSHED TO US BY THE PROCESSOR UPON ENTRY TO THIS INTERRUPT
        ; uint32_t ip
        ; uint32_t cs;
        ; uint32_t flags
        ; uint32_t sp;
        ; uint32_t ss;
        ; Pushes the general purpose registers to the stack
        pushad
        ; Interrupt frame end
        push esp
        push dword %1
        call interrupt_handler
        add esp, 8
        popad
        iret
%endmacro

%assign i 0
%rep 512 
    interrupt i
%assign i i+1
%endrep

isr80h_wrapper:
    ; push the general purpose registers to the stack 
    ; now the registers like the mode bit and the user mode PC are stored by the CPU
    ; upon trapping into the Kernel, and this pushad just save the gerneral purpose ones

    ; these registers are pushed by the CPU
    ; uint32_t ip
    ; uint32_t cs;
    ; uint32_t flags
    ; uint32_t sp;
    ; uint32_t ss;
    pushad

    ; Push the stack pointer so that we are pointing to the interrupt frame
    ; the stack pointer esp now holds all the cpu pushed registers + the general purpose ones from pushad
    push esp

    ; EAX holds our command lets push it to the stack for isr80h_handler
    push eax
    ; call the C handler
    call isr80h_handler

    ; Now that the syscall is handled, we store the return value in tem_res
    mov dword[tmp_res], eax
    ; move the stack pointer, basically popping the stack
    add esp, 8
    ; Restore general purpose registers for user land
    popad
    mov eax, [tmp_res]
    ; going back to user space
    iretd


section .data
; Inside here is stored the return result from isr80h_handler
tmp_res: dd 0 

;====================
; Here we create the table that stores the address of the handler to each of the interrupt number
;=====================
%macro interrupt_array_entry 1
    dd int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep 