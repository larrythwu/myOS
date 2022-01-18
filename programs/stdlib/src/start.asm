[BITS 32]

global _start
extern c_start
extern exit

section .asm

_start:
    call c_start
    call exit
    ret