
#ifndef _DCF_RCC_DTHELPER_H
#define _DCF_RCC_DTHELPER_H

/****************************************************************
*文件范围 : 本文件包含对通信头部等的一些校验方法，抽出来的目的:在支持多种通信协议时
                   这些校验和包头处理可以共用
*设计说明 : NA
*注意事项 : NA
*作   者 : zjb
*创建日期 : 2017-05-18  9:8:7
****************************************************************/
#include "dcf_i_rcc_dth.h"

class CRccHeaderHelper
{
public:
    CRccHeaderHelper(const CRccHeader *pHead):m_pHead(pHead){ ASSERT(pHead!=NULL);};
    ~CRccHeaderHelper(){m_pHead = NULL;};
    /* 检查是否为主机字节序 intel 系列CPU都是小字节序，其它大部分都是大字节序*/
    bool IsHostOrder()
    {
        return (m_pHead->dwMagic == get_rcc_magic_flag_host())?true:false;
    }
    /* 校验头部，注意:必须先转换成了主机字节序 */
    bool IsValidHead();
    /* 返回主机序的标记 */
    static DWORD get_rcc_magic_flag_host();
    /* 返回网络字节序的标记 */
    static DWORD get_rcc_magic_flag_net();
    /* 找帧开始的位置 */
    static WORD search_header(BYTE *pData,BYTE *pOutHeader,WORD FromPt,WORD DataLen);
    /* 填写帧头转换为网络字节序 */
    static void package_header(BYTE *pData,WORD &MsgLen,WORD msgType,BYTE bHost = 0);
    /* 解包 */
    static void unpackage_header(BYTE *pData,BYTE &bHost);
protected:
    const CRccHeader *m_pHead;
};
class CRccFrameHelper
{
public:
    /* 返回主机序的标记 */
    static WORD get_rcc_magic_flag_host();
    /* 返回网络字节序的标记 */
    static WORD get_rcc_magic_flag_net();
    static bool IsValidHead(BYTE *pFrameData);
    static BYTE*GetBufHeader(BYTE *pBuf);
    static void package_header(BYTE *pframeData,const char *key,BYTE bHost = 0);
    static void unpackage_header(BYTE *pframeData,const char *key,BYTE &bHost);
};
/* RAW MSG:CRccHeader/CRccFrame/CRM_HEAD,CRM_CMD[,...] */
class CRawMsgHelper
{
public:
    static DWORD InitMsgBuffer(BYTE *pMsg,BYTE ctrl,WORD TotalLen,WORD DataType = 0,DWORD cirtifyid = 0);
    static DWORD AddToPacket(BYTE *pMsg,WORD TotalLen,WORD &Offset,CRM_CMD &cmd,BYTE bHost = 0);
    static DWORD GetFromPacket(BYTE *pMsg,WORD TotalLen,WORD &Offset,CRM_CMD &cmd,BYTE bHost = 0);
    static void PacketFrame(BYTE *pMsg,WORD &TotalLen,const char *key,BYTE bHost = 0);
    static void UnpacketFrame(BYTE *pMsg,const char *key,BYTE bHost = 0);
};

class CMBMsgHelper
{
public:
    static void cvt_hton(BYTE *pMbHeader);
    static void cvt_ntoh(BYTE *pMbHeader);
    static DWORD Packet(BYTE *pRccPtr,const char *key);
    static void Unpacket(BYTE *pRccPtr,const char *key);
    static BYTE GetHostFlag(BYTE *pRccPtr);
    static DWORD GetDstAddr(BYTE *pRccPtr,MAIL_BOX_ADDR &dstAddr);
};

/* 2017-06-23  13:48:32 */
/* 封装专门针对CRM的报文 */
class CCRMMsgHelper
{
public:
    static DWORD AddToPacket(BYTE *pMsg,WORD MsgLen,WORD &Offset,CRM_CMD &cmd,BYTE bHost = 0);
    static DWORD GetFromPacket(BYTE *pMsg,WORD MsgLen,WORD &Offset,CRM_CMD &cmd,BYTE bHost = 0);
    static void PacketFrame(BYTE *pMsg,WORD buf_len,WORD &MsgLen,const char *key,BYTE bHost = 0);
    static void UnpacketFrame(BYTE *pMsg,const char *key,WORD MsgLen,BYTE bHost = 0);
};

class CBDTMsgHelper
{
public:
    static void InitMsgBuffer(BYTE *pMsg,WORD ctrl,WORD cmd,DWORD buf_len);
    static void PacketFrame(BYTE *pMsg,DWORD buf_len,DWORD &MsgLen,const char *key,BYTE bHost = 0);
    static void UnpacketFrame(BYTE *pMsg,const char *key,DWORD MsgLen,BYTE bHost = 0);
    static void GetFromPacket(BYTE *pMsg,BDT_TLV &tlv);
};

#endif
