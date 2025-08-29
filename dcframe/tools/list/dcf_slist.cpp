
#include "dcf_slist.h"

void SingleList::PushHead(SListNode *pNode)
{
    pNode->pNext = m_Head.pNext;
    m_Head.pNext = pNode;
}

SListNode *SingleList::PopHead()
{
    SListNode *pRet = m_Head.pNext;
    if (pRet)
    {
        m_Head.pNext = pRet->pNext;
        pRet->pNext = 0;
    }

    return pRet;
}

void SingleList::PushTail(SListNode *pNode)
{
    SListNode *pItr = &m_Head;
    while (pItr->pNext)
    {
        pItr = pItr->pNext;
    }

    pItr->pNext = pNode;
    pNode->pNext = 0;
}

SListNode *SingleList::FindIf(VALUE para, COMP_FUNC comp) const
{
    SListNode *pRet = m_Head.pNext;
    while (pRet)
    {
        if (comp(pRet, para))
        {
            break;
        }

        pRet = pRet->pNext;
    }

    return pRet;
}

SListNode *SingleList::RemoveIf(VALUE para, COMP_FUNC comp)
{
    SListNode *pRet = 0;
    SListNode *pItr = &m_Head;
    while (pItr->pNext)
    {
        if (comp(pItr->pNext, para))
        {
            pRet = pItr->pNext;
            pItr->pNext = pRet->pNext;
            pRet->pNext = 0;
            break;
        }

        pItr = pItr->pNext;
    }

    return pRet;
}

void SingleList::RemoveAll(NODE_FREE_FUNC pFreeFunc)
{
    while (m_Head.Next())
    {
        SListNode *pNode = m_Head.Next();
        m_Head.pNext = pNode->Next();
        pFreeFunc(pNode);
    }
}



