#include "file.h"
#include "config.h"
#include "memory/memory.h"
#include "status.h"
#include "std/stdio.h"
#include "memory/heap/kheap.h"
#include "fat/fat16.h"
#include "disk/disk.h"
#include "std/string.h"
#include "kernel.h"

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

FILE_MODE file_get_mode_by_string(const char* str)
{
    FILE_MODE mode = FILE_MODE_INVALID;
    if(strncmp(str, "r", 1) == 0)
    {
        mode = FILE_MODE_READ;
    }
    else if(strncmp(str, "w", 1) == 0)
    {
        mode = FILE_MODE_WRITE;
    }
    else if(strncmp(str, "a", 1) == 0)
    {
        mode = FILE_MODE_APPEND;
    }
    return mode;
}

//VFS file open this will make call to the approapriate fs implmentation
//return the index of the handle descriptor
int fopen(const char* filename, const char* mode_str)
{
    int res = 0;
    //path root points to the first element of the path part linked list
    struct path_root* root_path = pathparser_parse(filename, NULL);
    if(!root_path)
    {
        res = -EINVARG;
        goto out;
    }

    //make sure the path acutally contain a filename
    //and not just a root path "/"
    if(!root_path->first)
    {
        res = -EINVARG;
        goto out;
    }

    struct disk* disk = disk_get(root_path->drive_no);
    if(!disk)
    {
        res = -EIO;
        goto out;
    }

    //check if there if is a filesystem binded with the disk
    if(!disk->filesystem)
    {
        res = -EIO;
        goto out;
    }

    FILE_MODE mode = file_get_mode_by_string(mode_str);
    if(mode == FILE_MODE_INVALID)
    {
        res = -EINVARG;
        goto out;
    }

    //call the lower level implementation of fopen in the acutal file system
    void* descriptor_private_data = disk->filesystem->open(disk, root_path->first, mode);

    if(ISERR(descriptor_private_data))
    {
        res = ERROR_I(descriptor_private_data);
        goto out;
    }

    struct file_descriptor* desc = 0;
    res = file_new_descriptor(&desc);
    if(res < 0)
    {
        goto out;
    }

    desc->filesystem = disk->filesystem;
    desc->private = descriptor_private_data;
    desc->disk = disk;
    res = desc->index;

out:
    //fopen should not return < 0 , index 0 means error
    if(res<0)
        res = 0;

    return res;
} 


//ptr the return address of the read data
//size: what is the block size that we want to read; nmemb: how many of these blocks we want to read
//the file descriptor for the opened file
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd)
{
    int res = 0;
    if (size == 0 || nmemb == 0 || fd < 1)
    {
        res = -EINVARG;
        goto out;
    }

    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EINVARG;
        goto out;
    }

    res = desc->filesystem->read(desc->disk, desc->private, size, nmemb, (char*) ptr);
out:
    return res;
} 

//the virtual file system function for moving the file descriptor position
int fseek(int fd, int offset, FILE_SEEK_MODE whence)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->seek(desc->private, offset, whence);
out:
    return res;
}

//VFS function for getting the stat of a file
int fstat(int fd, struct file_stat* stat)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->stat(desc->disk, desc->private, stat);
out:
    return res;
}

//zero out the entry in the desciptor table when we close the descriptor
static void file_free_descriptor(struct file_descriptor* desc)
{
    file_descriptors[desc->index-1] = 0x00;
    kfree(desc);
}

int fclose(int fd)
{
    int res = 0;
    struct file_descriptor* desc = file_get_descriptor(fd);
    if (!desc)
    {
        res = -EIO;
        goto out;
    }

    res = desc->filesystem->close(desc->private);
    if(res == ALL_OK)
    {
        file_free_descriptor(desc);
    }
out:
    return res;
}

