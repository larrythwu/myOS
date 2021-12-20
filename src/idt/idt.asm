section .asm

; C functions imports 
extern int21h_handler
extern no_interrupt_handler

; asm function export to C
global int21h
global idt_load
global no_interrupt

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
int21h:
    cli
    pushad
    call int21h_handler
    popad
    sti
    iret

no_interrupt:
    cli
    pushad
    call no_interrupt_handler
    popad
    sti
    iret