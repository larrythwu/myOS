[BITS 32]; all code after this is seem as 32 bit code 

global _start ;export the sympol

CODE_SEG equ 0x08
DATA_SEG equ 0x10


_start: ;set up the 32 bit registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000 ;location of the kernel to load
    mov esp, ebp

        
    ;enable a20 line
    in al, 0x92
    or al, 2
    out 0x92, al
    
    jmp $

times 512-($ - $$) db 0
