
#include "dcf_err.h"
#include "dcf_lock.h"
#include "extern_api.h"
#if _SW_DCF_MACOS
#include <sys/time.h>
#include <errno.h>
#include <new>
#endif
// WaitForSingleObject函数可以等待的资源:Event、Job、Memory resource notification、Mutex、Process、Semaphore、Thread、Waitable timer
// 其返回只有2种情况:
// 1.时间到,返回:WAIT_TIMEOUT   0x00000102L
// 2.对象有信号 :WAIT_OBJECT_0 0x00000000L

critical_section::critical_section()
{
    memset(&m_CriticalSection,0,sizeof(m_CriticalSection));
}

critical_section::~critical_section()
{
    DeleteCriticalSection(&m_CriticalSection);
}

void critical_section::initialize()
{
    InitializeCriticalSection(&m_CriticalSection);
}
void critical_section::lock()
{
    EnterCriticalSection(&m_CriticalSection);
}
void critical_section::unlock()
{
    LeaveCriticalSection(&m_CriticalSection);
}
DWORD critical_section::try_lock(DWORD ms)
{
    const DWORD sleep_ms = 5;

    while(true)
    {
        if (TryEnterCriticalSection(&m_CriticalSection))
        {
            return DCF_SUCCESS;
        }

        if (ms < sleep_ms)
        {
            break;
        }

        ms -= sleep_ms;
    }

    return DCF_ERR_TIMEOUT;
}

#if _SW_DCF_WINDOWS
mutex::mutex()
{
    m_mutex = NULL;
}

mutex::~mutex()
{
    if (m_mutex)
    {
        ::CloseHandle(m_mutex);
        m_mutex = NULL;
    }
}

void mutex::initialize(char *pname)
{
    m_mutex = ::CreateMutexA(NULL, FALSE, pname);
}
void mutex::lock()
{
    if (!m_mutex)
    {
        return;
    }

   (void)::WaitForSingleObject(m_mutex, INFINITE);
   
}
void mutex::unlock()
{
    if (m_mutex)
    {
        ::ReleaseMutex(m_mutex);
    }
}
DWORD mutex::try_lock(DWORD dwMs)
{
    if (!m_mutex)
    {
        return DCF_ERR_FAILED;
    }

    if (::WaitForSingleObject(m_mutex, dwMs) == WAIT_OBJECT_0)
    
    {
        return DCF_SUCCESS;
    }
    return DCF_ERR_TIMEOUT;    
}

#endif


/*http://www.cnblogs.com/jiangwang2013/p/3726097.html*/
#if _SW_DCF_MACOS
mutex::mutex()
{
    m_pSem = NULL;
}

mutex::~mutex()
{
    int ret = sem_close(m_pSem);
    if (0 != ret)
    {
        printf("sem_close error %d\n", ret);
    }

}

void mutex::initialize(char *pname)
{
    m_pSem = ::sem_open(pname, O_RDWR | O_CREAT, 0644, 1);
}

void mutex::lock()
{
    (void)::sem_wait(m_pSem);
}

void mutex::unlock()
{
    (void)::sem_post(m_pSem);
}

DWORD mutex::try_lock(DWORD dwMs)
{
    if (!m_pSem)
    {
        return DCF_ERR_FAILED;
    }
    if (  ::sem_wait(m_pSem) == 0)
    {
        return DCF_SUCCESS;
    }
    return DCF_ERR_TIMEOUT;
}

#endif



semaphore::semaphore()
{
#if  _SW_DCF_WINDOWS
    m_hSemaphore = NULL;
#endif
}

semaphore::~semaphore()
{
    close();
}

void semaphore::initialize(char *pname,LONG lmin,LONG lMax)
{
#if  _SW_DCF_WINDOWS
    m_hSemaphore = ::CreateSemaphoreA(NULL, lmin, lMax, pname);
#else
    int rs = ::sem_init(&m_sem, 0, lmin);
    assert(0 == rs);
#endif
}
void semaphore::wait_semaphore()
{
#if  _SW_DCF_WINDOWS
    ::WaitForSingleObject(m_hSemaphore, INFINITE);
#else
    int rs = ::sem_wait(&m_sem);
    assert(0 == rs);
#endif
}
void semaphore::set_semaphore()
{
#if  _SW_DCF_WINDOWS
    if (m_hSemaphore)
    {
        ::ReleaseSemaphore(m_hSemaphore, 1, NULL);
    }
#else
    int rs = ::sem_post(&m_sem);
    assert(0 == rs);
#endif

}
// DWORD semaphore::try_wait_semaphore(DWORD dwMs)
// {
// #if  _SW_DCF_WINDOWS
//     if (!m_hSemaphore)
//     {
//         return DCF_ERR_FAILED;
//     }

//     if (::WaitForSingleObject(m_hSemaphore, dwMs) == WAIT_OBJECT_0)
  
//     {
//         return DCF_SUCCESS;
//     }
// #else

//     struct timespec abstime;
//     clock_gettime(CLOCK_REALTIME, &abstime);
//     int s = (dwMs>1000)?dwMs/1000:0;
//     int ms = dwMs - s*1000;
//     abstime.tv_sec += s;
//     abstime.tv_nsec += ((long)ms) * 1000 * 1000;
//     if (abstime.tv_nsec >= 1000000000)
//     {
//         abstime.tv_nsec -= 1000000000;
//         abstime.tv_sec++;
//     }
//     int res = sem_timedwait(&m_sem, &abstime);
//     if(res == 0)
//     {
//         return DCF_SUCCESS;
//     }

// #endif

//     return DCF_ERR_TIMEOUT;
// }

void semaphore::close()
{
#if  _SW_DCF_WINDOWS
    if (m_hSemaphore)
    {
        ::CloseHandle(m_hSemaphore);
        m_hSemaphore = NULL;
    }
#else
    int rs = ::sem_destroy(&m_sem);
    assert(0 == rs);
#endif
}

sys_event::sys_event()
{
    m_hEvent = NULL;
}

sys_event::~sys_event()
{
    close();
}

void sys_event::initialize(const char *pname,BOOL bmanu,BOOL binit_havesigna)
{
#if _SW_DCF_WINDOWS
    m_hEvent = ::CreateEventA(NULL,bmanu,binit_havesigna,pname);
#endif

#if _SW_DCF_MACOS
    m_hEvent = new(std::nothrow) event_t;
    if (m_hEvent == NULL)
    {
        return ;
    }
    m_hEvent->state = binit_havesigna;

    m_hEvent->manual_reset = bmanu;
    if (pthread_mutex_init(&m_hEvent->mutex, NULL))
    {
        delete m_hEvent;
        m_hEvent = NULL;
        return ;
    }
    if (pthread_cond_init(&m_hEvent->cond, NULL))
    {
        pthread_mutex_destroy(&m_hEvent->mutex);
        delete m_hEvent;
        m_hEvent = NULL;
        return ;
    }
#endif
}

void sys_event::set_event()
{
#if _SW_DCF_WINDOWS
    if (m_hEvent)
    {
        ::SetEvent(m_hEvent);
    }
#endif
#if _SW_DCF_MACOS
    if (pthread_mutex_lock(&m_hEvent->mutex) != 0)
    {
        return ;
    }

    m_hEvent->state = true;

    if (m_hEvent->manual_reset)
    {
        if(pthread_cond_broadcast(&m_hEvent->cond))
        {
            return ;
        }
    }
    else
    {
        if(pthread_cond_signal(&m_hEvent->cond))
        {
            return ;
        }
    }

    if (pthread_mutex_unlock(&m_hEvent->mutex) != 0)
    {
        return ;
    }

    return ;
#endif



}

void sys_event::reset_event()
{
#if _SW_DCF_WINDOWS
    if (m_hEvent)
    {
        ::ResetEvent(m_hEvent);
    }
#endif
#if _SW_DCF_MACOS
    if (pthread_mutex_lock(&m_hEvent->mutex) != 0)
    {
        return ;
    }

    m_hEvent->state = false;

    if (pthread_mutex_unlock(&m_hEvent->mutex) != 0)
    {
        return ;
    }
    return ;
#endif
}

DWORD sys_event::wait_event(DWORD ms)
{
#if _SW_DCF_WINDOWS
    if (!m_hEvent)
    {
        return DCF_ERR_FAILED;
    }

    if (::WaitForSingleObject(m_hEvent, ms) == WAIT_OBJECT_0)
  
    {
        return DCF_SUCCESS;
    }
    return DCF_ERR_TIMEOUT;
#endif

#if _SW_DCF_MACOS

    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    int s = (ms>1000)?ms/1000:0;
    int dwMs = ms - s*1000;
    abstime.tv_sec += s;
    abstime.tv_nsec += ((long)dwMs) * 1000 * 1000;
    if (abstime.tv_nsec >= 1000000000)
    {
        abstime.tv_nsec -= 1000000000;
        abstime.tv_sec++;
    }

    if (pthread_mutex_lock(&m_hEvent->mutex))
    {
        return DCF_ERR_FAILED;
    }
    while (!m_hEvent->state)
    {
        if (pthread_cond_timedwait(&m_hEvent->cond, &m_hEvent->mutex,&abstime))
        {
            pthread_mutex_unlock(&m_hEvent->mutex);
            return DCF_ERR_FAILED;
        }
    }

    if (!m_hEvent->manual_reset)
    {
        m_hEvent->state = false;
    }
    if (pthread_mutex_unlock(&m_hEvent->mutex))
    {
        return DCF_ERR_FAILED;
    }

    return DCF_SUCCESS;
#endif

}

void sys_event::close()
{
#if _SW_DCF_WINDOWS
    if (m_hEvent)
    {
        ::CloseHandle(m_hEvent);
        m_hEvent = NULL;
    }
#endif
#if _SW_DCF_MACOS
    pthread_cond_destroy(&m_hEvent->cond);
    pthread_mutex_destroy(&m_hEvent->mutex);
    delete m_hEvent;
    m_hEvent = NULL;
#endif
}


