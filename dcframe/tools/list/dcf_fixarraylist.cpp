
#include "dcf_fixarraylist.h"
#include "extern_api.h"
#include "dcf_err.h"

#ifdef TEST_KERNEL_EVENT
static DWORD g_que_print_switch_debug = 0;
#endif

CQueArray::CQueArray(WORD node_len,WORD node_nums)
{
    if (node_len < sizeof(BYTE))
    {
        node_len = sizeof(BYTE);
    }

    if (!node_len)
    {
        node_len = 1;
    }

    m_wConfignums = node_nums;
    m_wUserDataLen = node_len;
    m_wHeadPt = 0;
    m_wTailPt = 0;
    m_wNodeNums = 0;
    // 按照指针的方式对齐
    m_wOneNodeLen = ((m_wUserDataLen + sizeof(DCFLPARAM) - 1)/sizeof(DCFLPARAM))*sizeof(DCFLPARAM);
    DWORD dwSize = m_wConfignums * m_wOneNodeLen;
    m_pData = (BYTE*)dcf_mem_malloc(dwSize);
    memset(m_pData,0,dwSize);
}

CQueArray::~CQueArray()
{
    void *p = NULL;
    while((p = PopNode()) != NULL)
    {
        // 还有消息内容
        ClearContent(p);
    }

    dcf_mem_free((void*&)m_pData);
    m_wNodeNums = 0;
}

void CQueArray::ClearContent(void *p)
{
    memset(p,0,m_wUserDataLen);
}

DWORD CQueArray::Pop(void *p)
{
    void *pself = PopNode();
    if (!pself)
    {
        return DCF_ERR_EMPTY;
    }

    if (p)
    {
        memcpy(p,pself,m_wUserDataLen);
    }

    return DCF_SUCCESS;
}
void *CQueArray::PopNode()
{
#ifdef TEST_KERNEL_EVENT
    Print("PopNode-->\r\n");
#endif

    if (!m_wNodeNums)
    {
#ifdef TEST_KERNEL_EVENT
        Print("PopNode null\r\n");
#endif
        return NULL;
    }

    void *p = &m_pData[m_wHeadPt*m_wOneNodeLen];
    m_wNodeNums--;
    if (m_wNodeNums)
    {
        // 如果有数据则移动，否则不移动
        m_wHeadPt = (m_wHeadPt+1)%m_wConfignums;
    }
    else
    {
                ASSERT(m_wHeadPt == m_wTailPt);
    }
#ifdef TEST_KERNEL_EVENT
    Print("PopNode\r\n");
#endif
    return p;
}


DWORD CQueArray::PushHead(void *p)
{
    if(m_wNodeNums >= m_wConfignums)
    {
        return DCF_ERR_FULL;
    }

#ifdef TEST_KERNEL_EVENT
    Print("PushHead-->\r\n");
#endif
    if (!m_wNodeNums)
    {
        // 此时没有节点，头和尾均指向空节点
        // 就在当前位置插入即可
        memcpy(&m_pData[m_wHeadPt*m_wOneNodeLen],p,m_wUserDataLen);
                ASSERT(m_wHeadPt == m_wTailPt);
    }
    else
    {
        // 有节点，则头和尾均是有效节点，那么要先偏移
        m_wHeadPt = (m_wHeadPt > 0)?(m_wHeadPt - 1):(m_wConfignums - 1);
        memcpy(&m_pData[m_wHeadPt*m_wOneNodeLen],p,m_wUserDataLen);
    }

    m_wNodeNums++;

#ifdef TEST_KERNEL_EVENT
    Print("PushHead\r\n");
#endif
    return DCF_SUCCESS;
}

DWORD CQueArray::PushTail(void *p)
{
    if(m_wNodeNums >= m_wConfignums)
    {
        return DCF_ERR_FULL;
    }

#ifdef TEST_KERNEL_EVENT
    Print("PushTail-->\r\n");
#endif

    if (!m_wNodeNums)
    {
        // 此时没有节点，头和尾均指向空节点
        // 就在当前位置插入即可
        memcpy(&m_pData[m_wTailPt*m_wOneNodeLen],p,m_wUserDataLen);
                ASSERT(m_wHeadPt == m_wTailPt);
    }
    else
    {
        // 有节点，则头和尾均是有效节点，那么要先偏移
        m_wTailPt = (m_wTailPt + 1)%m_wConfignums;
        memcpy(&m_pData[m_wTailPt*m_wOneNodeLen],p,m_wUserDataLen);
    }

    m_wNodeNums++;

#ifdef TEST_KERNEL_EVENT
    Print("PushTail\r\n");
#endif
    return DCF_SUCCESS;
}

#ifdef TEST_KERNEL_EVENT
void CQueArray::Print(const char *op)
{
    if (!g_que_print_switch_debug)
    {
        return;
    }
    dcf_output("%s:nums:%d,head:%d,tail:%d\r\n",op,m_wNodeNums,m_wHeadPt,m_wHeadPt);
}
#endif
/****************************************************************
*功能描述 : 将srcQue队列中的数据逆序添加
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-11  11:9:55
****************************************************************/
DWORD CQueArray::PushHead(CQueArray *srcQue)
{
    return Push(srcQue,true);
}
DWORD CQueArray::PushTail(CQueArray *srcQue)
{
    return Push(srcQue,true);
}
/****************************************************************
*文件范围 : 将srcQue队列中的数据添加到自己的队列
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-11  11:25:50
****************************************************************/
DWORD CQueArray::Push(CQueArray *srcQue,bool bHead)
{
    if (!srcQue)
    {
        return DCF_ERR_PARAM;
    }

    if (srcQue->GetUserDataLen() != m_wUserDataLen)
    {
        /* 在这里只能校验数据长度 */
        dcf_output("%s,%d failed",__func__,__LINE__);
        return DCF_ERR_PARAM;
    }

    if (((DWORD)srcQue->Total() + m_wNodeNums) > m_wConfignums)
    {
        // 数据太多，吃不下        
        dcf_output("%s,%d failed",__func__,__LINE__);
        return DCF_ERR_FULL;
    }

    void *p = NULL;
    DWORD dwRet = 0;
    while ((p = srcQue->PopNode()) != NULL)
    {
        // 添加到自己的队列
        dwRet = (bHead)?PushHead(p):PushTail(p);
        if (dwRet)
        {
            dcf_output("%s,%d failed",__func__,__LINE__);
            dcf_output("que exception(confignum:%d,usenum:%d)\r\n",m_wConfignums,m_wNodeNums);
            break;
        }
    }

    return dwRet;
}

void *CQueArray::GetNode(DWORD iPt)
{
    if (iPt >= m_wNodeNums)
    {
        return NULL;
    }

    return &m_pData[iPt*m_wOneNodeLen];
}

