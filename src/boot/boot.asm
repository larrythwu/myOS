;need specify the assembly origin to the CPU
;so that the processor know what mem offset to use when dealing with data

ORG  0x7c00;ORG specify the origin 
BITS 16 ;we are using 16 bit architecture, all assembly command are assemnled into 16 bit machien code

CODE_SEG equ gdt_code - gdt_start 
DATA_SEG equ gdt_data - gdt_start

_start: 
    jmp short start
    nop

; the BIOS paramenter block will go here

times 33 db 0 ;create 33 bytes here filled with 0

start:
    jmp 0:code ; ensures that our code segment start at 0x7c0


code:
    cli ; clear interrupts
    mov ax, 0
    mov ds, ax
    mov es, ax
    ; set the stack segment to be 0
    mov ax, 0x00
    mov ss, ax
    ; set stack pointer to 7c00
    mov sp, 0x7c00
    sti ; enable interrupts

; Entering protected mode
.load_protected:
    cli ; disable the interrupt
    lgdt[gdr_descriptor]
    ;note how the instructions changes after this point 
    mov eax, cr0
    or eax, 0x1
    mov cr0, eax
    jmp CODE_SEG:load32

; GDT 
gdt_start:
gdt_null: ; a null segment in GDT, a total of 32 bits
    dd 0x00
    dd 0x00
    ; Offset 0x0

; offset 0x8
gdt_code:
    dw 0xffff ; segment limit of first 15 bits
    dw 0 ; the first 0-15 bits 
    db 0 ; base 13-23 bits 
    db 0x9a ; access byte. a bit mask
    db 11001111b ; high 4 bit flags and the low 4 bit flags 
    db 0 ;base 24 31 bits 
    ; the above values are basically the default settings 

; offset 0x10
gdt_data:
    dw 0xffff ; segment limit of first 15 bits
    dw 0 ; the first 0-15 bits 
    db 0 ; base 13-23 bits 
    db 0x92 ; access byte. a bit mask
    db 11001111b ; high 4 bit flags and the low 4 bit flags 
    db 0 ;base 24 31 bits 

gdt_end:

; this just gets the size of the gdt laoding code block that we have written above
gdr_descriptor:
    dw gdt_end - gdt_start-1
    dd gdt_start

[BITS 32]; all code after this is seem as 32 bit code 
load32: ;set up the 32 bit registers
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000 ;location of the kernel to load
    mov esp, ebp
    jmp $


times 510-($ - $$) db 0
dw 0xAA55 ;intel use little endian 
; note that the buffer is not loaded by the assembler, we are only loaded one sector of 512 bytes 
; from address 0x7c00, the next sector contains the data we want to load, however, the label buffer can still be referenced, its just the 
; address of the begining of the next sector