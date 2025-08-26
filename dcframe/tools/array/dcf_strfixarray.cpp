
#include "dcf.h"
#include "dcf_strfixarray.h"
#include "dcf_err.h"
#define NODE_TAIL_STR "SFA\0"


struct HASHHEAD
{
    DWORD dwStrLen;
    DWORD dwHashKey;
    DWORD dwHashA;
    DWORD dwHashB;
};


/*
函数名：构造函数
参  数：byStrKeyLen:字符串长度
        wDataLen:   用户数据长度
        dwArrayNodeNum:配置的节点数量
作  者：zjb
时  间：2017-4-23
*/
CFixStrArray::CFixStrArray(WORD wDataLen, WORD wArrayNodeNum)
{
    // 单节点的长度   
    m_wUserDataOffset = (( sizeof(HASHHEAD) + sizeof(DCFLPARAM) - 1)/sizeof(DCFLPARAM))*sizeof(DCFLPARAM);
    m_wOneNodeLen = m_wUserDataOffset + wDataLen + sizeof(DWORD);
    // 弄个指针整数倍的位置开始，存储效率高一些
    m_wOneNodeLen = ((m_wOneNodeLen + sizeof(DCFLPARAM) - 1)/sizeof(DCFLPARAM))*sizeof(DCFLPARAM);
    DWORD dwTotalLen = m_wOneNodeLen * wArrayNodeNum;
    m_wArrayNum_Config = wArrayNodeNum;
    m_wUserDataLen = wDataLen;

    m_pData = (BYTE*)dcf_mem_malloc(dwTotalLen);
    memset(m_pData,0,dwTotalLen);
}

CFixStrArray::~CFixStrArray()
{
    BYTE *pData = (BYTE *)m_pData;
    if (!pData)
    {
        return;
    }

    for(WORD i = 0;i < m_wArrayNum_Config;i++)
    {
        BYTE *p = pData + m_wUserDataOffset;
        ClearContend(p,m_wUserDataLen);
        pData += m_wOneNodeLen;
    }
    dcf_mem_free((void*&)m_pData);
}
/*
函数名：清掉客户区数据
参  数：pData:用户区数据首地址
        wDataLen:用户数据长度
作  者：zjb
时  间：2017-4-23
*/
void CFixStrArray::ClearContend(void *pData, WORD wDataLen)
{
    memset(pData,0,wDataLen);
}

/*
函数名：获取数据区的关键字符串
参  数：pData:用户区数据首地址                    
作  者：zjb
时  间：2017-5-02
*/
const char *CFixStrArray::Getkey(void *pData)
{
    return (char *)pData;
}

/*
函数名：获取节点的用户信息
参  数：pKeyStr:用户关键字符
        wPos:输出位置
作  者：zjb
时  间：2017-4-24
*/
void* CFixStrArray::GetNode(WORD wPos)
{
    if(wPos >= m_wArrayNum_Config)
    {
        return NULL;
    }
    HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
    if (!pHashHead->dwStrLen)
    {
        return NULL;
    }

    return (BYTE*)m_pData + (wPos * m_wOneNodeLen) + m_wUserDataOffset;
}

/*
函数名：增加一个节点
参  数：pKeyStr:用户关键字符
        wPos:输出位置
作  者：zjb
时  间：2017-4-23
*/
DWORD CFixStrArray::AddNode(const char *lpszString,WORD &wPos)
{
    DWORD dwStrLen = (DWORD)strlen(lpszString) + 1;
    if (FindNode(lpszString,wPos) != NULL)
    {
        // 一定要先查找一遍，否则会出现重复节点
        return DCF_ERR_REPEAT;
    }

    const int  HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;
    DWORD  nHash = dcf_hash_string(lpszString, HASH_OFFSET);
    DWORD  nHashA = dcf_hash_string(lpszString, HASH_A);
    DWORD  nHashB = dcf_hash_string(lpszString, HASH_B);

    WORD   nHashStart = nHash % m_wArrayNum_Config;
    WORD   nHashPos = nHashStart;
    do
    {
        HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (nHashPos * m_wOneNodeLen));
        if (pHashHead->dwStrLen)
        {
            // 这个用了,指向下一个
            nHashPos = (nHashPos + 1) % m_wArrayNum_Config;
        }
        else
        {
            pHashHead->dwHashKey = nHash;
            pHashHead->dwHashA = nHashA;
            pHashHead->dwHashB = nHashB;
            pHashHead->dwStrLen = dwStrLen;
            wPos = nHashPos;
            // 填写尾部标记
            FillTailFlag(nHashPos);
            // 就它了
            return DCF_SUCCESS;
        }
    }while(nHashPos != nHashStart);

    return DCF_ERR_FULL;
}

void CFixStrArray::RemoveNode(const char *pKeyStr)
{
    WORD wPos = 0;
    if (FindNode(pKeyStr,wPos) == NULL)
    {
        // 没有该节点
        return;
    }

    // 清除用户数据
    Remove_Pt(wPos);
}
/****************************************************************
*功能描述 : 删除节点，可以将对象拷贝出去，也可以回调clearcontent
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-11  18:38:34
****************************************************************/
DWORD CFixStrArray::RemoveNode(WORD wPos,void *p)
{
    if(wPos >= m_wArrayNum_Config)
    {
        return DCF_ERR_PARAM;
    }

    HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
    void *pUserData = (BYTE*)m_pData + (wPos * m_wOneNodeLen) + m_wUserDataOffset;
    if (p)
    {
        // 拷贝出去了就不要回调来删除了
        memcpy(p,pUserData,m_wUserDataLen);
        // 直接删除
        memset(pUserData,0,m_wUserDataLen);
    }
    else
    {
        ClearContend(pUserData,m_wUserDataLen);
    }

    pHashHead->dwHashA = 0;
    pHashHead->dwStrLen = 0;
    pHashHead->dwHashB = 0;
    pHashHead->dwHashKey = 0;
    return DCF_SUCCESS;
}


void  CFixStrArray::Remove_Pt(WORD wPos)
{
    HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
    if (!pHashHead->dwStrLen)
    {
        return;
    }

    void *pUserData = (BYTE*)m_pData + (wPos * m_wOneNodeLen) + m_wUserDataOffset;
    // 释放用户信息
    ClearContend(pUserData,m_wUserDataLen);
    pHashHead->dwHashA = 0;
    pHashHead->dwStrLen = 0;
    pHashHead->dwHashB = 0;
    pHashHead->dwHashKey = 0;
}

void *CFixStrArray::GetFirst(WORD &wPos)
{
    for(wPos = 0; wPos < m_wArrayNum_Config;wPos++)
    {
        HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
        if (pHashHead->dwStrLen)
        {
            return  m_pData + (wPos * m_wOneNodeLen) + m_wUserDataOffset;
        }
    }

    return NULL;
}

void *CFixStrArray::GetNext(WORD &wPos)
{
    for(; wPos < m_wArrayNum_Config;wPos++)
    {
        HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
        if (pHashHead->dwStrLen)
        {
            return  m_pData + (wPos * m_wOneNodeLen) + m_wUserDataOffset;
        }
    }

    return NULL;
}

/*
函数名：查询节点
参  数：
作  者：zjb
时  间：2017-4-24
*/
void* CFixStrArray::FindNode(const char *lpszString, WORD &wPos)
{
    const int  HASH_OFFSET = 0, HASH_A = 1, HASH_B = 2;
    DWORD  nHash = dcf_hash_string(lpszString, HASH_OFFSET);
    DWORD  nHashA = dcf_hash_string(lpszString, HASH_A);
    DWORD  nHashB = dcf_hash_string(lpszString, HASH_B);
    WORD   nHashStart = nHash % m_wArrayNum_Config;
    WORD   nHashPos = nHashStart;
    DWORD dwStrLen = (DWORD)strlen(lpszString)+1;
    do
    {
        HASHHEAD *pHashHead = (HASHHEAD *)(m_pData + (nHashPos * m_wOneNodeLen));
        /* 如果仅仅是判断在该表中时候存在这个字符串，就比较这两个hash
        值就可以了，不用对结构体中的字符串进行比较。这样会加快运行的速度？减少hash
        表占用的空间？这种方法一般应用在什么场合？*/
        if (pHashHead->dwHashA == nHashA
            &&  pHashHead->dwHashB == nHashB
            &&  pHashHead->dwHashKey == nHash
            && dwStrLen == pHashHead->dwStrLen)
        {
            wPos = nHashPos;
            return (void*)(((BYTE*)m_pData + (nHashPos * m_wOneNodeLen)) + m_wUserDataOffset);
        }
        else
        {
            nHashPos = (nHashPos + 1) % m_wArrayNum_Config;
        }
    }while(nHashPos != nHashStart);

    return NULL;
}
/*
函数名：填写节点尾部信息
参  数：
作  者：zjb
时  间：2017-4-24
*/
void  CFixStrArray::FillTailFlag(WORD wPos)
{
    // 内部调用函数，已经完成了校验
            ASSERT(wPos < m_wArrayNum_Config);
            ASSERT(m_pData != NULL);
    memcpy(((BYTE*)m_pData + (wPos + 1) * m_wOneNodeLen) - sizeof(DWORD),NODE_TAIL_STR,sizeof(DWORD));
}

/*
函数名：增加一个HASH校验值，防止任务增删导致消息窜
参  数：
作  者：zjb
时  间：2017-4-24
*/
DWORD CFixStrArray::GetKeyHashV(WORD wPos)
{
    if (wPos >= m_wArrayNum_Config)
    {
        return 0;
    }

    HASHHEAD *pHashHead = (HASHHEAD *)((BYTE*)m_pData + (wPos * m_wOneNodeLen));
    return pHashHead->dwHashKey;
}


