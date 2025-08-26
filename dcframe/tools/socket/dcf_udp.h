#ifndef  _DCF_UDP_H
#define  _DCF_UDP_H

/****************************************************************
*文件范围 : 这个文件是最底层的UDP通信类
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-17  21:30:39
****************************************************************/
#include "dcf_def.h"
#include "config.h"
#include "dcf_i_rcc_dth.h"


#if _SW_DCF_LINUX
#include<errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET_ERROR  -1
#define INVALID_SOCKET  0xFFFFFFFF
#endif

class CDcfUDP
{
public:
    CDcfUDP();
    ~CDcfUDP();
    DWORD Initialize(DWORD recvIP,WORD recvPort);
    // 读取一帧数据(调用方会一直读取，直到超时)
    DWORD RecvFrame(void *pBuffer,DWORD buflen,DWORD &dataLen,DWORD &dataIP,WORD &dataPort,DWORD &timeout);
    // 发送一帧数据
    DWORD SendFrame(void *pData,DWORD len,DWORD toIP,WORD toPort);
protected:
    DWORD PreInit();
    // 数据填充到成员变量中
    DWORD ReadRawData(DWORD timeout);
    DWORD SearchFrame(BYTE *pBuffer,DWORD buflen,DWORD &dataLen);
    void Close();
    DWORD CheckHeader(CRccHeader *pHead);
protected:
    // 读数据到缓存
    BYTE            m_chRecvBuf[MAX_SOCKBUF_LEN];   /* 最大的接收缓存 */
    DWORD           m_Buff_IP;             /* 缓存中数据的IP */
    WORD            m_Buff_Port;          /* 缓存中数据的端口 */
    WORD            m_Buff_ReadLen;   /* 缓存中读到的总长度 */
    WORD            m_Buff_ProcPt;   /*  缓存中数据已经处理到的位置 */
#if _SW_DCF_LINUX
    int            m_socket;
#else
    SOCKET          m_socket;
#endif

    /* 增加一些统计能力 */
    DWORD          m_RecvTotalFrames;   /* 收报文总数 */
    DWORD          m_RecvOkFrames;       /* 正确报文总数 */
    DWORD          m_RecvErrFrames;      /*  丢弃报文总数 */
    DWORD          m_SendTotalFrames;  /*  发送报文总数 */
    DWORD          m_SendOkFrames;       /* 正确报文总数 */
    DWORD          m_SendErrFrames;      /*  丢弃报文总数 */
};
#endif

