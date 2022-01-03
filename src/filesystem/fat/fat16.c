#include "fat16.h"
#include "status.h"
#include "std/string.h"
#include <stdint.h>
#include "disk/disk.h"
#include "disk/streamer.h"
#include  "memory/heap/kheap.h"
#include  "memory/memory.h"
#include "fat_structures.h"

int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

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

//initialize the three streamer that allow us to read from the FAT hierarchy
static void fat16_init_private(struct disk* disk, struct fat_private* private)
{
    memset(private, 0, sizeof(struct fat_private));
    private->cluster_read_stream = diskstreamer_new(disk->id);
    private->fat_read_stream = diskstreamer_new(disk->id);
    private->directory_stream =  diskstreamer_new(disk->id);
    //disk->fs_private = private;
}

//sector number to the absolute position in bytes
int fat16_sector_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}


//return the total number of dir items inside the dir specified by the directory_start_sector
int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat_directory_item item;
    struct fat_directory_item empty_item;
    memset(&empty_item, 0, sizeof(empty_item));

    struct fat_private* fat_private = disk->fs_private;

    int res = 0;
    int i = 0;
    int directory_start_pos = directory_start_sector * disk->sector_size;
    struct disk_stream* stream = fat_private->directory_stream;
    if(diskstreamer_seek(stream, directory_start_pos) != ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    while(1)
    {
        if (diskstreamer_read(stream, &item, sizeof(item)) != ALL_OK)
        {
            res = -EIO;
            goto out;
        }

        if (item.filename[0] == 0x00)
        {
            // We are done
            break;
        }

        // Is the item unused
        if (item.filename[0] == 0xE5)
        {
            continue;
        }

        i++;
    }

    res = i;

out:
    return res;
}

//retrieve info about the root dir by reading the FAT header and load the too diretory items in to 'directory'
int fat16_get_root_directory(struct disk* disk, struct fat_private* fat_private, struct fat_directory* directory)
{
    int res = 0;
    struct fat_header* primary_header = &fat_private->header.primary_header;
    //we calculate the position of the root dir entry on the disk, basically this is just saying that the root comes after the two FAT table and the reserved areas 
    int root_dir_sector_pos = (primary_header->fat_copies * primary_header->sectors_per_fat)+primary_header->reserved_sectors;
    //get how many entries there are in the root dir
    int root_dir_entries = fat_private->header.primary_header.root_dir_entries;
    //get the total size of our struct to store the info of the root dir
    int root_dir_size = (root_dir_entries * sizeof(struct fat_directory_item));
    int total_sectors = root_dir_size / disk->sector_size;
    if(root_dir_size % disk->sector_size)
    {
        total_sectors++;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    //we read the entire root dir table which contain bunch of far fat_directory_items that is why 
    //dir is of type fat_directory_item*, an array of fat_directory_item
    struct fat_directory_item* dir = kzalloc(root_dir_size);
   
    if(!dir)
    {
        res = -ENOMEM;
        goto out;
    }

    struct disk_stream* stream = fat_private->directory_stream;
    if (diskstreamer_seek(stream, fat16_sector_to_absolute(disk, root_dir_sector_pos)) != ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    if(diskstreamer_read(stream, dir, root_dir_size) != ALL_OK)
    {
        res = -EIO;
        goto out;
    }

    directory->item=dir;
    directory->total = total_items;
    directory->sector_pos = root_dir_sector_pos;
    directory->ending_sector_pos = root_dir_sector_pos + (root_dir_size / disk->sector_size);

out:
    return res;
}

int fat16_resolve(struct disk* disk)
{
    //we need to read the first sector of the disk provided and check
    //if the fs is indeed FAT16
    int res = 0;

    //FIXME, if the resolve is not sucessfull wouldn't this heap allocation be leaked?
    struct fat_private* fat_private = kzalloc(sizeof(struct fat_private));
    fat16_init_private(disk, fat_private);
    disk->fs_private = fat_private;
    disk->filesystem = &fat16_fs;

    struct disk_stream* stream = diskstreamer_new(disk->id);
    if(!stream)
    {
        res = -ENOMEM;
        goto out;
    }

    //we read the FAT header block from the disk
    if(diskstreamer_read(stream, &(fat_private->header), sizeof(fat_private->header)) != ALL_OK)
    {
        //if the read is not sucessful then there is an IO error
        res = -EIO;
        goto out;
    }

    //we check the header that we just read, if the signature doesnot match that of the FAT16 system then we return NOTUS error
    if(fat_private->header.shared.extended_header.signature != 0x29)
    {
        res = -EFSNOTUS;
        goto out;
    }

    if(fat16_get_root_directory(disk, fat_private, &fat_private->root_directory) != ALL_OK)
    {
        //if the read is not sucessful then there is an IO error
        res = -EIO;
        goto out;
    }

    //now the filesystem has been resolved, and the disk is indeed of the FAT16 format we now set the relevant info in disk
    // disk->fs_private = fat_private;
    // disk->filesystem = &fat16_fs;

out:
    if(stream)
    {
        diskstreamer_close(stream);
    }
    if(res<0)
    {
        //if the resolve failed, meaning that the disk is not formatted in FAT16, we need to revert our changes 
        kfree(fat_private);
        disk->fs_private = 0;
        disk->filesystem = 0;
    }
    return res;
}

void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    return 0;
}