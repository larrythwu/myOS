#ifndef STREAMER_H
#define STREAMER_H
#include "disk.h"

//struc that stores which disk we want to read
//and which byte 
struct disk_stream
{
    int pos;
    struct disk* disk;
};

struct disk_stream* diskstreamer_new(int disk_id);
int diskstream_seek(struct disk_stream* stream, int pos);
int diskstreamer_read(struct disk_stream* stream, void* out, int total);
void diskstreamer_close(struct disk_stream* stream);




#endif