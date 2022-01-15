#include "elfloader.h"
#include "filesystem/file.h"
#include "status.h"
#include <stdbool.h>
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "std/string.h"
#include "memory/paging/paging.h"
#include "kernel.h"
#include "config.h"

//the elf signature, for the ox7f value look into the elf spec 
const char elf_signature[] = {0x7f, 'E', 'L', 'F'};

//comapre if the buffer matches the elf signature
static bool elf_valid_signature(void* buffer)
{
    return memcmp(buffer, (void*) elf_signature, sizeof(elf_signature)) == 0;
}

//check if the elfile class is compatible with out system
static bool elf_valid_class(struct elf_header* header)
{
    // We only support 32 bit binaries.
    return header->e_ident[EI_CLASS] == ELFCLASSNONE || header->e_ident[EI_CLASS] == ELFCLASS32;
}

//check the encoding is correct
static bool elf_valid_encoding(struct elf_header* header)
{
    return header->e_ident[EI_DATA] == ELFDATANONE || header->e_ident[EI_DATA] == ELFDATA2LSB;
}

//check if the elfile is executable, and the entry point makes sense
static bool elf_is_executable(struct elf_header* header)
{
    return header->e_type == ET_EXEC && header->e_entry >= MYOS_PROGRAM_VIRTUAL_ADDRESS;
}

//check if there is a program header
static bool elf_has_program_header(struct elf_header* header)
{
    return header->e_phoff != 0;
}

//return the elf memroy pointer
void* elf_memory(struct elf_file* file)
{
    return file->elf_memory;
}

//the very first elf address is the header so we return the addr of the beginiing memory
struct elf_header* elf_header(struct elf_file* file)
{
    return file->elf_memory;
}

//return the address of the section header
struct elf32_shdr* elf_sheader(struct elf_header* header)
{
    return (struct elf32_shdr*)((int)header+header->e_shoff);
}

//return the address of the program header
struct elf32_phdr* elf_pheader(struct elf_header* header)
{
    if(header->e_phoff == 0)
    {
        return 0;
    }

    return (struct elf32_phdr*)((int)header + header->e_phoff);
}

//access the entry inside the program header array at the index
struct elf32_phdr* elf_program_header(struct elf_header* header, int index)
{
    return &elf_pheader(header)[index];
}

//access and return the entry inside the sector header
struct elf32_shdr* elf_section(struct elf_header* header, int index)
{
    return &elf_sheader(header)[index];
}


void* elf_phdr_phys_address(struct elf_file* file, struct elf32_phdr* phdr)
{
    return elf_memory(file)+phdr->p_offset;
}

//return the address of the string table
char* elf_str_table(struct elf_header* header)
{
    return (char*) header + elf_section(header, header->e_shstrndx)->sh_offset;
}

void* elf_virtual_base(struct elf_file* file)
{
    return file->virtual_base_address;
}

void* elf_virtual_end(struct elf_file* file)
{
    return file->virtual_end_address;
}

void* elf_phys_base(struct elf_file* file)
{
    return file->physical_base_address;
}

void* elf_phys_end(struct elf_file* file)
{
    return file->physical_end_address;
}

//check if the header provided is loadable into the memory
int elf_validate_loaded(struct elf_header* header)
{
    //must be a elfiile, have the right class, encoding, and has a program header
    return (elf_valid_signature(header) && elf_valid_class(header) && elf_valid_encoding(header) && elf_has_program_header(header)) ? ALL_OK : -EINFORMAT;
}

//setting the elfile header's virtual and physical address form program header 
int elf_process_phdr_pt_load(struct elf_file* elf_file, struct elf32_phdr* phdr)
{
    //we want our elf_file's stating virutal address to equal to the smallest virtual address among all the program headers inside this elfile header
    //the looping is done in elf_process_pheaders
    if (elf_file->virtual_base_address >= (void*) phdr->p_vaddr || elf_file->virtual_base_address == 0x00)
    {
        elf_file->virtual_base_address = (void*) phdr->p_vaddr;
        elf_file->physical_base_address = elf_memory(elf_file)+phdr->p_offset;
    }
    //we want the elffile header's virutal and phyeiscal hed to be the largest virutal address amoung all the program headers
    unsigned int end_virtual_address = phdr->p_vaddr + phdr->p_filesz;
    if (elf_file->virtual_end_address <= (void*)(end_virtual_address) || elf_file->virtual_end_address == 0x00)
    {
        elf_file->virtual_end_address = (void*) end_virtual_address;
        elf_file->physical_end_address = elf_memory(elf_file)+phdr->p_offset+phdr->p_filesz;
    }
    return 0;
}

//process a single program header base on its p_type
int elf_process_pheader(struct elf_file* elf_file, struct elf32_phdr* phdr)
{
    int res = 0;
    switch(phdr->p_type)
    {
        case PT_LOAD:
            res = elf_process_phdr_pt_load(elf_file, phdr);
        break;
    }
    return res;
}

//loop through the program headers inside this elf_file object and process each one of them base on their p_type
int elf_process_pheaders(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_header(elf_file);
    //loop through all the program headers we have
    for(int i = 0; i < header->e_phnum; i++)
    {
        //get one entry of the program header
        struct elf32_phdr* phdr = elf_program_header(header, i);
        res = elf_process_pheader(elf_file, phdr);
        if (res < 0)
        {
            break;
        }

    }
    return res;
}

//validate the elf header and call function to load the program header
int elf_process_loaded(struct elf_file* elf_file)
{
    int res = 0;
    struct elf_header* header = elf_header(elf_file);
    //check the elfile memory is supported 
    res = elf_validate_loaded(header);
    if (res < 0)
    {
        goto out;
    }
    //process the program header
    res = elf_process_pheaders(elf_file);
    if (res < 0)
    {
        goto out;
    }

out:
    return res;
}

//we will allocate a brand new elfile in memory and point file_out to it 
int elf_load(const char* filename, struct elf_file** file_out)
{
    struct elf_file* elf_file = kzalloc(sizeof(struct elf_file));
    int fd = 0;
    //open the file for read 
    int res = fopen(filename, "r");
    if (res <= 0)
    {
        res = -EIO;
        goto out;
    }

    fd = res;

    //use fstate to get the file size 
    struct file_stat stat;
    res = fstat(fd, &stat);
    if (res < 0)
    {
        goto out;
    }

    //read the entire file into the memory 
    elf_file->elf_memory = kzalloc(stat.filesize);
    res = fread(elf_file->elf_memory, stat.filesize, 1, fd);
    if (res < 0)
    {
        goto out;
    }

    res = elf_process_loaded(elf_file);
    if(res < 0)
    {
        goto out;
    }

    //set the output pointer
    *file_out = elf_file;

out:
    fclose(fd);
    return res;
}

void elf_close(struct elf_file* file)
{
    if (!file)
        return;

    kfree(file->elf_memory);
    kfree(file);
}