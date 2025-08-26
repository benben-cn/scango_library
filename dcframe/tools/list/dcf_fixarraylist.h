
#ifndef _DCF_FIXARRAYLIST_H
#define _DCF_FIXARRAYLIST_H
/*
文件说明:  本文件为固定长度数组做循环链表的类
作            者:  zjb
日            期:  2017.5.5 14:06
*/
#include "dcf_def.h"

class CQueArray
{
public:
    CQueArray(WORD node_len,WORD node_nums);
    ~CQueArray();
    // 在最后析构函数中会调用
    virtual void ClearContent(void *p);
    WORD Total(){return m_wNodeNums;};
    WORD GetConfigNums(){return m_wConfignums;};
    WORD GetUserDataLen(){return m_wUserDataLen;};
    DWORD Pop(void *p);
    void *Pop();
    DWORD PushHead(void *p);
    DWORD PushTail(void *p);
    DWORD PushHead(CQueArray *srcQue);
    DWORD PushTail(CQueArray *srcQue);
    void *GetNode(DWORD iPt);
protected:
    DWORD Push(CQueArray *srcQue,bool bHead);
    // 隐藏不合法的初始化
    CQueArray(){};
    void *PopNode();
#ifdef TEST_KERNEL_EVENT
    void Print(const char *op);
#endif
protected:
    WORD   m_wConfignums;
    WORD   m_wNodeNums;
    // 这两个位置变量均指向有效节点位置
    WORD   m_wHeadPt;
    WORD   m_wTailPt;
    WORD   m_wUserDataLen;
    WORD   m_wOneNodeLen;
    BYTE    *m_pData;
};
#endif

