#ifndef DISK_H
#define DISK_H


//we want to create a structure that can allow us to generalize our disk IO code 
//for all the disk type our system can have
typedef unsigned int MYOS_DISK_TYPE;

//represent a real physical hard drive
#define MYOS_DISK_TYPE_REAL 0

struct disk
{
    MYOS_DISK_TYPE type;
    int sector_size;
};

void disk_search_and_init();

int disk_read_block(struct disk* idisk, unsigned int lba, int total, void* buf);

struct disk* disk_get(int index);

#endif