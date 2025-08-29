
#include "dcf_time.h"
#include "dcf_def.h"
#include "extern_api.h"

#if  _SW_DCF_MACOS
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
int  dcf_time_compare(clock_t ctime1,clock_t ctime2);
clock_t dcf_time_add(clock_t &ctime1,clock_t &ctime2);
clock_t dcf_time_minus(clock_t ctime1,clock_t ctime2);
clock_t dcf_time_add_num(clock_t &ctime1,DWORD num);
#endif

// 毫秒和clock的转换(一般等于1)
// windows:clock表示毫秒 #define CLOCKS_PER_SEC ((clock_t)1000)
// linux/macOS:clock表示微妙 #define CLOCKS_PER_SEC ((clock_t)1000000)
//const long CLOCKS_PER_MS = (CLOCKS_PER_SEC/1000);
const long CLOCKS_PER_MS = 1000; //no matching conversion for C-style cast from 'int' to 'struct timespec'

clock_t dcf_time_get_cur_clock()
{
#if _SW_DCF_WINDOWS
    return clock();
#else
    clock_t cur;
    clock_gettime(CLOCK_REALTIME, & cur);
    return cur;
#endif
}


DWORD dcf_time_get_ms_interval(clock_t start,clock_t cur)
{
#if _SW_DCF_WINDOWS
    return (DWORD)((cur - start) / CLOCKS_PER_MS);
#else
    clock_t tmp = dcf_time_minus(cur,start);
    return  tmp.tv_sec*1000+tmp.tv_nsec/1000000;
#endif

}




bool dcf_time_exact_work_and_sleep(clock_t &pre_work,clock_t &pre_sleep,DWORD workms,DWORD sleepms)
{
#if _SW_DCF_WINDOWS
    clock_t cur = clock();
    // 先计算休息的事情
    long worktick = workms * CLOCKS_PER_MS;
    long sleeptick = sleepms * CLOCKS_PER_MS;
    // 先解决工作的事情，如果工作时间到那么

    if ((!pre_work) || (pre_work > cur))
    {
        // 没有初始化或者是反转
        pre_work = cur;
    }

    if ((!pre_sleep) || (pre_sleep > cur))
    {
        // 没有初始化或者是反转
        pre_sleep = cur;
    }

    // 先计算本次要休息的时间
    long sleepleft = 0;
    if ((pre_sleep + sleeptick) > cur)
    {
        sleepleft = pre_sleep + sleeptick - cur;
    }

    if ((cur + sleepleft) >= (pre_work + worktick))
    {
        // 要立即工作了，不能休息
        pre_work = cur;
        return true;
    }

    if (sleepleft > 0)
    {
     // 休息吧
        dcf_time_sleep(DWORD(sleepleft / CLOCKS_PER_MS));

        // 休息之后要重新刷新,可能有任务切换调度，不一定是要求的那么长
        pre_sleep = clock();
    }

#else
    clock_t cur;
    clock_gettime(CLOCK_REALTIME, & cur);
    if ((pre_work.tv_sec==0 && pre_work.tv_nsec==0) || (dcf_time_compare(pre_work,cur) != -1))
    {
        // 没有初始化或者是反转
        pre_work = cur;
    }

    if ((pre_sleep.tv_sec==0 && pre_sleep.tv_nsec==0) || (dcf_time_compare(pre_sleep,cur)!= -1))
    {
        // 没有初始化或者是反转
        pre_sleep = cur;
    }
    // 先计算本次要休息的时间
    clock_t sleepleft;
    memset(&sleepleft, 0, sizeof(sleepleft));

    if (dcf_time_compare(dcf_time_add_num(pre_sleep,sleepms),cur)==1)
    {
        sleepleft = dcf_time_minus(dcf_time_add_num(pre_sleep,sleepms),cur);
    }

    if(0==dcf_time_compare(dcf_time_add(cur,sleepleft),dcf_time_add_num(pre_work,workms)) ||
       1==dcf_time_compare(dcf_time_add(cur,sleepleft),dcf_time_add_num(pre_work,workms)))
    {
        // 要立即工作了，不能休息
        pre_work = cur;
        //dcf_output("[%s]timer spool!\r\n",dcf_get_time_string_unsafe());
        //inum++;
        return true;
    }

    if (sleepleft.tv_sec > 0 ||sleepleft.tv_nsec > 0)
    {
        // 休息吧
        dcf_time_sleep(sleepleft);

        // 休息之后要重新刷新,可能有任务切换调度，不一定是要求的那么长
        clock_gettime(CLOCK_REALTIME, & pre_sleep);
    }

#endif
    //inum++;
    return false;
}

#if _SW_DCF_MACOS
//ctime1 > ctime2 return 1;
//ctime1 = ctime2 return 0;
int dcf_time_compare(clock_t ctime1,clock_t ctime2)
{
    if ((ctime1.tv_sec > ctime2.tv_sec)||((ctime1.tv_sec == ctime2.tv_sec) && (ctime1.tv_nsec > ctime2.tv_nsec)))
        return 1;
    else if((ctime1.tv_sec == ctime2.tv_sec) && (ctime1.tv_nsec == ctime2.tv_nsec))
        return 0;
    return -1;
}

clock_t dcf_time_add(clock_t &ctime1,clock_t &ctime2)
{
    clock_t tmp;
    tmp.tv_sec  = ctime1.tv_sec+ctime2.tv_sec;
    tmp.tv_nsec = ctime1.tv_nsec+ctime2.tv_nsec;
    if(tmp.tv_nsec>=1000000000)
    {
        tmp.tv_sec += 1;
        tmp.tv_nsec %= 1000000000;
    }
    return tmp;
}

//ctime1 > ctime2
clock_t dcf_time_minus(clock_t ctime1,clock_t ctime2)
{
    clock_t tmp;
    memset(&tmp, 0, sizeof(tmp));
    int re = dcf_time_compare(ctime1,ctime2);
    if(re >= 0)
    {
        if(ctime1.tv_nsec < ctime2.tv_nsec)
        {
            tmp.tv_nsec = ctime1.tv_nsec+1000000000-ctime2.tv_nsec;
            tmp.tv_sec = ctime1.tv_sec - ctime2.tv_sec -1;
        }
        else
        {
            tmp.tv_nsec = ctime1.tv_nsec-ctime2.tv_nsec;
            tmp.tv_sec = ctime1.tv_sec - ctime2.tv_sec;
        }
    }
    return tmp;
}

clock_t dcf_time_add_num(clock_t &ctime,DWORD num)
{
    clock_t tmp = ctime;
    int s = (num>1000)?num/1000:0;
    int ms = num - s*1000;
    tmp.tv_sec += s;
    tmp.tv_nsec += ((long)ms) * 1000000;
    if(tmp.tv_nsec>=1000000000)
    {
        tmp.tv_sec += 1;
        tmp.tv_nsec %= 1000000000;
    }
    return tmp;

}

void dcf_time_sleep(clock_t ms)
{
    /*    #if  _SW_DCF_MACOS
    struct timespec ts;
    ts.tv_sec = ms/1000;
    ts.tv_nsec = (ms%1000)*1000*1000;
    nanosleep(&ts,NULL);

    usleep(CLOCKS_PER_MS*ms);

    #else */
    struct timeval tmp;
    tmp.tv_sec = ms.tv_sec;
    tmp.tv_usec = ms.tv_nsec/1000;
    select(0,NULL,NULL,NULL,&tmp);
}
#endif


void dcf_time_sleep(DWORD ms)
{
#if _SW_DCF_WINDOWS
    Sleep(ms);
#else
    struct timeval tmp;
    tmp.tv_sec = ms/1000;
    tmp.tv_usec =(ms%1000)*1000;
    select(0,NULL,NULL,NULL,&tmp);
#endif
}

char* dcf_get_time_string(char chbuffer[32],int iflag)
{
    char *p = chbuffer;
#if  _SW_DCF_MACOS
    struct tm *sys;
    struct timezone tz;
    struct timeval tv;
    gettimeofday(&tv, &tz);
    sys = localtime(&tv.tv_sec);
    if (iflag & NEED_DAY)
    {
        p += sprintf(p,"%4d-%02d-%02d ",sys->tm_year+1900,sys->tm_mon+1,sys->tm_mday);
    }
    if (iflag & NEED_TIME)
    {
        p += sprintf(p,"%2d:%02d:%02d ",sys->tm_hour,sys->tm_min,sys->tm_sec);
    }

    if (iflag & NEED_MS)
    {
        p += sprintf(p,"%06d",tv.tv_usec);
    }

#endif

#if _SW_DCF_WINDOWS
    SYSTEMTIME sys;
    GetLocalTime( &sys );

    if (iflag & NEED_DAY)
    {
        p += sprintf(p,"%4d-%02d-%02d ",sys.wYear,sys.wMonth,sys.wDay);
    }


    if (iflag & NEED_TIME)
    {
        p += sprintf(p,"%02d:%02d:%02d ",sys.wHour,sys.wMinute,sys.wSecond);
    }

     if (iflag & NEED_MS)
    {
        p += sprintf(p,"%3d ",sys.wMilliseconds);
    }
      // 将0转换为空格
    if (p != chbuffer)
    {
        p--;
        *p = 0;
    }
#endif



    return chbuffer;
}

char *dcf_get_time_string_unsafe(int iflag)
{
    static char chbuffer[32] = {0};
    return dcf_get_time_string(chbuffer,iflag);
}

void dcf_print_oneminute()
{
    extern DWORD g_malloc_times,g_free_times;
    dcf_output("[%s]one minute(%d,%d,%d)...\r\n",dcf_get_time_string_unsafe(),g_malloc_times,g_free_times,g_malloc_times-g_free_times);
}

