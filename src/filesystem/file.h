#ifndef FILE_H
#define FILE_H

#include "path_parser.h"

typedef unsigned int FILE_SEEK_MODE;

enum{
    SEEK,SET,
    SEEK_CUR,
    SEEK_END
};

typedef unsigned int FILE_MODE;
enum
{ 
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND,
    FILE_MODE_INVALID
};

struct disk;

//this is a function pointer, different file system will have their own implementation and this function pointer will point to it
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
//check to see if a filesystem is compatible 
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);

struct filesystem
{
    //two funciton pointers 
    
    //the filesystem should return 0 from resolve if the provided disk is using its fs
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;

    char name[20];
};

struct file_descriptor
{
    //the descriptor index
    int index;
    struct filesystem* filesystem;
    
    //private data for internal file descriptor
    void* private;

    // the disk that the file descriptor should be used on
    struct disk* disk;

};


void fs_init();
int fopen(const char* filename, const char* mode);
void fs_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);


#endif