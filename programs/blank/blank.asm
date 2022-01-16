[BITS 32]
section .asm

global loop


loop:
    call getkey
    push eax
    mov eax, 3 ; Command putchar
    int 0x80
    add esp, 4
    jmp _start

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