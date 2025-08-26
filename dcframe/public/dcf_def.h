#ifndef _DCF_DEF_H
#define _DCF_DEF_H

/*
文件说明:该文件为整个框架可见的宏，函数等定义
作者：zjb
时间：2014-4-20
*/
#include "config.h"


#include <stdio.h>
#include <stdlib.h>
#include "config.h"

#if _SW_DCF_WINDOWS


#include <winsock2.h>
#include <windows.h>  // for DWORD...
#include <assert.h>
#include <string.h>
#include <assert.h>
/* 数据库中需要bigint即int64 */
typedef unsigned __int64 uint64;
#else
#include "win_to_linux_type.h"
#include <unistd.h>
#include <assert.h>
#include "string.h"
#include <netinet/in.h>
#include <stdarg.h>
#include <string.h>

#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include "pthread.h"


// AES operates on 16 bytes at a time
#define AES_BLOCK_SIZE 16


#define strncpy_s strncpy
#define _stricmp  strcasecmp
#define sprintf_s snprintf


//linux线程优先级
#define THREAD_PRIORITY_TIME_CRITICAL 15  // value that gets a thread to LowRealtime-1
#define THREAD_PRIORITY_HIGHEST  2   // maximum thread base priority boost
#define THREAD_PRIORITY_NORMAL    0
#define THREAD_PRIORITY_BELOW_NORMAL  (-2)  // minimum thread base priority boost
#define THREAD_PRIORITY_IDLE (-15) // value that gets a thread to idle

#endif




// 调测开关
#define TEST_SWITCH 0
#if TEST_SWITCH
// #define TEST_OUT_TO_FILE
#define TEST_DEBUG
#define TEST_PRINT_TIMIER
// #define TEST_KERNEL_EVENT
#endif




// 发行版本是否包含调试开关的代码
#define INCLUDE_DEBUG_SWITCH_VAR 1

// typedef existing_type new_type_name ;
typedef void* DCFLPARAM ;

typedef DWORD (* DCF_FUNCPTR)(...);
typedef void (*DCF_FUNCPTR_VOID)();
typedef void* (*DCF_FUNCPTR_VOIDPTR)(...);



// 子系统初始化函数
typedef bool (* DCF_SYSINIT_FUNCPTR)(void *ptr);
#ifndef ASSERT
#define ASSERT assert
#endif

#define CATCH_ERR_RET(a,b) if(a) \
    return b;

// 任务名长度(不含0)
#define DCF_TASK_NAME_LEN 63
// 缺省可以管理的任务数量
#define DCF_TASK_NUM_DEF 64
#define DCF_WAIT_FOREVER ((DWORD)(-1))
/*设置一组任务优先级定义,避免大家乱定义*/
#define DCF_TASK_PRIORITY_IDLE         1      // 最低优先级
#define DCF_TASK_PRIORITY_LOW        2      // 低优先级
#define DCF_TASK_PRIORITY_NORMAL 3     // 一般任务的优先级
#define DCF_TASK_PRIORITY_HIGH        4     // 高优先级
#define DCF_TASK_PRIORITY_SUPER    5     // 最高优先级

/*设置一组任务栈大小*/
#define DCF_TASK_STACKSIZE_LOW   (64*1024)
#define DCF_TASK_STACKSIZE_NORMAL   (128*1024)
#define DCF_TASK_STACKSIZE_LARGE   (512*1024)
#define DCF_TASK_STACKSIZE_SUPER   (1024*1024)

#define DCF_MSGQUE_NAME_LEN    64         // 消息队列名称长度
#define DCF_MSGQUE_SIZE_MAX 1024         // 消息队列的最大长度
#define DCF_MSGQUE_FLAG_FIFO 0     // 队列先进先出
#define DCF_MSGQUE_FLAG_LIFO 1    // 队列后进先出


/*定时器定义*/
#define DCF_TIMER_PERIOD   0   // 周期定时器
#define DCF_TIMER_TIMEOUT 1    // 一次性超时定时器
#define DCF_TIMER_UNIT_MS 100  // 框架的定时器粒度,100毫秒

/*模块管理相关*/
#define DCF_MODULE_NAME_LEN 63  // 模块名 不包括0

/*定义一组无效句柄*/
#define DCF_INVALID_HANDLE_B  ((BYTE)-1)
#define DCF_INVALID_HANDLE_W  ((WORD)-1)
#define DCF_INVALID_HANDLE_DW ((DWORD)-1)

/*************************************************
子系统一组定义宏  begin
**************************************************/
#define DECLARE_SUBSYS(name) \
extern bool dcf_init_##name(void*);\
extern void dcf_exit_##name();

#define IMPLEMENT_SUBSYS(name,func_init,func_exit) \
bool dcf_init_##name(void*p) \
{ \
    return func_init(p); \
} \
void dcf_exit_##name() \
{\
    func_exit(); \
}

/*************************************************
子系统一组定义宏  end
**************************************************/

/*************************************************
系统定义宏:复位类型宏定义
**************************************************/
#define DCF_RESET_TYPE_CLOSE     1    // 停止框架
#define DCF_RESET_TYPE_WARM      2    // 软服务
#define DCF_RESET_TYPE_ERASE_DB  4    // 擦除数据

/*************************************************
系统定义宏:日志类型宏
**************************************************/
#define LOG_INFO_ERRLOG 1             // 系统复位日志

// 下面定义一组日志级别宏
#define LEVEL_FATAL_ERROR 1       // 致命错误(需要系统复位)
#define LEVEL_COMM_ERROR 2       // 一般错误(记录黑匣子)
#define LEVEL_LOW_ERROR    3       // 低级别错误(只打印)

/*****************************************************************
********** 数据通信相关 **********************************
*****************************************************************/
/* 字节序相关API */
#define dcf_htonl htonl
#define dcf_htons htons
#define dcf_ntohl ntohl
#define dcf_ntohs ntohs

/* 系统起始年份 */
const DWORD   DCF_YEAR_BEGIN = 2015;

/* 各层数据大小 */
/* 通信总长度，比广域网的1460定义略短，避免被拆分乱序丢包 ，因CRccHeader是20个字节，其后的报文需要加密，则必须为16的倍数，所以总长度为1012*/
const WORD RCC_SIZE_TOTAL = 1012;
// 这个宏的值要大于(按1024的倍数来)
const WORD MAX_SOCKBUF_LEN = ((RCC_SIZE_TOTAL*2 + 1023)/1024)*1024;

/* 文件系统相关 */
#define DCF_DIR_FPLEN_MAX 128


/*****************************************************************
********** 业务定义相关 **********************************
*****************************************************************/
#define QRCODE_LEN_MAX  64      /* 二维码所有信息包含之后的总长度最大值,采用新的算法之后二维码长度缩短为48个字符,这里也调小 */
/* 单品码 */
#define QRCODE_TYPE_SINGLE 1
/* 箱码 */
#define QRCODE_TYPE_BOX 2
/* 垛码 */
#define QRCODE_TYPE_BATTER 3
/* 场地码 */
#define QRCODE_TYPE_AREA 11
/* 生长码 */
#define QRCODE_TYPE_GROW 12
/* 农销码 */
#define QRCODE_TYPE_FARM 13
/* 农标码 */
#define QRCODE_TYPE_MARK 14
/* 农售码 */
#define QRCODE_TYPE_DETAIL 15
/* 农框码 */
#define QRCODE_TYPE_CONTAIN 16

struct zhw_datetime
{
    BYTE Year;   /* 相对于2015(DCF_YEAR_BEGIN)的减量 */
    BYTE Month;
    BYTE Day;
    BYTE Hour;
    BYTE Minute;
    BYTE Second;
};


#endif

