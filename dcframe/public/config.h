
#pragma once
// 定义操作系统
//#ifndef __OS_TYPE__
//#error "需要定义目标操作系统"
//#endif

#ifdef _MSC_VER
    #define _SW_DCF_LINUX         0 
    #define _SW_DCF_WINDOWS       1 
#else
    #define _SW_DCF_LINUX         1
    #define _SW_DCF_WINDOWS       0
#endif


