#include "io/io.h"
#include "disk.h"

//lba: the logical block number of the sector on the hardrive that we want to read
//total: how many sector to read
//buf: pointer to the memory location to store the read data
int disk_read_sector(int lba, int total, void* buf)
{
    outb(0x1F6, (lba>>24) | 0xE0);
    outb(0x1F2, total);
    outb(0x1F3, (unsigned char)(lba & 0xff));
    outb(0x1F4, (unsigned char)(lba >> 8));
    outb(0x1F5, (unsigned char)(lba >> 16));
    outb(0x1F7, 0x20);

    //we frame ptr to be a short because short is two byte
    unsigned short* ptr = (unsigned short*) buf;
    
    //we need to do this for every byte that we want to read
    for(int b = 0; b < total; b++)
    {
        //wait for the data return buffer to be ready
        char c = insb(0x1F7);
        while(! (c & 0x88))
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