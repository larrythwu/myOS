#include "memory.h"

//our own memset function
//since we are in protected mode we can do what ever we want to any memory address
//as protection is not enforced yet, that is why we can set memory directorly instead of implemnting 
//somehting like malloc which we need in user space
void* memset(void* ptr, int c, size_t size)
{
    char* c_ptr = (char*) ptr;
    for(int i = 0; i< size; i++)
    {
        c_ptr[i] = (char) c;

    }
    return ptr;
}
