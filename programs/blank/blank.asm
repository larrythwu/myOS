[BITS 32]
section .asm

global _start

_start:
; just a loop

    
    push message ; note this push in the message address
    mov eax, 1 ; Command print
    int 0x80
    add esp, 4 ;pop the stack

    call getkey

    push keypress
    mov eax, 1 ; Command print
    int 0x80
    add esp, 4 ;pop the stack
    jmp $

getkey:
    mov eax, 2 ; Command getkey
    int 0x80
    ; if the key press is a null char, then we keep on reading til there is a actual key press
    ; and then we return
    cmp eax, 0x00
    je getkey
    ret


section .data
message: db 'Trapping into the fucking kernel!\n', 0 
keypress: db "Keypressed from user space!\n", 0