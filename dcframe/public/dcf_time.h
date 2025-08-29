#ifndef Dcf_time_h
#define Dcf_time_h

/*
文件说明:本文件包含与时间操作相关的API和宏定义，便于跨平台使用
文件作者:zjb
创建日期:2017.5.4
*/

#include <time.h>
#include "dcf_def.h"


#if _SW_DCF_MACOS
#define  clock_t   struct timespec
extern void dcf_time_sleep(clock_t ms);
#endif


// 得到当前的时钟tick
extern clock_t dcf_time_get_cur_clock();


// 得到两个时钟点的毫秒间隔
extern DWORD dcf_time_get_ms_interval(clock_t start,clock_t cur);
// 该函数有2个用途:
// 1.定时产生工作点(从pre_work到当前如果满足workms时长，则返回true，否则返回false)
// 2.定时休息(从pre_sleep到当前可以休息sleepms，每次固定会休息完这个时间)
// 从pre_work 时间开始工作到当前时间总共对应ms，剩余的用于休息
// 返回true 表示可以开始下一份工作,返回false表示还未到下一次处理的时间
extern bool dcf_time_exact_work_and_sleep(clock_t &pre_work,clock_t &pre_sleep,DWORD workms,DWORD sleepms);
extern void dcf_time_sleep(DWORD ms);

#endif
