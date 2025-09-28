//
// Created by zhw on 2017/7/27.
//

#ifndef ZHONGHAIWAN_ZHW_TOOLS_H
#define ZHONGHAIWAN_ZHW_TOOLS_H
#include <random>
#endif //ZHONGHAIWAN_ZHW_TOOLS_H

class zhw_tools
{
public:
    static bool isUTF8(const void* pBuffer, long size);
    static unsigned int rand(unsigned int min, unsigned int max);
};