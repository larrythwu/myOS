[BITS 32]
section .asm

global _start

_start:
; just a loop

    
    push message ; note this push in the message address
    mov eax, 1 ; Command print
    int 0x80
    add esp, 4 ;pop the stack
    jmp $

section .data
message: db 'Trapping into the fucking kernel!', 0 