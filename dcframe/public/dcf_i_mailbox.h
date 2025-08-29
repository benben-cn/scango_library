#ifndef Dcf_i_mailbox_h
#define Dcf_i_mailbox_h
/*
文件说明:  该模块是进程间、模块间、跨服务器间通信的核心模块
                          最核心的功能就是给应用层屏蔽寻址,如果没有这些诉求，任务可以自己创建队列，直接使用即可
创建日期:  2017.5.2
创建作者:  zjb
*/

#include "dcf_sysmng.h"
#include "dcf_msg0.h"

// DMM自身的广播消息
const DWORD MSG_DMM_BROADCAST = 0x80000000;
typedef DWORD(*MBCSENDMSG)(void*pThis,BYTE *pRccMsg,WORD rccMsgType);
/*
DCF框架的邮箱原理:
1.一个任务(线程)一个独立邮箱
2.队列挂接在任务管理组件上
*/
class IDCFMailBox
{
public:
    /* 收队列中消息 */
    virtual DWORD ReceiveMessage(DCFQUEMSG &msg) = 0;
    /* 释放内存 */
    virtual DWORD FreeMsg(void *pMsg) = 0;
    /* 申请内存 将目标地址在申请的时候就写入，这样在CRM打包等处理时就可以知道是否需要加密、字节序转换*/
    virtual DWORD MallocMsgEx(void **pMsg,WORD wBufferLen,MAIL_BOX_ADDR &dstAddr,BYTE bHost,const char *pFilename,DWORD dwLine) = 0;
#define MallocMsg(pMsg,dwBufferLen,dstAddr,bHost) MallocMsgEx(pMsg,dwBufferLen,dstAddr,bHost,__FILE__, __LINE__)
    /*  发送消息 bPriority 为优先级 0正常优先级 1高优先级*/
    virtual DWORD SendMessage(DCFQUEMSG &msg,WORD rccMsgType,BYTE bPriority = 0) = 0;
    /*  获取数据长度*/
    virtual DWORD GetMessageLen(void *pMsg,WORD &MsgLen) = 0;
    /*  设置数据长度*/
    virtual DWORD SetMsgLen(void *pMsg,WORD MsgLen) = 0;
    /* 获取缓存长度 */
    virtual DWORD GetBufLen(void *pMsg,WORD &BufLen) = 0;
    /*  获取发送消息方地址*/
    virtual DWORD GetDstAddr(void *pMsg,MAIL_BOX_ADDR &dstAddr) = 0;
    /*  获取接收方地址*/
    virtual DWORD GetSrcAddr(void *pMsg,MAIL_BOX_ADDR &recvAddr) = 0;
    /*  是否为IDCFMailBox封装的消息报文*/
    virtual bool IsDCFMailMsg(void *pMsg) = 0;
    /*  用指定的源地址转发消息,不要用自己的 */
    virtual DWORD TransMessage(MAIL_BOX_ADDR &dstAddr,MAIL_BOX_ADDR &srcAddr,DCFQUEMSG &msg,DWORD dwDataLen) = 0;
};

/*
取名为MEMC的含义:模块(插件)之间的Modul_Event_mailbox_Communication
源自:http://www.cnblogs.com/w2011/archive/2013/03/31/2992416.html
日期:2017.5.2
作者:zjb
1修改描述 : 将报文内存的申请、释放、地址的获取等均也新增到memc，因管理模块需要获取相关信息
*修改作者 : zjb
*修改时间 : 2017-05-24  11:50:28

*/

IF_VERSION (IMemc, 1, 0,1);
class  IMemc:public IDCFPlugIn
{
public:
    // 创建邮箱(参数慢慢加)
    // 参数1为返回对象
    // 参数2为自身邮箱地址
    // 参数3为消息队列的名称
    // 参数4为任务的名称
    // 参数5为本邮箱可以接收的消息ID:每个bit位可以定义一个消息
    virtual DWORD CreateMailbox(IDCFMailBox**p,const MAIL_BOX_ADDR& addr,const char *msgQuename,const char *taskname,DWORD msgQueSize,DWORD msgEvts) = 0;
    /*  获取数据长度*/
    virtual DWORD GetMessageLen(void *pMsg,WORD &MsgLen) = 0;
    /*  设置数据长度*/
    virtual DWORD SetMsgLen(void *pMsg,WORD MsgLen) = 0;
    /* 获取缓存长度 */
    virtual DWORD GetBufLen(void *pMsg,WORD &BufLen) = 0;
    /* 申请消息内存 */
    virtual DWORD MallocMsgEx(void **pMsg,WORD wBufferLen,MAIL_BOX_ADDR &srcAddr,MAIL_BOX_ADDR &dstAddr,BYTE bHost,const char *pFilename,DWORD dwLine) = 0;
#undef MallocMsg
#define MallocMsg(pMsg,dwBufferLen,srcAddr,dstAddr,bHost) MallocMsgEx(pMsg,dwBufferLen,srcAddr,dstAddr,bHost,__FILE__, __LINE__)
    /* 释放内存 */
    virtual DWORD FreeMsg(void *pMsg) = 0;
    /*  获取发送消息方地址*/
    virtual DWORD GetDstAddr(void *pMsg,MAIL_BOX_ADDR &dstAddr) = 0;
    /*  获取接收方地址*/
    virtual DWORD GetSrcAddr(void *pMsg,MAIL_BOX_ADDR &recvAddr) = 0;
    /*  给通道层的邮寄消息的接口 */
    virtual DWORD PostRccMessage(BYTE *pRccMsg) = 0;
    /* 是否为邮箱消息 */
    virtual bool IsDCFMailMsg(void *pMsg) = 0;
    /* 发送邮箱消息 */
    virtual DWORD SendMessage(DCFQUEMSG &msg,WORD rccMsgType,BYTE bPriority = 0) = 0;
    /* 是否为主机字节序 */
    virtual bool IsHostOrder(void *pMsg) = 0;
    /* 注册通信通道 */
    virtual DWORD RegisterChannel(WORD msgType,MBCSENDMSG Func,void *pParam) = 0;
};
#endif
