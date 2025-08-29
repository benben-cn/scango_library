
#ifndef _dcf_numsfixarray_H_
#define _dcf_numsfixarray_H_


/*
文件说明:一种以数字为HASH的、固定长度的数据结构。有如下规则:
         1.固定长度为N,KEY值可以是DWORD的值
         2.对于值小于N的节点一定在其对应位置(如果被占用，原节点会被搬迁)
           对于那种以ID自然增长分配的KEY非常有效
         3.对于大于N的节点，以搜索的方向(向右)查找空余节点
作    者:zjb
时    间:2017-5-5 23:21
*/
#include "dcf_def.h"

// 关键字是数字 用户数据是指针 总长度是固定的数组
// 用于用户数据比较大的场景，避免配置过多，浪费太多的内存
class CNumKeyPtrFixArray
{
public:
    CNumKeyPtrFixArray(WORD wNodeNum);
    ~CNumKeyPtrFixArray();
    virtual void ClearContent(void *p);
    DWORD AddNode(DWORD keyValue,void *pUserData);
    void* FindNode(DWORD keyValue);
    DWORD ModifyNode(DWORD keyValue,void *pUserData);
    void* RemoveNode(DWORD keyValue);
    WORD GetConfigNum() {return m_wConfigNums;};
    WORD GetUsedNums(){return m_wUsedNums;};
    /* 提供遍历能力 */
    void* GetFirst(WORD &wPt);
    void* GetNext(WORD &wPt);
protected:
    DWORD FindEmptyNode(DWORD StartPt);
    DWORD FindNodePt(DWORD keyValue);
    WORD  m_wConfigNums;
    WORD m_wUsedNums;
    BYTE *m_pData;
};


// 关键字是数字 用户数据结构体 总长度是固定的数组
// 用于用户数据小的常见，避免申请小内存
class CNumKeyStructArray
{
public:
    CNumKeyStructArray(WORD wNodeNum,WORD wUserDataLen);
    ~CNumKeyStructArray();
    virtual void ClearContent(void *p);
    DWORD AddNode(DWORD keyValue,void *pUserData);
    DWORD GetNode(DWORD keyValue,void *pUserData);
    void *GetNode(DWORD keyValue);
    DWORD ModifyNode(DWORD keyValue,void *pUserData);
    DWORD RemoveNode(DWORD keyValue);
    WORD   GetConfigNum(){return m_wConfigNums;};
    /* 提供遍历能力 */
    void* GetFirst(WORD &wPt);
    void* GetNext(WORD &wPt);
protected:
    void SaveUserData(BYTE *pDst,void *pUserData);
    DWORD FindEmptyNode(DWORD StartPt);
    DWORD FindNodePt(DWORD keyValue);
    BYTE *GetNodePtr(DWORD dwPt)
    {
                ASSERT(dwPt < m_wConfigNums);
        return m_pData + (dwPt * m_wOneNodeLen);
    }
    BYTE *GetUserDataPtr(DWORD dwPt)
    {
                ASSERT(dwPt < m_wConfigNums);
        return m_pData + (dwPt * m_wOneNodeLen) + m_wUserDataOffset;
    }
    WORD  m_wConfigNums;
    WORD  m_wUserDataLen;
    WORD  m_wOneNodeLen;
    WORD  m_wUserDataOffset;
    BYTE *m_pData;
};

#endif

