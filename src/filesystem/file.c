#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "std/stdio.h"
#include "memory/heap/kheap.h"
#include "fat/fat16.h"

struct filesystem* filesystems[MYOS_MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[MYOS_MAX_FILE_DESCRIPTORS];

//return the pointer to the first emptry slot in filesystems array
static struct filesystem** fs_get_free_filesystem()
{
    int i = 0;
    for (i = 0; i < MYOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] == 0)
        {
            return &filesystems[i];
        }
    }

    return 0;
}

//set a free entry in the filesystems array to the filesystem specified 
void fs_insert_filesystem(struct filesystem* filesystem)
{
    struct filesystem** fs;
    fs = fs_get_free_filesystem();

    //no empty spot in the filesystems array
    if (!fs)
    {
        //panic
        print("Problem inserting filesystem"); 
        while(1) {}
    }

    *fs = filesystem;
}

//load the static filesystems, the one that are preinstalled by the kernel without external drivers
//in this case just the fat16 that we have defined in fat/fat16.c
static void fs_static_load()
{
    //fat16_init will point the function pointers to the appropriate fat16 implementation
    fs_insert_filesystem(fat16_init());
}

//allocate the memory space for the filesystems array and load the 
//preinstalled kernel filesystems
void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

//allocate the space for file_descriptors 
//and call function to init the filesystems[]
void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

//set the first free entry of the file_descriptors[] to a new descriptor
//and return the descriptor back through the double pointer 
static int file_new_descriptor(struct file_descriptor** desc_out)
{
    int res = -ENOMEM;
    for (int i = 0; i < MYOS_MAX_FILE_DESCRIPTORS; i++)
    {
        if (file_descriptors[i] == 0)
        {
            struct file_descriptor* desc = kzalloc(sizeof(struct file_descriptor));
            // Descriptors start at 1
            desc->index = i + 1;
            file_descriptors[i] = desc;
            *desc_out = desc;
            res = 0;
            break;
        }
    }

    return res;
}

//get the file_descriptor with the fd index
static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd >= MYOS_MAX_FILE_DESCRIPTORS)
    {
        return 0;
    }

    // Descriptors start at 1
    int index = fd - 1;
    return file_descriptors[index];
}


//loop though the all the filesystems loaded and determine which one resovles with the disk provided
struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = 0;
    for (int i = 0; i < MYOS_MAX_FILESYSTEMS; i++)
    {
        if (filesystems[i] != 0 && filesystems[i]->resolve(disk) == 0)
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

int fopen(const char* filename, const char* mode)
{
    return -EIO;
} 