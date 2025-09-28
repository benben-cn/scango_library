//
// Created by zhw on 2017/7/27.
//

#include "zhw_tools.h"

bool zhw_tools::isUTF8(const void* pBuffer, long size)
{
    bool IsUTF8 = true;
    unsigned char* start = (unsigned char*)pBuffer;
    unsigned char* end = (unsigned char*)pBuffer + size;
    while (start < end)
    {
        if (*start < 0x80) // (10000000): value less then 0x80 ASCII char
        {
            start++;
        }
        else if (*start < (0xC0)) // (11000000): between 0x80 and 0xC0 UTF-8 char
        {
            IsUTF8 = false;
            break;
        }
        else if (*start < (0xE0)) // (11100000): 2 bytes UTF-8 char
        {
            if (start >= end - 1)
                break;
            if ((start[1] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }
            start += 2;
        }
        else if (*start < (0xF0)) // (11110000): 3 bytes UTF-8 char
        {
            if (start >= end - 2)
                break;
            if ((start[1] & (0xC0)) != 0x80 || (start[2] & (0xC0)) != 0x80)
            {
                IsUTF8 = false;
                break;
            }
            start += 3;
        }
        else
        {
            IsUTF8 = false;
            break;
        }
    }
    return IsUTF8;
}

unsigned int zhw_tools::rand(unsigned int min, unsigned int max) {
    static std::default_random_engine e;
    static std::uniform_int_distribution<unsigned> u(min, max);
    return u(e);
}