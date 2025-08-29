
#include "extern_hook_api.h"
#include <stdarg.h>
#include "dcf_err.h"

#if _SW_DCF_MACOS
#define vsprintf_s(a,b,c,d) vsprintf(a,c,d)
#endif

DWORD g_malloc_times = 0;
DWORD g_free_times = 0;
extern DWORD g_log_type;    /* 1:输出到回调变量 2:输出到日志 3:输出到日志和控制台*/
DWORD g_log_type = 1;

void dcf_output(const char *fmt,...)
{
    if ((!g_print_debugstring) && (g_log_type < 2))
    {
        return;
    }

    va_list ArgList;
    char buf[1024];
    va_start(ArgList, fmt);
    int ilen = vsprintf_s(buf, 1024, fmt, ArgList);
    va_end(ArgList);

    if ((g_log_type == 2) || (g_log_type == 3))
    {
//        extern void dcf_log_write(char *str,int ilen);
//        dcf_log_write(buf,ilen);
    }

    if ((g_log_type == 1) || (g_log_type == 3))
    {
        g_print_debugstring(buf);
    }
}

void* dcf_mem_malloc_ex(DWORD dwSize,DWORD dwFileLine,char *pFileName)
{
    if(!dwSize)
    {
        // 不要申请0字节
        dwSize = 1;
    }

    g_malloc_times++;

    if(g_malloc_mem)
    {
        return g_malloc_mem(dwSize,dwFileLine,pFileName);
    }

    return malloc(dwSize);
}

DWORD dcf_mem_free_ex(void *&p,DWORD dwFileLine,char *pFileName)
{
    if (!p)
    {
        // 参数校验
        return 0;
    }

    g_free_times++;

    if(g_malloc_free)
    {
        return g_malloc_free(p,dwFileLine,pFileName);
    }

    free(p);
    p = NULL;
    return DCF_SUCCESS;
}
// new/delete 的6种重载方法 http://www.cnblogs.com/zhenjing/archive/2011/01/archive/2011/01/10/groble_new.html
void *operator new(size_t size)
{
    return dcf_mem_malloc_ex((int)size,0,"new.c");
}
void operator delete(void *p)
{
    dcf_mem_free_ex(p,0,"new.c");
}



