#ifndef extern_hook_api_h
#define extern_hook_api_h
/*
文件说明:该文件包含用户可以定义的外挂的API函数
作者：zjb
时间：2014-4-20
*/

#include "dcf_def.h"

/*****************************************************************
********** 用户外挂API定义区域 **********************************
*****************************************************************/
// 调试信息输出函数
typedef void(*CALL_FUNC_PRINT_DEBUGSTRING)(char *p);
// 申请和释放内存函数
typedef void*(*CALL_FUNC_MALLOC)(DWORD dwSize,DWORD dwFileLine,char *pFileName);
typedef DWORD(*CALL_FUNC_FREE)(void *&p,DWORD dwFileLine,char *pFileName);
extern CALL_FUNC_PRINT_DEBUGSTRING g_print_debugstring;
extern CALL_FUNC_MALLOC g_malloc_mem;
extern CALL_FUNC_FREE g_malloc_free;

#endif
