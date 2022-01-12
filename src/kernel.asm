[BITS 32]; all code after this is seem as 32 bit code 

global _start ;export the sympol
global problem
global kernel_registers

extern kernel_main

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
    
    ; remap the master PIC
    ; Google x86 Assembly / Programmbale Interrupt Contorller
    
    ; restart PCI_1
    mov al, 0x11
    out 0x20, al
    
    ; tell PCI to set the vector offset to 0x20 (32)
    ; so now an IRQ0 interrupt with tirgger int 0x20 routine which we will define
    mov al, 0x20
    out 0x21, al

    ;done with PCI_1  
    mov al, 0x01
    out 0x21, al

    ; enable interrupts 
    ; now we enable this a little too early here, because tghe IDt is not set up yet
    ; we will fix this later
    ;sti

    call kernel_main

    jmp $

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov gs, ax
    mov fs, ax
    ret


; a division by zero function, to test out our interrupts in protected mode
problem:
    mov edx, 0
    mov eax, 0
    div eax    
    

times 512-($ - $$) db 0
