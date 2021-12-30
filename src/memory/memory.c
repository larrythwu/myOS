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

int memcmp(void* s1, void* s2, int count)
{
    char* c1 = s1;
    char* c2 = s2;
    while(count-- > 0)
    {
        if (*c1++ != *c2++)
        {
            return c1[-1] < c2[-1] ? -1 : 1;
        }
    }

    return 0;
} 
