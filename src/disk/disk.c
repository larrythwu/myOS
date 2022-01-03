#include "io/io.h"
#include "disk.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"
#include "filesystem/file.h"

struct disk disk;

//lba: the logical block number of the sector on the hardrive that we want to read
//total: how many sector to read
//buf: pointer to the memory location to store the read data
int disk_read_sector(int lba, int total, void* buf)
{
    //set 0xE0 for lba mode, the lower four bit contain the 24-27 bit of lba
    outb(0x1F6, (lba>>24) | 0xE0);
    //send the number of sectors we want to write
    outb(0x1F2, total);
    //send lower 8bit of the lba
    outb(0x1F3, (unsigned char)(lba & 0xff));
    //send bit 8 - 15 of LBA
    outb(0x1F4, (unsigned char)(lba >> 8));
    //send bit 16 - 23 of LBA
    outb(0x1F5, (unsigned char)(lba >> 16));
    //1f7 is the command port, 0x20 is read
    outb(0x1F7, 0x20);

    //we frame ptr to be a short because short is two byte
    unsigned short* ptr = (unsigned short*) buf;
    
    //we need to do this for every byte that we want to read
    for(int b = 0; b < total; b++)
    {
        //wait for the data return buffer to be ready
        char c = insb(0x1F7);
        while(! (c & 0x08))
        {
            c = insb(0x1F7);
        }

        //copy from hardrive to memory, 256 * 2 byte a time = 512 bytes, a sector from the hardrive  
        for(int i = 0; i < 256; i++)
        {
            //read two byte into our buffer
            *ptr = insw(0x1F0);
            //remember short is two byte
            ptr++;
        }

    }
    
    return 0;
}

//searching for all the disks and initialize/register them wiht the system
void disk_search_and_init()
{
    //we only have one disk right now, so no need for a searching mechanism yet   
    memset(&disk, 0, sizeof(disk));
    disk.id = 0;
    disk.type = MYOS_DISK_TYPE_REAL;
    disk.sector_size = MYOS_SECTOR_SIZE;
    disk.filesystem = fs_resolve(&disk);
}

//reading the disk from the struct
int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf)
{
    if(idisk != &disk)
    {
        return -EIO;
    }
    return disk_read_sector(lba, total, buf);
}

struct disk* disk_get(int index)
{
    if (index != 0)
        return 0;
    
    return &disk;
}