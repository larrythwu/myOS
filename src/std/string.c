#include "string.h"

//return the length of a string
int strlen(const char* ptr)
{
    int i = 0;
    while(*ptr != 0)
    {
        i++;
        ptr += 1;
    }

    return i;
}

//strlen with a upper limit
int strnlen(const char* ptr, int max)
{
    int i = 0;
    for (i = 0; i < max; i++)
    {
        if (ptr[i] == 0)
            break;
    }

    return i;
}

bool isdigit(char c)
{
    //range of the ascii code that describes numbers
    return (c>=48 && c<= 57);
}

//char to digit
int tonumericdigit(char c)
{
    return c - 48;
}

char* strcpy(char* dest, const char* src)
{
    char* temp = dest;
    while(*src != 0)
    {
        *dest = *src;
        src += 1;
        dest += 1;
    }

    //add the null terminator
    *dest = 0x00;

    return temp;
}