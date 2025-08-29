/****************************************************************
*文件范围 : 此文件是单向链表类 可以以此作为基类，可以有2个好处:
                   1.避免APP写链表，容易出错
                   2.链表指针和用户数据结构体在一起，避免出现多个指针
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-12  11:47:50
****************************************************************/

#pragma once
#include "dcf_def.h"

// 申请小内存多，系统碎片多，适合小数据量的
struct SListNode
{
    SListNode *pNext;
    SListNode() : pNext(0) {}
    SListNode *Next() { return pNext; }
};

union VALUE
{
    DWORD dwValue;
    void *pValue;
    VALUE() : dwValue(0) {}
    VALUE(DWORD _dw) : dwValue(_dw) {};
    VALUE(void *_p) : pValue(_p) {};
};

typedef bool (*COMP_FUNC)(const SListNode *pNode, VALUE para);
typedef void (*NODE_FREE_FUNC)(const SListNode *pNode);

class SingleList
{
public:
    // 头节点操作
    SListNode *PopHead();
    void PushHead(SListNode *pNode);
    // 尾节点操作
    SListNode *PopTail();
    void PushTail(SListNode *pNode);
    SListNode *FindIf(VALUE para, COMP_FUNC comp) const;
    SListNode *RemoveIf(VALUE para, COMP_FUNC comp);
protected:
    SingleList() { m_Head.pNext = 0; }
    void RemoveAll(NODE_FREE_FUNC pFreeFunc);
    SListNode *Begin() const { return m_Head.pNext; }
private:
    SListNode m_Head;
};


