#pragma once
/****************************************************************
*文件范围 : 企业服务器/移动终端和喆道服务器对接的通信协议
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  11:40:3
****************************************************************/
#include "dcf_i_rcc_dth.h"

class ISPbRpc
{
public:
    /* 发送可靠消息，外部自己释放内存 */
    virtual DWORD SendMessage(MSG_RPC &msg) = 0;
    /* 直接发送消息，无需等待超时，内存有外面释放 */
    virtual DWORD DirectSendMessage(void *pMsg,DWORD dstIP,WORD Port) = 0;
    /* 查询和bss的通讯状态 */
    virtual bool CanSendMsg() = 0;
};