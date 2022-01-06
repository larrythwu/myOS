#ifndef FILE_H
#define FILE_H
#include <stdint.h>

#include "path_parser.h"

typedef unsigned int FILE_SEEK_MODE;
enum{
    SEEK_SET,
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

typedef unsigned int FILE_STAT_FLAGS;
enum
{
    FILE_STAT_READ_ONLY = 0b00000001
};

//used to store the file stats
struct file_stat
{
    FILE_STAT_FLAGS flags;
    uint32_t filesize;
};


struct disk;

//this is a function pointer, different file system will have their own implementation and this function pointer will point to it
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_part* path, FILE_MODE mode);
//check to see if a filesystem is compatible 
typedef int (*FS_RESOLVE_FUNCTION)(struct disk* disk);
//function pointers for read
typedef int (*FS_READ_FUNCTION)(struct disk* disk, void* private, uint32_t size, uint32_t nmemb, char* out);
//function poitner for seek
typedef int (*FS_SEEK_FUNCTION)(void* private, uint32_t offset, FILE_SEEK_MODE seek_mode);
//function pointer to stat
typedef int (*FS_STAT_FUNCTION)(struct disk* disk, void* private, struct file_stat* stat);
typedef int (*FS_CLOSE_FUNCTION)(void* private);

struct filesystem
{
    //two funciton pointers 
    
    //the filesystem should return 0 from resolve if the provided disk is using its fs
    FS_RESOLVE_FUNCTION resolve;
    FS_OPEN_FUNCTION open;
    FS_READ_FUNCTION read;
    FS_SEEK_FUNCTION seek;
    FS_STAT_FUNCTION stat;
    FS_CLOSE_FUNCTION close;

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


//the Virtual File System function prototypes, note these are not the function pointer implementations
void fs_init();
int fopen(const char* filename, const char* mode_str);
void fs_insert_filesystem(struct filesystem* filesystem);
struct filesystem* fs_resolve(struct disk* disk);
int fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);
int fseek(int fd, int offset, FILE_SEEK_MODE whence);
int fstat(int fd, struct file_stat* stat);
int fclose(int fd);

#endif