[BITS 32]
section .asm

global _start

_start:
; just a loop

    push 0x20
    push 0x30
    mov eax, 0 ;command 0 syscall
    int 0x80
    add esp, 8 ;pop the stack
    jmp $