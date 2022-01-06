#include "fat16.h"
#include "status.h"
#include "std/string.h"
#include <stdint.h>
#include "disk/disk.h"
#include "disk/streamer.h"
#include  "memory/heap/kheap.h"
#include  "memory/memory.h"
#include "fat_structures.h"
#include "kernel.h"
#include "status.h"
#include "config.h"


//instantiate filesyste struct and set the resolve function pointer
struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat
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

//parse through a string (in) and produce a copy (out) by adding a null terminator at the position of the space if there is a space
//file name is eight char by default anything shorter than that are padded with space, so we really need this
void fat16_to_proper_string(char** out, const char* in)
{
    //FIXME: there is a bug when the name of the file is full 8 byte, there would not be a null terminator or a space
    //simply add a counter to count to eight
    int counter = 8;
    //stop when there is a space or null terminator
    while(*in != 0x00 && *in != 0x20 && counter>0)
    {
        **out = *in;
        *out += 1;
        in +=1;
        counter--;
    }

    if (*in == 0x20)
    {
        **out = 0x00;
    }
}

//take in the raw filename in out and change it to the space eliminated + extension appended version
//ie 'hello   '   ->> "hello.pdf"
void fat16_get_full_relative_filename(struct fat_directory_item* item, char* out, int max_len)
{
    memset(out, 0x00, max_len);
    char *out_tmp = out;
    fat16_to_proper_string(&out_tmp, (const char*) item->filename);
    if (item->ext[0] != 0x00 && item->ext[0] != 0x20)
    {
        *out_tmp++ = '.';
        //join the filename and the extension ie hello (ext pdf) -> hello.pdf
        fat16_to_proper_string(&out_tmp, (const char*) item->ext);
    }
}

//clone a fat_dir_item into a new memory space
struct fat_directory_item* fat16_clone_directory_item(struct fat_directory_item* item, int size)
{
    struct fat_directory_item* item_copy = 0;
    if (size < sizeof(struct fat_directory_item))
    {
        return 0;
    }

    item_copy = kzalloc(size);
    if (!item_copy)
    {
        return 0;
    }

    memcpy(item_copy, item, size);
    return item_copy;
}

//join the address together 
static uint32_t fat16_get_first_cluster(struct fat_directory_item* item)
{
    return (item->high_16_bits_first_cluster)<<16 | item->low_16_bits_first_cluster;
}

//convert cluster number to the actual sector number 
static int fat16_cluster_to_sector(struct fat_private* private, int cluster)
{
    //the cluster data start after the root dir stuff 
    return private->root_directory.ending_sector_pos + ((cluster - 2) * private->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat_private* private)
{
    /*
                 Disk
    |----------------------------|

    Reserved secotors (our kernel)

    |----------------------------|

    Our fat entry table begins

    |----------------------------|
    */
    return private->header.primary_header.reserved_sectors;
}

//get the fat entry from the fat entry table given the cluster number
static int fat16_get_fat_entry(struct disk* disk, int cluster)
{
    int res = -1;
    struct fat_private* private = disk->fs_private;
    struct disk_stream* stream = private->fat_read_stream;
    if (!stream)
    {
        goto out;
    }

    //this get the secotr position of the entry table 
    uint32_t fat_table_position = fat16_get_first_fat_sector(private) * disk->sector_size;
    //to read the entire fat entry table, we first move the descriptor position to the entry of our cluster
    //remember the fat table is a lienar mapping between the table entry index and the cluster number
    res = diskstreamer_seek(stream, fat_table_position * (cluster * MYOS_FAT16_FAT_ENTRY_SIZE));
    if (res < 0)
    {
        goto out;
    }

    //we read exactly one fat entry 2 bytes, the first entry of the cluster we want
    uint16_t result = 0;
    res = diskstreamer_read(stream, &result, sizeof(result));
    if (res < 0)
    {
        goto out;
    }

    res = result;
out:
    return res;
}


//givne the starting_cluster number, and the offset, we return the cluster number that we are really reading from
//in the case that offset exeed the starting cluster's size, we need to go to the next clusters
static int fat16_get_cluster_for_offset(struct disk* disk, int starting_cluster, int offset)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = starting_cluster;
    //since the cluster are not spaced concurrenrly in the memory, we need to know how many cluster ahead we want to read
    //kinda like parsing through a linked list, when we only have the head*
    int clusters_ahead = offset / size_of_cluster_bytes;
    for (int i = 0; i < clusters_ahead; i++)
    {
        //get the entry from the table
        int entry = fat16_get_fat_entry(disk, cluster_to_use);
        //we are at the last cluster
        if (entry == 0xFF8 || entry == 0xFFF)
        {
            res = -EIO;
            goto out;
        } 

        // Sector is marked as bad?
        if (entry == MYOS_FAT16_BAD_SECTOR)
        {
            res = -EIO;
            goto out;
        }

        // Reserved sector?
        if (entry == 0xFF0 || entry == 0xFF6)
        {
            res = -EIO;
            goto out;
        }

        //zer0 means there is no data in this entry/cluster
        if (entry == 0x00)
        {
            res = -EIO;
            goto out;
        }
        //we entered this loop in the first place because the cluster we want to read is not the first one
        //tne entry stores the fat table entry index of the next cluster 
        cluster_to_use = entry;
    }

    res = cluster_to_use;
out:
    return res;
}


//read from the fat data clusters, used to load the direcotory items or data,
//note that if total exceed the cluster size, we will read automatically from the next cluster
static int fat16_read_internal_from_stream(struct disk* disk, struct disk_stream* stream, int cluster, int offset, int total, void* out)
{
    int res = 0;
    struct fat_private* private = disk->fs_private;
    //byte per cluster
    int size_of_cluster_bytes = private->header.primary_header.sectors_per_cluster * disk->sector_size;
    
    int cluster_to_use = fat16_get_cluster_for_offset(disk, cluster, offset);
    if (cluster_to_use < 0)
    {
        res = cluster_to_use;
        goto out;
    }


    int offset_from_cluster = offset % size_of_cluster_bytes;

    int starting_sector = fat16_cluster_to_sector(private, cluster_to_use);
    int starting_pos = (starting_sector * disk->sector_size) + offset_from_cluster;
    //we cannot read more than 1 cluster at a time
    int total_to_read = total > size_of_cluster_bytes ? size_of_cluster_bytes : total;
    res = diskstreamer_seek(stream, starting_pos);
    if (res != ALL_OK)
    {
        goto out;
    }

    res = diskstreamer_read(stream, out, total_to_read);
    if (res != ALL_OK)
    {
        goto out;
    }

    total -= total_to_read;
    if (total > 0)
    {
        // We still have more to read, we call recursivly and remember to increment the offset nad the out*
        res = fat16_read_internal_from_stream(disk, stream, cluster, offset+total_to_read, total, out + total_to_read);
    }

out:
    return res;
}

//read from the fat data clusters, used to load the direcotory items or data,
//note that if total exceed the cluster size, we will read automatically from the next cluster
//starting_cluster: the first cluster form which we want to read
//offset: the offset within the first cluster which we want to read
//total: how many bytes to read
//out: where we store the read data
static int fat16_read_internal(struct disk* disk, int starting_cluster, int offset, int total, void* out)
{
    struct fat_private* fs_private = disk->fs_private;
    struct disk_stream* stream = fs_private->cluster_read_stream;
    return fat16_read_internal_from_stream(disk, stream, starting_cluster, offset, total, out);
}


//given that the fat_directory_item* item is a directory, we return a fat_directory struct that hold all the fat_dir_items inside the directory represented in 'item'
struct fat_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_item* item)
{
    int res = 0;
    struct fat_directory* directory = 0;
    struct fat_private* fat_private = disk->fs_private;

    //double check if the dir item really is a dir
    if (!(item->attribute & FAT_FILE_SUBDIRECTORY))
    {
        res = -EINVARG;
        goto out;
    }

    directory = kzalloc(sizeof(struct fat_directory));
    if (!directory)
    {
        res = -ENOMEM;
        goto out;
    }

    //get the first sector and then convert it to sector number 
    int cluster = fat16_get_first_cluster(item);
    int cluster_sector = fat16_cluster_to_sector(fat_private, cluster);
    //get the total number of dir_items in this directory
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    directory->total = total_items;
    //get the size of the directory struct we need to store all the dir items
    int directory_size = directory->total * sizeof(struct fat_directory_item);
    directory->item = kzalloc(directory_size);
    if (!directory->item)
    {
        res = -ENOMEM;
        goto out;
    }

    res = fat16_read_internal(disk, cluster, 0x00, directory_size, directory->item);
    if (res != ALL_OK)
    {
        goto out;
    }


out:
    if (res != ALL_OK)
    {
        kfree(directory->item);
    }
    return directory;
}

struct fat_item* fat16_new_fat_item_for_directory_item(struct disk* disk, struct fat_directory_item* item)
{
    struct fat_item* f_item = kzalloc(sizeof(struct fat_item));
    if (!f_item)
    {
        return 0;
    }
    //if the item is a sub dir, then we load its fat_direcotry struct
    if (item->attribute & FAT_FILE_SUBDIRECTORY)
    {
        f_item->directory = fat16_load_fat_directory(disk, item);
        f_item->type = FAT_ITEM_TYPE_DIRECTORY;
        return f_item;
    }

    //if its a file, we clone its item struct as to be safe, in case the original item get changed in the future
    f_item->type = FAT_ITEM_TYPE_FILE;
    f_item->item = fat16_clone_directory_item(item, sizeof(struct fat_directory_item));
    return f_item;
}

//search for the fat_item by name and return it if it exist in the directory given
struct fat_item* fat16_find_item_in_directory(struct disk* disk, struct fat_directory* directory, const char* name)
{
    struct fat_item* f_item = 0;
    char tmp_filename[MYOS_MAX_PATH];
    for (int i = 0; i < directory->total; i++)
    {
        //get the name of the item in the directory one by one and compare it with the name provided
        //not this filename is extension appended, ie hello.pdf instead of hello as in the plain directory item name
        fat16_get_full_relative_filename(&directory->item[i], tmp_filename, sizeof(tmp_filename));
        //note that we are using the case insensitive string compare, because FAT is case insensitive
        if(istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0)
        {
            // if found, let's create a new fat_item
            f_item = fat16_new_fat_item_for_directory_item(disk, &directory->item[i]);
        }
    }

    return f_item;
}


void fat16_free_directory(struct fat_directory* directory)
{
    if (!directory)
    {
        return;
    }

    if (directory->item)
    {
        kfree(directory->item);
    }

    kfree(directory);
}


void fat16_fat_item_free(struct fat_item* item)
{
    if (item->type == FAT_ITEM_TYPE_DIRECTORY)
    {
        fat16_free_directory(item->directory);
    }
    else if(item->type == FAT_ITEM_TYPE_FILE)
    {
        kfree(item->item);
    }

    kfree(item);
}



//return the fat_item of the target file to open, given the path in path_part* and disk
struct fat_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    struct fat_private* fat_private = disk->fs_private;
    struct fat_item* current_item = 0;
    //get the fat_item of the first part after root ie.   0:/first_part/second_part/....
    struct fat_item* root_item = fat16_find_item_in_directory(disk, &fat_private->root_directory, path->part);
    if(!root_item)
    {
        //direcotry or file does not exist
        goto out;
    }


  struct path_part* next_part = path->next;
    current_item = root_item;
    while(next_part != 0)
    {
        //all the iterations we should see dir, except the last iteration where the file is finally found
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY)
        {
            current_item = 0;
            break;
        }

        struct fat_item* tmp_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->part);
        fat16_fat_item_free(current_item);
        current_item = tmp_item;
        next_part = next_part->next;
    }
out:
    return current_item;

}
 
//given the path in linked list form and the disk, we return the file descritor 
//which contains the pos = 0 (begining of file) and the fat_item of the file 
//the cluster number is stored in the fat item which we can use then to access the data
//here we are simply retrieving this fat item 
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode)
{
    if(mode != FILE_MODE_READ)
    {
        return ERROR(-ERDONLY);
    }

    struct fat_file_descriptor* descriptor = 0;
    descriptor = kzalloc(sizeof(struct fat_file_descriptor));
    if(!descriptor)
    {
        kfree(descriptor);
        return ERROR(-ENOMEM);
    }

    descriptor->item = fat16_get_directory_entry(disk, path);
    if(!descriptor->item)
    {
        kfree(descriptor);
        return ERROR(-EIO);
    }

    //when the file is frist opened the position is zero of course
    descriptor->pos=0;


    return descriptor;
}

//after a file is opened, ie if we have the file descriptor, then we can read it 
//by specifying the descriptor, the block size we want to read and how many of these blocks to read
//the data will be read into the addr pointed to by out 
int fat16_read(struct disk* disk, void* descriptor, uint32_t size, uint32_t nmemb, char* out_ptr)
{
    int res = 0;
    struct fat_file_descriptor* fat_desc = descriptor;
    struct fat_directory_item* item = fat_desc->item->item;
    int offset = fat_desc->pos;
    //the first cluster where the data of the file are stored 
    uint32_t first_cluster_number =  fat16_get_first_cluster(item);
    for (uint32_t i = 0; i < nmemb; i++)
    {
        res = fat16_read_internal(disk, first_cluster_number, offset, size, out_ptr);
        if (res<0)
        {
            goto out;
        }

        out_ptr += size;
        offset += size;
    }

    res = nmemb;
out:
    return res;
} 

//change the position of the file descriptor
int fat16_seek(void *private, uint32_t offset, FILE_SEEK_MODE seek_mode)
{
    int res = 0;
    struct fat_file_descriptor *desc = private;
    struct fat_item *desc_item = desc->item;

    //make sure that the descriptor is indeed poiting to file and not a dir
    //cannot be a dir anyway really, since we do the checks in open
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item *ritem = desc_item->item;

    //check if we are seeking to a position outside of the file
    if (offset >= ritem->filesize)
    {
        res = -EIO;
        goto out;
    }

    switch (seek_mode)
    {
    case SEEK_SET:
        desc->pos = offset;
        break;
    //not implemented yet
    case SEEK_END:
        res = -EUNIMP;
        break;

    case SEEK_CUR:
        desc->pos += offset;
        break;

    default:
        res = -EINVARG;
        break;
    }
out:
    return res;
} 

//get the file size and the attr flags of the file from the descriptor
int fat16_stat(struct disk* disk, void* private, struct file_stat* stat)
{
    int res = 0;
    struct fat_file_descriptor* descriptor = (struct fat_file_descriptor*) private;
    struct fat_item* desc_item = descriptor->item;
    if (desc_item->type != FAT_ITEM_TYPE_FILE)
    {
        res = -EINVARG;
        goto out;
    }

    struct fat_directory_item* ritem = desc_item->item;
    stat->filesize = ritem->filesize;
    stat->flags = 0x00;

    //check if the read only bit in the attribute is set
    if (ritem->attribute & FAT_FILE_READ_ONLY)
    {
        stat->flags |= FILE_STAT_READ_ONLY;
    }
    //we didn't bother with other flags like write, because we didn't implement them anyway

out:
    return res;
}

static void fat16_free_file_descriptor(struct fat_file_descriptor* desc)
{
    fat16_fat_item_free(desc->item);
    kfree(desc);
}


int fat16_close(void* private)
{
    fat16_free_file_descriptor((struct fat_file_descriptor*) private);
    return 0;
}