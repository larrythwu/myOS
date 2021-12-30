#include "path_parser.h"
#include "config.h"
#include "std/string.h"
#include "memory/heap/kheap.h"
#include "status.h"
#include "memory/memory.h"
#include "std/stdio.h"

//determine if the file path is of valid format
static int path_valid_format(const char* filename)
{
    int len = strnlen(filename, MYOS_MAX_PATH);
    //path name is greater than 3 and the first digit must be a digit (the drive number) and the two character following the drvie number must be :/
     return (len >= 3 && isdigit(filename[0]) && memcmp((void*)&filename[1], ":/", 2) == 0);
}

//Note that we use the double pointer when we need to change the address of path
static int get_drive_by_path(const char** path)
{
    if(!path_valid_format(*path))
    {
        return -EBADPATH; 
    }

    //     0:/root -> 0
    int drive_no = tonumericdigit(*path[0]);

    //add 3 byte to skip the drive number 0:/
    *path += 3;

    return drive_no;
}

//instantiate a path_root object
static struct path_root* create_root(int drive_number)
{
    //allocate a heap space to hold the path_root
    struct path_root* path_r = kzalloc(sizeof(struct path_root));
    path_r->drive_no = drive_number;
    path_r->first=0;
    return path_r;
}

//get portion of the path name that is the path part 0:/path/... |-> path
static const char* get_path_part(const char** path)
{
    char* result_path_part = kzalloc(MYOS_MAX_PATH);
    int i = 0;
    while(**path != '/' && **path != 0x00)
    {
        result_path_part[i] = **path;
        *path += 1;
        i++;
    }
    
    //reached the end of one part, we jumpt over the / so that the next portion of the path can be read
    if(**path == '/')
    {
        (*path)++;
    }

    //if we are already at the end of the path, there is nothing to read
    if(i == 0)
    {
        kfree(result_path_part);
        result_path_part = 0;
    }

    return result_path_part;
}

//append the path part to the tail of the path_part linked list
struct path_part* parse_path_part(struct path_part* last_part, const char** path)
{
    const char* path_part_str = get_path_part(path);
    
    if(!path_part_str) return 0;

    struct path_part* part = kzalloc(sizeof(struct path_part));
    part->part = path_part_str;
    part->next = 0x00;

    if(last_part)
    {
        last_part->next = part;
    }

    return part;
}

//delete all the path related stuff
void pathparser_free(struct path_root* root)
{
    struct path_part* part = root->first;

    //delete the linked list
    while(part)
    {
        struct path_part* next_part = part->next;
        kfree((void*) part->part);
        kfree(part);
        part = next_part;
    }

    //free the root struct
    kfree(root);
}

//construct the root objects and the part objects (linked list head and the objects) from the path name, current_direcotry_path is reserved for now
struct path_root* pathparser_parse(const char* path, const char* currrent_directory_path)
{
    int res = 0;
    const char*  temp_path = path;
    struct path_root* path_root = 0;

    //check the length of the path is valid
    if(strlen(path) > MYOS_MAX_PATH)
    {
        print("Error in path parser");
        goto out;
    }

    //get the drive number and  temp_path += 3
    res = get_drive_by_path(&temp_path);
    if(res<0)
    {
        print("Error in path parser");
        goto out;
    }

    //create the root struct (linked list head object)
    path_root = create_root(res);
    if(!path_root)
    {
        print("Error in path parser");
        goto out;
    }

    //create the first part, the first object that follows the root (the first object in the linked list right after the head)
    struct path_part* first_part = parse_path_part(NULL, &temp_path);
    if(!first_part)
    {
        print("Error in path parser");
        goto out;
    }

    //append the object to be the next of the head
    path_root->first = first_part;
    struct path_part* part = parse_path_part(first_part, &temp_path);
   
    //append the rest of the parts to the linked list
    while(part)
    {
        part = parse_path_part(part, &temp_path);
    }

out:
    return path_root;

}