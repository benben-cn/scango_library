#include "dcf_numsfixarray.h"
#include "extern_api.h"
#include "dcf_err.h"

struct PTR_NODE
{
    WORD UseFlag;
    WORD wRes;
    DWORD dwKey;
    void *pUserData;
};

CNumKeyPtrFixArray::CNumKeyPtrFixArray(WORD wNodeNum)
{
    m_wConfigNums = (wNodeNum>0)?wNodeNum:1;
    m_pData = (BYTE*)dcf_mem_malloc(m_wConfigNums*sizeof(PTR_NODE));
    memset(m_pData,0,m_wConfigNums*sizeof(PTR_NODE));
    m_wUsedNums = 0;
}

CNumKeyPtrFixArray::~CNumKeyPtrFixArray()
{
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    for(int i = 0; i < m_wConfigNums;i++)
    {
        if ((Node[i].UseFlag) && Node[i].pUserData)
        {
            ClearContent(Node[i].pUserData);
            Node[i].pUserData = NULL;
        }
    }
    m_wUsedNums = 0;
}


void CNumKeyPtrFixArray::ClearContent(void *p)
{
    p = 0;
}

DWORD CNumKeyPtrFixArray::FindNodePt(DWORD keyValue)
{
    DWORD dwPt = keyValue % m_wConfigNums;
    DWORD dwStopPt = dwPt;
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    do
    {
        if (Node[dwPt].UseFlag && (Node[dwPt].dwKey == keyValue))
        {
            return dwPt;
        }
        dwPt = (dwPt + 1) % m_wConfigNums;
    } while (dwStopPt != dwPt);

    return (DWORD)-1;
}


void* CNumKeyPtrFixArray::FindNode(DWORD keyValue)
{
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt < m_wConfigNums)
    {
        return Node[dwPt].pUserData;
    }

    return NULL;
}

DWORD CNumKeyPtrFixArray::AddNode(DWORD keyValue,void *pUserData)
{
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt < m_wConfigNums)
    {
        // 已经有该节点了，则如果用户数据是一样的，则返回OK，否则返回错误
        if(Node[dwPt].pUserData == pUserData)
        {
            return DCF_SUCCESS;
        }
        return DCF_ERR_REPEAT;
    }

    dwPt = keyValue % m_wConfigNums;
    if (!Node[dwPt].UseFlag)
    {
        // 还没有使用
        Node[dwPt].UseFlag = 1;
        Node[dwPt].dwKey = keyValue;
        Node[dwPt].pUserData = pUserData;
        m_wUsedNums++;
        return DCF_SUCCESS;
    }

    DWORD dwEmptyPt = FindEmptyNode(dwPt);
    if (dwEmptyPt >= m_wConfigNums)
    {
        // 没有空节点了
        return DCF_ERR_FULL;
    }

    Node[dwEmptyPt].UseFlag = 1;
    if (keyValue == dwPt)
    {
        // 是小于表长的节点，需要换位置        
        Node[dwEmptyPt].dwKey = Node[dwPt].dwKey;
        Node[dwEmptyPt].pUserData = Node[dwPt].pUserData;
        // 当前填写    
        Node[dwPt].dwKey = keyValue;
        Node[dwPt].pUserData = pUserData;
    }
    else
    {
        // 直接先填写在这里
        Node[dwEmptyPt].dwKey = keyValue;
        Node[dwEmptyPt].pUserData = pUserData;
    }

    m_wUsedNums++;
    return DCF_SUCCESS;
}


DWORD CNumKeyPtrFixArray::FindEmptyNode(DWORD StartPt)
{
    StartPt = StartPt % m_wConfigNums;
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    DWORD dwPt = (StartPt + 1)%m_wConfigNums;
    while ((dwPt != StartPt) && (Node[dwPt].UseFlag))
    {
        dwPt = (dwPt + 1) % m_wConfigNums;
    }

    if ((dwPt != StartPt) && (!Node[dwPt].UseFlag))
    {
        return dwPt;
    }

    return (DWORD)(-1);
}

DWORD CNumKeyPtrFixArray::ModifyNode(DWORD keyValue,void *pUserData)
{
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt >= m_wConfigNums)
    {
        return DCF_ERR_PARAM;
    }
    Node[dwPt].pUserData = pUserData;
    return DCF_SUCCESS;
}

void* CNumKeyPtrFixArray::RemoveNode(DWORD keyValue)
{
    PTR_NODE *Node = (PTR_NODE *)m_pData;
    DWORD dwPt = FindNodePt(keyValue);
    void *pUserData = NULL;
    if (dwPt < m_wConfigNums)
    {
        pUserData = Node[dwPt].pUserData;
        Node[dwPt].UseFlag = 0;
        Node[dwPt].dwKey = 0;
        Node[dwPt].pUserData = NULL;
        m_wUsedNums = (m_wUsedNums>0)?(m_wUsedNums - 1):0;
    }
    return pUserData;
}

void* CNumKeyPtrFixArray::GetFirst(WORD &wPt)
{
    PTR_NODE *Node = (PTR_NODE*)m_pData;
    for(wPt = 0;wPt < m_wConfigNums;wPt++)
    {
        if (Node[wPt].UseFlag)
        {
            return Node[wPt].pUserData;
        }
    }

    return NULL;
}
void* CNumKeyPtrFixArray::GetNext(WORD &wPt)
{
    PTR_NODE *Node = (PTR_NODE*)m_pData;
    for(++wPt;wPt < m_wConfigNums;wPt++)
    {
        if (Node[wPt].UseFlag)
        {
            return Node[wPt].pUserData;
        }
    }

    return NULL;
}


struct STRUCT_NODE_HEAD
{
    WORD UseFlag;
    WORD wRes;
    DWORD dwKey;
};


CNumKeyStructArray::CNumKeyStructArray(WORD wNodeNum,WORD wUserDataLen)
{
    wNodeNum = (wNodeNum>0) ? wNodeNum:1;
    wUserDataLen = (wUserDataLen>0) ? wUserDataLen:1;
    m_wConfigNums = wNodeNum;
    m_wUserDataLen = wUserDataLen;
    m_wUserDataOffset = ((sizeof(STRUCT_NODE_HEAD) + sizeof(DCFLPARAM) - 1)/sizeof(DCFLPARAM))*sizeof(DCFLPARAM);
    m_wOneNodeLen = m_wUserDataOffset + m_wUserDataLen;
    // 整个结构体长度指针整数倍
    m_wOneNodeLen = ((m_wOneNodeLen + sizeof(DCFLPARAM) - 1)/sizeof(DCFLPARAM))*sizeof(DCFLPARAM);
    // 下面是申请内存
    DWORD dwTotalSize = m_wOneNodeLen * m_wConfigNums;
    m_pData = (BYTE*)dcf_mem_malloc(dwTotalSize);
    memset(m_pData,0,dwTotalSize);
}

CNumKeyStructArray::~CNumKeyStructArray()
{
    BYTE *pCur = m_pData;
    if (!pCur)
    {
        return;
    }


    for(int i = 0; i < m_wConfigNums;i++)
    {
        STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD *)pCur;
        if (pHead->UseFlag)
        {
            ClearContent(pCur + m_wUserDataOffset);
        }
        pCur += m_wOneNodeLen;
    }
}

void CNumKeyStructArray::ClearContent(void *p)
{
    memset(p,0,m_wUserDataLen);
}

void CNumKeyStructArray::SaveUserData(BYTE *pDst,void *pUserData)
{
            ASSERT(pDst != NULL);
    if (pUserData)
    {
        memcpy(pDst,pUserData,m_wUserDataLen);
    }
    else
    {
        memset(pDst,0,m_wUserDataLen);
    }
}

DWORD CNumKeyStructArray::AddNode(DWORD keyValue,void *pUserData)
{
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt < m_wConfigNums)
    {
        // 已经有该节点了，则如果用户数据是一样的，则返回OK，否则返回错误
        return DCF_ERR_REPEAT;
    }

    dwPt = keyValue % m_wConfigNums;
    STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwPt);
    if (!pHead->UseFlag)
    {
        // 还没有使用
        pHead->UseFlag = 1;
        pHead->dwKey = keyValue;
        SaveUserData(GetUserDataPtr(dwPt),pUserData);
        return DCF_SUCCESS;
    }

    DWORD dwEmptyPt = FindEmptyNode(dwPt);
    if (dwEmptyPt >= m_wConfigNums)
    {
        // 没有空节点了
        return DCF_ERR_FULL;
    }

    STRUCT_NODE_HEAD *pEmptyHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwEmptyPt);
    pEmptyHead->UseFlag = 1;
    if (keyValue == dwPt)
    {
        // 是小于表长的节点，需要换位置        
        pEmptyHead->dwKey = pHead->dwKey;
        SaveUserData(GetUserDataPtr(dwEmptyPt),GetUserDataPtr(dwPt));
        // 当前填写    
        pHead->dwKey = keyValue;
        SaveUserData(GetUserDataPtr(dwPt),pUserData);
    }
    else
    {
        // 直接先填写在这里
        pEmptyHead->dwKey = keyValue;
        SaveUserData(GetUserDataPtr(dwEmptyPt),pUserData);
    }

    return DCF_SUCCESS;
}

DWORD CNumKeyStructArray::GetNode(DWORD keyValue,void *pUserData)
{
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt >= m_wConfigNums)
    {
        return DCF_ERR_PARAM;
    }
    memcpy(pUserData,GetUserDataPtr(dwPt),m_wUserDataLen);
    return DCF_SUCCESS;
}

void *CNumKeyStructArray::GetNode(DWORD keyValue)
{
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt >= m_wConfigNums)
    {
        return NULL;
    }
    return GetUserDataPtr(dwPt);
}


DWORD CNumKeyStructArray::ModifyNode(DWORD keyValue,void *pUserData)
{
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt >= m_wConfigNums)
    {
        return DCF_ERR_PARAM;
    }
    memcpy(GetUserDataPtr(dwPt),pUserData,m_wUserDataLen);
    return DCF_SUCCESS;
}

DWORD CNumKeyStructArray::RemoveNode(DWORD keyValue)
{
    DWORD dwPt = FindNodePt(keyValue);
    if (dwPt >= m_wConfigNums)
    {
        return DCF_ERR_PARAM;
    }

    STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwPt);
    pHead->UseFlag = 0;
    pHead->dwKey = 0;
    ClearContent(GetUserDataPtr(dwPt));
    return DCF_SUCCESS;
}


DWORD CNumKeyStructArray::FindNodePt(DWORD keyValue)
{
    DWORD dwPt = keyValue % m_wConfigNums;
    DWORD dwStopPt = dwPt;
    do
    {
        STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwPt);
        if (pHead->UseFlag && (pHead->dwKey == keyValue))
        {
            return dwPt;
        }
        dwPt = (dwPt + 1) % m_wConfigNums;
    } while (dwStopPt != dwPt);

    return (DWORD)-1;
}

DWORD CNumKeyStructArray::FindEmptyNode(DWORD StartPt)
{
    StartPt = StartPt % m_wConfigNums;
    STRUCT_NODE_HEAD *Node = (STRUCT_NODE_HEAD *)m_pData;
    DWORD dwPt = (StartPt + 1)%m_wConfigNums;
    STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwPt);
    while ((dwPt != StartPt) && (pHead->UseFlag))
    {
        dwPt = (dwPt + 1) % m_wConfigNums;
        pHead = (STRUCT_NODE_HEAD*)GetNodePtr(dwPt);
    }

    if ((dwPt != StartPt) && (!pHead->UseFlag))
    {
        return dwPt;
    }

    return (DWORD)(-1);
}

void* CNumKeyStructArray::GetFirst(WORD &wPt)
{
    for(wPt = 0;wPt < m_wConfigNums;wPt++)
    {
        STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(wPt);
        if (pHead->UseFlag)
        {
            return GetUserDataPtr(wPt);
        }
    }

    return NULL;
}
void* CNumKeyStructArray::GetNext(WORD &wPt)
{
    for(++wPt;wPt < m_wConfigNums;wPt++)
    {
        STRUCT_NODE_HEAD *pHead = (STRUCT_NODE_HEAD*)GetNodePtr(wPt);
        if (pHead->UseFlag)
        {
            return GetUserDataPtr(wPt);
        }
    }

    return NULL;
}


