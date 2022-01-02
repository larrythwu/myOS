;need specify the assembly origin to the CPU
;so that the processor know what mem offset to use when dealing with data

ORG  0x7c00;ORG specify the origin 
BITS 16 ;we are using 16 bit architecture, all assembly command are assemnled into 16 bit machien code

CODE_SEG equ gdt_code - gdt_start 
DATA_SEG equ gdt_data - gdt_start

jmp short start
nop

;============= the BIOS paramenter block ==============
OEMIdentifier           db 'MYOS    '   ;the space is to pad it to 8 byte (8 chars)
BytesPerSector          dw 0x200        ;512 bytes per sector, note that we are simply telling the system how the disk works, we cannot can the hardware spec of the disk so 512 is set
SectorsPerCluster       db 0x80         ;128 sectors per cluster
ReservedSectors         dw 200          ;200 reserved sectors, the space reserved for the kernel code
FATCopies               db 0x02         ;have 2 fat copies
RootDirEntries          dw 0x40         ;64 
NumSectors              dw 0x00         ;not used
MediaType               db 0xF8         
SectorsPerFat           dw 0x100        
SectorsPerTrack         dw 0x20
NumberOfHeads           dw 0x40
HiddenSectors           dd 0x00
SectorsBig              dd 0x773594

;|----- Extended BPB (Dos 4.0)-------|
DriveNumber             db 0x80
WinNTBit                db 0x00
Signature               db 0x29
VolumeID                dd 0xD105
VolumeIDString          db 'MYOS BOOT  ' ;must be 11 bytes
SystemIDString          db 'FAT16   '
;======================================================


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

    ;enable the protected mode bit
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

[BITS 32]
 load32:
    mov eax, 1
    mov ecx, 100
    mov edi, 0x0100000
    call ata_lba_read
    jmp CODE_SEG:0x0100000

;=================Kernel HD sector->memory loader=====================

ata_lba_read:
    mov ebx, eax, ; Backup the LBA
    ; Send the highest 8 bits of the lba to hard disk controller
    shr eax, 24
    or eax, 0xE0 ; Select the  master drive
    mov dx, 0x1F6
    out dx, al
    ; Finished sending the highest 8 bits of the lba

    ; Send the total sectors to read
    mov eax, ecx
    mov dx, 0x1F2
    out dx, al
    ; Finished sending the total sectors to read

    ; Send more bits of the LBA
    mov eax, ebx ; Restore the backup LBA
    mov dx, 0x1F3
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send more bits of the LBA
    mov dx, 0x1F4
    mov eax, ebx ; Restore the backup LBA
    shr eax, 8
    out dx, al
    ; Finished sending more bits of the LBA

    ; Send upper 16 bits of the LBA
    mov dx, 0x1F5
    mov eax, ebx ; Restore the backup LBA
    shr eax, 16
    out dx, al
    ; Finished sending upper 16 bits of the LBA

    mov dx, 0x1f7
    mov al, 0x20
    out dx, al

    ; Read all sectors into memory
.next_sector:
    push ecx

; Checking if we need to read
.try_again:
    mov dx, 0x1f7
    in al, dx
    test al, 8
    jz .try_again

; We need to read 256 words at a time
    mov ecx, 256
    mov dx, 0x1F0
    rep insw
    pop ecx
    loop .next_sector
    ; End of reading sectors into memory
    ret
;=============================================


times 510-($ - $$) db 0
dw 0xAA55 ;intel use little endian 
; note that the buffer is not loaded by the assembler, we are only loaded one sector of 512 bytes 
; from address 0x7c00, the next sector contains the data we want to load, however, the label buffer can still be referenced, its just the 
; address of the begining of the next sector