
#ifndef _DCF_LOCK_H
#define _DCF_LOCK_H
/*
本文件提供线程、进程间同步的三种基类
*/
#include "dcf_def.h"
#if _SW_DCF_WINDOWS
#include <windows.h>
#endif
#if _SW_DCF_MACOS
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <memory.h>

#define STATUS_WAIT_0        ((DWORD   )0x00000000L)

#define CRITICAL_SECTION pthread_mutex_t
#define DeleteCriticalSection(p) pthread_mutex_destroy(p)
#define EnterCriticalSection(p) pthread_mutex_lock(p)
#define LeaveCriticalSection(p) pthread_mutex_unlock(p)
#define TryEnterCriticalSection(p) pthread_mutex_trylock(p)

#define InitializeCriticalSection(p)\
 { pthread_mutexattr_t attr; \
   pthread_mutexattr_init(&attr); \
 pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE); \
 pthread_mutex_init((p), &attr); \
 }

#define CloseHandle(p) pthread_mutex_destroy(p)
#endif


/*
临界区互斥,特点:
1.范围:不是内核对象,进程内
2.效率:9条指令，最快
3.操作函数:EnterCriticalSection,LeaveCriticalSection
4.任何一个时刻只允许一个线程在临界区
*/
class critical_section
{
public:
    critical_section();
    ~critical_section();
    void initialize();
    void lock();
    DWORD try_lock(DWORD ms);
    void unlock();
protected:
    CRITICAL_SECTION m_CriticalSection;

};


/*
互斥的互斥,特点:
1.是内核对象，进程间(匿名只能是进程内)
2.效率:低，600条指令
3.操作函数:WaitForSingleObject,ReleaseMutex
4.任何一个时刻只允许一个线程在临界区
*/
class mutex
{
public:
    mutex();
    ~mutex();
    DWORD try_lock(DWORD dwMs);
    void initialize(char *pname);
    void lock();
    void unlock();
protected:
#if _SW_DCF_WINDOWS
    HANDLE m_mutex;
#endif
#if _SW_DCF_MACOS
    sem_t *m_pSem;
#endif


};

/*
http://www.jb51.net/article/56050.htm
信号量的互斥特点:
1.单进程内有效
2.可以允许多个线程进入临界区
*/


class semaphore
{
public:
    semaphore();
    ~semaphore();
    void initialize(char *pname,LONG lmin,LONG lMax);
    void wait_semaphore();
    void set_semaphore();
    //DWORD try_wait_semaphore(DWORD dwMs);
    void close();
protected:
#if _SW_DCF_WINDOWS
    HANDLE   m_hSemaphore;
#else
    sem_t   m_sem;
#endif
};



/*
http://chinaxyw.iteye.com/blog/548622
备注:
事件的状态:激发态(signaled or true)和未激发态(unsignal or false)
事件的互斥特点:
1.可以跨进程
2.只与信号有关，与线程数无关
*/
#if _SW_DCF_WINDOWS
#define event_handle HANDLE
#endif
#if _SW_DCF_MACOS
#include <sys/time.h>
#include <errno.h>
typedef struct
{
    BOOL state;
    BOOL manual_reset;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}event_t;
#define event_handle event_t*
#endif

class sys_event
{
public:
    sys_event();
    ~sys_event();
    void initialize(const char *pname = NULL,BOOL bmanu = TRUE,BOOL binit_havesigna = FALSE);
    // 将事件设置为有信号态
    void set_event();
    // 设置为无信号态
    void reset_event();
    // 等待信号
    DWORD wait_event(DWORD ms);
    void close();
protected:
    event_handle   m_hEvent;
};

#endif
