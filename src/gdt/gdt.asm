section .asm
global gdt_load

gdt_load:
    mov eax, [esp+4]

    ;we are writing to the gdt_descriptor from the argument we got 
    mov [gdt_descriptor + 2], eax
    mov ax, [esp+8]
    ;address of gdt structure
    mov [gdt_descriptor], ax

    lgdt [gdt_descriptor]
    ret

section .data
gdt_descriptor:
    dw 0x00 ; Size
    dd 0x00 ; GDT Start Address 