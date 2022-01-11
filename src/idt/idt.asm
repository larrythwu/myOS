section .asm

; C functions imports 
extern int21h_handler
extern no_interrupt_handler
extern isr80h_handler

; asm function export to C
global int21h
global idt_load
global no_interrupt
global enable_interrupts
global disable_interrupts
global isr80h_wrapper

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
int21h:
    pushad
    call int21h_handler
    popad
    iret

no_interrupt:
    pushad
    call no_interrupt_handler
    popad
    iret

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