[BITS 32]
section .asm

global _start

_start:
; just a loop

    ; lets run the syscall 0
    mov eax, 0
    int 0x80
    
    jmp $