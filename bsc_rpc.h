#pragma once
/****************************************************************
*文件范围 : 企业服务器或者移动终端上运行的和喆道服务器通讯的系统
*设计说明 : 先通过和鉴权服务器进行认证(使用非可靠帧)，然后分配到指定的服务器进行数据交互(使用可靠帧或者非可靠帧)
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  13:42:45
****************************************************************/
#include <semaphore.h>
#include <pthread.h>
#include "bs_i_rpc.h"
#include "dcf_udp.h"
#include "dcf_dtframe.h"


#define SPBC_TASK_NAME "taskSPBCName"
const DWORD TASK_EVENT_SPBC_MSG = 1;       /* 任务消息 */
const DWORD TASK_EVENT_SPBC_TIMER = (1 << 1);  /* 模块定时器消息 */
const DWORD TASK_EVENT_SPBC_ALL = ((DWORD) -1);

/* 定时器ID */
const DWORD TIMER_ID_SPBC_1S = 1;      /* 1秒周期，用于可靠数据的检测周期 */
const DWORD TIMEOUT_SPBC_1S = 1000; /* 1秒 */
const DWORD RECV_SOCKET_MSG_TIMEOUT = 500;         // 收SOCKET时间为500ms

const DWORD TIMER_ID_SPBC_GB_LOGIN = (1 << 1);   /* 客户端登录网关的超时时间 */
const DWORD TIMEOUT_SPBC_GB_LOGIN = 10 * 1000;   /* 超时重发的周期 */

const DWORD TIMER_ID_SPBC_SV_LOGIN = (1 << 2);   /* 客户端登录服务器的超时时间 */
const DWORD TIMEOUT_SPBC_SV_LOGIN = 10 * 1000;   /* 超时重发的周期 */

const DWORD TIMER_ID_SPBC_PB_ACTIVE = (1 << 3);   /* 客户端和服务器间的保活帧 */
const DWORD TIMEOUT_SPBC_PB_ACTIVE = 1 * 30 * 1000;   /* 1分钟保活周期 */

#define MAX_ACTIVE_NOACK_TIMES 3            /* 和服务器间没有应答，判定为连接断开的次数 10分钟 */

class CBSClient : public ISPbRpc {
public:
    // 构造函数
    CBSClient(
            char *recv_addr,
            char *gateway_addr,
            char *dev_name,
            char *dev_cirtify,
            DWORD bs_id,
            DWORD dev_id,
            WORD dev_no,
            DWORD dev_opt_related,
            char *version,
            char *address
    );

    // 析构函数
    ~CBSClient();

    // 初始化模块
    virtual bool Initialize();

public:
    /* 查询服务器的状态 */
    virtual BYTE GetSeverState();

    /* 查询功能列表 */
    virtual WORD GetFuncList(WORD *funcList);

    /* 发送保活帧 */
    void server_active();

    /* 发送可靠消息，外部自己释放内存 */
    virtual DWORD SendMessage(MSG_RPC &msg);

    /* 直接发送消息，无需等待超时，内存有外面释放 */
    virtual DWORD DirectSendMessage(void *pMsg, DWORD dstIP, WORD Port);

    virtual bool CanSendMsg();

protected:
    static void *taskEntry(void *p);

    void MainLoop();

    void TimerEntry(DWORD dwTimerIDs);

    void TryRecvSocketMsg();

protected:
    /* 网关服务器间的相关操作 */
    DWORD gateway_send_cmd(WORD cmd, ...);

    void gateway_login();

    void gateway_recv_msg_entry();

    void gateway_rsp_login(CRM_CMD &Rsp);

    void gateway_rsp_logout(CRM_CMD &Rsp);

protected:
    /* 和服务器间的相关操作 */
    void server_recv_msg_entry();

    void server_recv_unsafe_entry();

    void server_send_cmd(WORD Cmd, ...);

    void server_rsp_login(CRM_CMD &Rsp);

    void server_rsp_logout(CRM_CMD &Rsp);

    void server_rsp_active(CRM_CMD &Rsp);

protected:
    // 提供给可靠帧的回调函数
    static DWORD safe_frame_send_callback(void *pMsg, WORD MsgLen, DWORD dstIP, WORD dstPort, WORD MsgType, BYTE bHost, void *pThis);

    static DWORD safe_frame_recv_callback(MSG_RPC &msg, void *pThis);

    static DWORD safe_frame_Response_callback(MSG_RPC &msg, void *pThis);

    static void  safe_frame_Progress_callback(WORD modId, WORD curBegin, WORD totalFrame);

protected:
    CDcfUDP *m_psocket;         /* 数据接收和发送的socket */
    /* SOCKET接收缓冲池 */
    BYTE *m_RecvFrameBuffer;
    /* 和服务端的可靠通讯组件 :和网关端都是非可靠通讯报文*/
    CMsgSender *m_SafeFrame_Sender;
    CMsgRecver *m_SafeFrame_Recver;

    /* 通讯IP地址部分 */
    DWORD m_dwRecvDataIP;   /* 自身数据接收IP地址 */
    DWORD m_dwGateWayIP;    /* 网关服务器IP地址 */
    DWORD m_dwServiceIP;       /* 服务进程的IP地址 */
    WORD m_wRecvDataPort;
    WORD m_wGateWayPort;
    WORD m_wServicePort;
    WORD m_tid;

    /* 状态部分 */
    BYTE m_gateway_state;   /* 和网关服务器的状态 0:初始化 1:正在登陆等待响应 2:连接正常 */
    BYTE m_server_state;       /* 和服务端的状态 0:初始化，等待和网关通讯正常 1:正在登陆 2:通讯正常*/
    BYTE m_times_noack_with_server;     /* 和服务端没有应答的次数 */
    WORD m_gate_cmdidx;
    WORD m_sbs_cmdidx;
    DWORD dwLoginTimeout;

    /* 设备属性 */
    char m_dev_name[16];   /* 设备账户(电话号码) */
    char m_dev_cirtify[16];   /* 设备认证(mac地址) */
    char m_version[16];   /* 软件版本 */
    char m_address[16];   /* IP地址 */
    WORD m_dev_no;         /* 企业内设备编号 */
    DWORD m_bsid;                    /* 企业内码id */
    DWORD m_dev_id;               /* 设备内码id */
    DWORD m_dev_opt_related;  /* 设备关联的操作员id */
    /* 密钥认证信息部分 */
    DWORD m_sbg_cirtifyid;
    DWORD m_sbs_cirtifyid;
    const char *m_com_pubkey_bsg;
    const char *m_com_pubkey_bss;
    const char *m_com_selfkey;

    pthread_t m_thread;
    bool m_stop_work;

    /* 为了减少函数间的参数传递，增加如下变量，在一次socket报文处理中有效 */
    BYTE m_hot_bhost;
    WORD m_hot_msg_len;

    /* 2017-08-23  16:55:21 增加数据库更新序号的同步 */
    DWORD m_bsdb_synidx;    /* 基本信息同步序号 */
    DWORD m_psdb_synidx;    /* 待收发货同步序号 */

    WORD m_func_length;
    WORD *m_func_list;
};
