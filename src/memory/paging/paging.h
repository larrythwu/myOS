#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGING_CACHE_DISABLED   0b00010000 //the fifth bit is cache disable bit
#define PAGING_WRITE_THROUGH    0b00001000
#define PAGING_ACCESS_FROM_ALL  0b00000100 //if this bit is set, the page can be use by all previledge mode
#define PAGING_IS_WRITEABLE     0b00000010
#define PAGING_IS_PRESENT       0b00000001

#define PAGING_TOTAL_ENTRIES_PER_TABLE 1024
#define PAGING_PAGE_SIZE 4096

struct paging_4gb_chunk
{
    uint32_t* directory_entry;

};

void paging_switch(uint32_t* directory);
uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk* chunk);
struct paging_4gb_chunk* paging_new_4gb(uint8_t flags);

//we donnot enable paging until we created a page table and switced to it
void enable_paging();
void paging_load_directory(uint32_t* directory);

int paging_set(uint32_t* directory, void* virtual_addr, uint32_t val);
int paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out);
bool paging_is_aligned(void* address);
void paging_free_4gb(struct paging_4gb_chunk* chunk);
int paging_map_range(uint32_t* directory, void* virt, void* phys, int count, int flags);
int paging_map(uint32_t* directory, void* virt, void* phys, int flags);
int paging_map_to(uint32_t *directory, void *virt, void *phys, void *phys_end, int flags);
void* paging_align_address(void* ptr);

#endif