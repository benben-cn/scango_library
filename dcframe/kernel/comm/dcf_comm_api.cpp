
#include "dcf.h"
#include "dcf_def.h"
#include "dcf_err.h"
#include "cJSON.h"
#include "dcf_string.h"
#include "dcf_lock.h"

#if  _SW_DCF_LINUX
#include <ctype.h>
#include <stdarg.h>
#include "dcf_def.h"
#define vsprintf_s(a,b,c,d) vsprintf(a,c,d)
#endif

critical_section    m_err_log_lock;//err_log文件锁
/*
下面的hash算法称为:One-Way Hash，来自http://kb.cnblogs.com/page/189480/
*/
static DWORD g_cryptTable[0x500];
void dcf_prepareCryptTable()
{
    static bool g_hasinit_hashtable = false;
    if (g_hasinit_hashtable)
    {
        return;
    }
    unsigned int seed = 0x00100001, index1 = 0, index2 = 0, i;
    for( index1 = 0; index1 < 0x100; index1++ )
    {
        for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
        {
            unsigned int temp1, temp2;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp1 = (seed & 0xFFFF) << 0x10;
            seed = (seed * 125 + 3) % 0x2AAAAB;
            temp2 = (seed & 0xFFFF);
            g_cryptTable[index2] = ( temp1 | temp2 );
        }
    }
}

/*
函数名：Hash关键值计算
参  数：
作  者：zjb
时  间：2017-4-24
*/

DWORD dcf_hash_string(const char *lpszFileName, DWORD dwHashType)
{
    BYTE *key = (BYTE *)lpszFileName;
    DWORD seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
    int ch;
    while (*key != 0)
    {
        ch = toupper(*key++);
        seed1 = g_cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2);
        seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
    }
    return seed1;
}

// 增加时间打印函数windows和linux不一样

void dcf_reboot_ex(DWORD sysid,DWORD reset_type,WORD modid,char *pinfo)
{

}

void dcf_sys_write_log(DWORD sysid,WORD logtypeid,WORD level,char *pinfo)
{

}

/****************************************************************
*功能描述 : 检查日志的函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-10  16:15:1
****************************************************************/
void dcf_sys_checkerr_ex(DWORD dwRet,BYTE byLevel,WORD modid,char *errinfo,const char *pfilename,DWORD dwFileLine,const char *pfuncname)
{
    if (!dwRet)
    {
        return;
    }

    static bool bInit = false;
    if (!bInit)
    {
        m_err_log_lock.initialize();
        bInit = true;
    }

    m_err_log_lock.lock();

    static char buffer[1024];
    int ilen = sprintf_s(buffer,sizeof(buffer),"[%s]error:%d,level:%d,mod:0x%x,func:%s,filen:%s,line:%d,info:%s\r\n",
                         dcf_get_time_string(buffer),dwRet,byLevel,modid,pfuncname, \
                pfilename,dwFileLine,errinfo);
//    extern void dcf_log_write(char *str,int ilen);
//    dcf_log_write(buffer,ilen);
    dcf_output(buffer);
    m_err_log_lock.unlock();

}

void dcf_sys_checker_fmt_ex(DWORD dwRet,BYTE byLevel,WORD modid,const char *pfilename,DWORD dwFileLine,const char *pfuncname,char *fmt,...)
{
    if (!dwRet)
    {
        return;
    }
    va_list ArgList;
    char info[512];

    va_start(ArgList, fmt);
    (void)vsprintf_s(info, sizeof(info), fmt, ArgList);
    va_end(ArgList);

    dcf_sys_checkerr_ex(dwRet,byLevel,modid,info,pfilename,dwFileLine,pfuncname);
}



DWORD dcf_trans_task_priority(BYTE priority)
{
    DWORD dwRet = 0;
    switch(priority)
    {
        case DCF_TASK_PRIORITY_IDLE:
            dwRet = THREAD_PRIORITY_IDLE;
            break;
        case DCF_TASK_PRIORITY_LOW:
            dwRet = THREAD_PRIORITY_BELOW_NORMAL;
            break;
        case DCF_TASK_PRIORITY_NORMAL:
            dwRet = THREAD_PRIORITY_NORMAL;
            break;
        case DCF_TASK_PRIORITY_HIGH:
            dwRet = THREAD_PRIORITY_HIGHEST;
            break;
        case DCF_TASK_PRIORITY_SUPER:
            dwRet = THREAD_PRIORITY_TIME_CRITICAL;
            break;
        default:
            dcf_output("unkown task priority(%d)\r\n",priority);
            dwRet = THREAD_PRIORITY_NORMAL;
            break;
    }
    return dwRet;
}

/****************************************************************
*功能描述 : 获取配置项中的数字值
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :
*创建日期 : 2017-05-10  11:1:48
****************************************************************/
DWORD dcf_cjson_get_numvar(void *root,const char *valuename,DWORD def_value,DWORD max_value)
{
            ASSERT( root!= NULL);
    cJSON *pItem = cJSON_GetObjectItem((cJSON *)root,valuename);
    DWORD dwValue = def_value;
    if (pItem)
    {
        dwValue = pItem->valueint;// (DWORD)dcf_strtools::strtol(pItem->valuestring);
        if ((max_value) && (dwValue > max_value))
        {
            dwValue = def_value;
        }
    }
    return dwValue;
}
/****************************************************************
*功能描述 : 获取字符串值
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-16  10:27:53
****************************************************************/
char* dcf_cjson_get_stringvar(void *root,const char *valuename,char *chbuffer,DWORD buflen,const char *valuedef)
{
            ASSERT(root != NULL);
    CATCH_ERR_RET(!valuename,NULL);
    CATCH_ERR_RET(!chbuffer,NULL);
    CATCH_ERR_RET(!buflen,NULL);

    char *pRet = NULL;
    cJSON *pItem = cJSON_GetObjectItem((cJSON *)root,valuename);
    if (pItem && pItem->valuestring)
    {
        dcf_strtools::strcpy_s(chbuffer,pItem->valuestring,buflen);
    }
    else if (valuedef)
    {
        dcf_strtools::strcpy_s(chbuffer,valuedef,buflen);
    }
    return chbuffer;
}


DWORD dcf_ini_get_string(const char *title, const char *key, const char *def, char *buffer, int buf_len, const char *filename)
{
#ifdef WIN32
    return GetPrivateProfileStringA(title, key, def, buffer, buf_len, filename);
#else
#endif
    return 0;
}

int dcf_ini_get_int(const char *title,const char *key,int def,const char *filename)
{
#ifdef WIN32
    return GetPrivateProfileIntA(title,key,def,filename);
#else
#endif
    return def;
}


