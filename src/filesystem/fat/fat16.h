#ifndef FAT16_H
#define FAT16_H

#include "filesystem/file.h"
struct filesystem* fat16_init();
int fat16_resolve(struct disk* disk);
void* fat16_open(struct disk* disk, struct path_part* path, FILE_MODE mode);

#endif