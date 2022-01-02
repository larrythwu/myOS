#include "fat16.h"
#include "status.h"
#include "std/string.h"

//this is a function pointer, different file system will have their own implementation and this function pointer will point to it
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
//check to see if a filesystem is compatible 
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

//instantiate filesyste struct and set the resolve function pointer
struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open
};

struct filesystem* fat16_init()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

int fat16_resolve(struct disk* disk)
{
    return 0;
}

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    return 0;
}