#ifndef Dcf_sort_h
#define Dcf_sort_h
/****************************************************************
*文件范围 : 本文件提供各种排序的方法，以及查找方法
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  15:29:38
****************************************************************/
#include "dcf_def.h"

class IUserDataProc
{
public:
    virtual DWORD GetKey(BYTE *pNode) = 0;
    /*
    -1:key1<key2
    0  :key1=key2
    1  :key1>key2
    */
    virtual int CompareKey(DWORD key1,DWORD key2value);
    virtual void SwitchNode(BYTE *pNode1,BYTE *pNode2) = 0;
};

class CQuickSortTools
{
public:
    /* 3路快速排序法:是2路快速法的改进 使用的是递归算法*/
    static void quickSort3Way(BYTE* a, DWORD dwNodSize, int left, int right, IUserDataProc *Proc);
};

class CQuickFindTools
{
public:
    /* 折半查找 */
    static DWORD quickFind2Way(BYTE *pData,DWORD dwNodSize,DWORD from,DWORD to,IUserDataProc *Proc);
};
#endif

