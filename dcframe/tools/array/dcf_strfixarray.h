#ifndef _DCF_STRFIXARRAY_H
#define _DCF_STRFIXARRAY_H
/*
本类是字符串型(关键字)固定长度数组的类，可以用于快速查找
*/
#include "dcf_def.h"

/*
如果数据对象中有指针等，则需要从该类派生，实现ClearContend函数，否则会有内存泄漏
节点的数据组织格式:
HASHHEAD  +  用户数据区
BYTE[4]  尾部校验字符
*/

class CFixStrArray
{
public:
    CFixStrArray(WORD wDataLen,WORD wArrayLen = 32);
    ~CFixStrArray();
    // 清除节点数据的类
    virtual void ClearContend(void *pData, WORD wDataLen);
    // 获取该节点的关键字名字
    // 缺省该结构体的第一个变量就是关键字的数组
    // 如果不符合，则需要重载该函数
    virtual const char *Getkey(void *pData);
    // 获取节点数据区(数据区不包含字符串自身)
    void* GetNode(WORD wPos);
    // 查询节点
    void* FindNode(const char *pKeyStr, WORD &wPos);
    // 增加节点
    DWORD AddNode(const char *pKeyStr,WORD &wPos);
    // 删除节点
    void RemoveNode(const char *pKeyStr);
    // 删除节点
    DWORD RemoveNode(WORD wPt,void *p);
    void *GetFirst(WORD &wPos);
    void *GetNext(WORD &wPos);
    DWORD GetKeyHashV(WORD wPos);
protected:
    void  FillTailFlag(WORD wPos);
    void  Remove_Pt(WORD wPos);
protected:
    // 配置的节点数
    WORD m_wArrayNum_Config;
    WORD m_wUserDataLen;
    WORD m_wUserDataOffset;
    WORD m_wOneNodeLen;
    BYTE*m_pData;
};
#endif
