#ifndef PATH_PARSER_H
#define PATH_PARSER_H

struct path_part
{
    const char* part;
    //kinda like a linked list
    struct path_part* next;
};

struct path_root
{
    //drive number
    int drive_no;
    struct path_part* first;
};



// 0:/test.txt | 0:/ drive number 0 and root, test.txt is the part and next = null
struct path_root* pathparser_parse(const char* path, const char* currrent_directory_path);
void pathparser_free(struct path_root* root);

#endif