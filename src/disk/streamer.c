#include "streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"
#include "disk.h"

struct disk_stream* diskstreamer_new(int disk_id)
{
    struct disk* disk = disk_get(disk_id);
    if(!disk)
    {
        return 0;
    }

    struct disk_stream* streamer = kzalloc(sizeof(struct disk_stream));
    streamer -> pos = 0;
    streamer -> disk = disk;
    return streamer;
}

int diskstream_seek(struct disk_stream* stream, int pos)
{
    stream->pos = pos;
    return 0;
}

// reading custom amount from the disk, pior to this function we can only read at 1 sector granduality 
//now this function still read at sector by secotr basis, but return the custom amount
int diskstreamer_read(struct disk_stream* stream, void* out, int total)
{
    int sector = stream->pos / MYOS_SECTOR_SIZE;
    int offset = stream->pos % MYOS_SECTOR_SIZE;
    char buf[MYOS_SECTOR_SIZE];

    int res = disk_read_block(stream->disk, sector, 1, buf);
    if(res<0)
    {
        goto out;
    }

    int total_to_read = total > MYOS_SECTOR_SIZE? MYOS_SECTOR_SIZE : total;
    for(int i = 0; i<total_to_read; i++)
    {
        *(char*)out++ = buf[offset++];
    }

    stream->pos += total_to_read;
    if(total > MYOS_SECTOR_SIZE)
    {
        res = diskstreamer_read(stream, out, total - MYOS_SECTOR_SIZE);
    }
out:
    return res;
}

void diskstreamer_close(struct disk_stream* stream)
{
    kfree(stream);
}