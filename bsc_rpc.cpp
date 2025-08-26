/****************************************************************
*文件范围 : 中海塆服务客户端通信处理层
*设计说明 : 分为2部分:
                   1.和网关鉴权登录部分
                      a)V2.0版本:登陆校验企业id,设备id
                      b)V2.x版本:私钥变更
                      鉴权成功之后，网关生成一个DWORD的随机码给服务进程(企业id，设备id，随机码，客户IP地址，端口等五元组)和客户端(服务端IP地址，端口，随机码)
                   2.和喆道服务端数据交互部分
                      a)客户端登录:使用企业id，设备id，网关分配的DWORD随机码登录服务端
                      b)服务端校验:三者信息一致
                      c)服务端生成DWORD的认证码返回给客户端，并记录其IP&端口
                      d)客户端通过认证码，IP地址，端口进行交互
                   特别注意:端口是否能用来判断，还需要实际测试
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  15:47:20
****************************************************************/
#include "dcframe_proc.h"
#include "bsc_rpc.h"
#include "extern_api.h"
#include "dcf_modid.h"
#include "dcf_err.h"
#include "dcf_pkghelper.h"
#include "dcf_rcc_dthelper.h"
#include "dcf_cmdid.h"
#include "dcf_time.h"
#include "dcf_string.h"
#include "bs_def.h"
#include "zhw_pdalib.h"

extern CBSClient *g_bsc_rpc;

/****************************************************************
*功能描述 : 任务主循环函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  17:3:34
****************************************************************/
void CBSClient::MainLoop() {
    gateway_login();

    clock_t pre_work_1s, pre_sleep_1s;
    clock_t pre_work_1min, pre_sleep_1min;
    pre_work_1s = dcf_time_get_cur_clock();
    pre_sleep_1s = pre_work_1s;
    pre_work_1min = pre_work_1s;
    pre_sleep_1min = pre_work_1s;

    for (;;) {
        if (m_stop_work) {
            // 要求退出
            break;
        }

        DWORD dwTimerIDs = 0;
        if (dcf_time_exact_work_and_sleep(pre_work_1s, pre_sleep_1s, TIMEOUT_SPBC_1S, 0)) {
            // 1秒定时器
            dwTimerIDs |= TIMER_ID_SPBC_1S;
            //dcf_output("[%s]TIMER_ID_SPBC_1S is ok!\r\n", dcf_get_time_string_unsafe());
            if (m_gateway_state == 0) {

            } else if (m_gateway_state == 1) {
                dwLoginTimeout += TIMEOUT_SPBC_1S;
                if (dwLoginTimeout >= TIMEOUT_SPBC_GB_LOGIN) {
                    dwTimerIDs |= TIMER_ID_SPBC_GB_LOGIN;
                }
            } else if (m_gateway_state == 2) {
                if (m_server_state == 0) {

                } else if (m_server_state == 1) {
                    dwLoginTimeout += TIMEOUT_SPBC_1S;
                    if (dwLoginTimeout >= TIMEOUT_SPBC_SV_LOGIN) {
                        dwTimerIDs |= TIMER_ID_SPBC_SV_LOGIN;
                    }
                }
            }
        }

        if (dcf_time_exact_work_and_sleep(pre_work_1min, pre_sleep_1min, TIMEOUT_SPBC_PB_ACTIVE, 10)) {
            // 1分钟定时器
            dwTimerIDs |= TIMER_ID_SPBC_PB_ACTIVE;
            //dcf_output("[%s]TIMER_ID_SPBC_PB_ACTIVE is ok!\r\n", dcf_get_time_string_unsafe());
        }

        if (dwTimerIDs) {
            // 控制消息
            //dcf_output("[%s]dwTimerIDs:%u\r\n", dcf_get_time_string_unsafe(), dwTimerIDs);
            TimerEntry(dwTimerIDs);
        }

        if (m_psocket) {
            //dcf_output("[%s]TryRecvSocketMsg is start!\r\n", dcf_get_time_string_unsafe());
            TryRecvSocketMsg();
            //dcf_output("[%s]TryRecvSocketMsg is end!\r\n", dcf_get_time_string_unsafe());
        }
    }

    dcf_output("[%s]sbc task auto quit!\r\n", dcf_get_time_string_unsafe());
    pthread_exit(NULL);
}

/* 查询服务器的状态 */
BYTE CBSClient::GetSeverState() {

    //dcf_output("[%d]sbc m_server_state\r\n",m_server_state);
    return m_server_state;
}

WORD CBSClient::GetFuncList(WORD *funcList) {
    memcpy(funcList, m_func_list, m_func_length * sizeof(WORD));
    return m_func_length;
}

/****************************************************************
*功能描述 : 发送可靠消息
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  17:16:1
****************************************************************/
DWORD CBSClient::SendMessage(MSG_RPC &msg) {
    if (!m_SafeFrame_Sender) {
        return DCF_ERR_SYS_NONE_INIT;
    }

    /* 数据加密 */
    ASSERT((msg.MsgLen % 16) == 0);
    DWORD dwRet = dcf_tools_ces128_encrypt(msg.pMsg, msg.MsgLen, msg.pMsg, m_com_selfkey);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSS_MOD_ID_RPC, "encrypt");
    msg.MsgIP = m_dwServiceIP;
    msg.MsgPort = m_wServicePort;
    return m_SafeFrame_Sender->SendMsg(msg);
}

DWORD CBSClient::DirectSendMessage(void *pMsg, DWORD dstIP, WORD Port) {
    return 0;
}

/****************************************************************
*功能描述 : 判断是否可以发送消息了
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-08-05  13:44:5
****************************************************************/
bool CBSClient::CanSendMsg() {
    return m_server_state == 2;
}

/****************************************************************
*功能描述 : 定时器消息入口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-29  17:54:48
****************************************************************/
void CBSClient::TimerEntry(DWORD dwTimerIDs) {
    char chbuffer[32];
    if (dwTimerIDs & TIMER_ID_SPBC_GB_LOGIN) {
        // 登陆超时
        dcf_output("[%s]bsc login bsg timeout,now retry...\r\n", dcf_get_time_string(chbuffer));
        gateway_login();
    }

    if ((dwTimerIDs & TIMER_ID_SPBC_PB_ACTIVE) && (m_server_state >= 2)) {
        m_times_noack_with_server++;

        if (m_times_noack_with_server >= MAX_ACTIVE_NOACK_TIMES) {
            // 设置为状态1，会启动登陆超时定时器
            dcf_output("[%s]bsc server active timeout,try to login...\r\n", dcf_get_time_string(chbuffer));
            /* 和服务器的连接断开之后，直接从登录网关开始 */
            gateway_login();
        } else {
            // 保活帧的发送
            dcf_output("[%s]bsc check server active...\r\n",dcf_get_time_string(chbuffer));
            server_active();
        }
    }

    if (dwTimerIDs & TIMER_ID_SPBC_1S) {
        if (m_SafeFrame_Sender) {
            //dcf_output("[%s]m_SafeFrame_Sender TIMER_ID_SPBC_1S is start\r\n", dcf_get_time_string(chbuffer));
            m_SafeFrame_Sender->ProcTimer();
            //dcf_output("[%s]m_SafeFrame_Sender TIMER_ID_SPBC_1S is end\r\n", dcf_get_time_string(chbuffer));
        }
        if (m_SafeFrame_Recver) {
            //dcf_output("[%s]m_SafeFrame_Recver TIMER_ID_SPBC_1S is start\r\n", dcf_get_time_string(chbuffer));
            m_SafeFrame_Recver->ProcTimer();
            //dcf_output("[%s]m_SafeFrame_Recver TIMER_ID_SPBC_1S is end\r\n", dcf_get_time_string(chbuffer));
        }
    }

    if (dwTimerIDs & TIMER_ID_SPBC_SV_LOGIN) {
        /* 登录服务器超时 */
        dcf_output("[%s]bsc login bss timeout,now retry...\r\n", dcf_get_time_string(chbuffer));
        gateway_login();
    }
}

/****************************************************************
*功能描述 : 尝试读socket消息
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  18:15:2
****************************************************************/
void CBSClient::TryRecvSocketMsg() {
    ASSERT(m_psocket != NULL);
    DWORD dwWaitTimeOut = RECV_SOCKET_MSG_TIMEOUT;
    DWORD dataIP;
    WORD dataPort;
    DWORD dataLen;

    for (;;) {
        if (m_stop_work) {
            // 要求退出
            break;
        }

        DWORD dwRet = m_psocket->RecvFrame(m_RecvFrameBuffer, RCC_SIZE_TOTAL, dataLen, dataIP, dataPort, dwWaitTimeOut);
        if (dwRet) {
            // 直到取完socket数据
            break;
        }

        if (!dataLen) {
            continue;
        }

        m_hot_msg_len = (WORD) dataLen;
        // CRccHeader层从soket中出来就已经做了转换
        CRccHeader *pHeader = (CRccHeader *) m_RecvFrameBuffer;
        // 先看是否为确认帧
        if ((pHeader->Msgtype != RCC_MSG_TYPE_RAW_BSS_CRM) && (pHeader->Msgtype != RCC_MSG_TYPE_RAW_BSS_BIGDATA)
            && (pHeader->Msgtype != RCC_MSG_TYPE_RAW_BSG_CRM)
            && (pHeader->Msgtype != RCC_MSG_TYPE_CHANNEL)) {
            // 串网了?
            dcf_output("[%s]bsc recv exception typeframe(%d)?\r\n", dcf_get_time_string_unsafe(), pHeader->Msgtype);
            continue;
        }

        if ((pHeader->Msgtype == RCC_MSG_TYPE_RAW_BSG_CRM)
            && (dataIP == m_dwGateWayIP) && (dataPort == m_wGateWayPort)) {
            CRccFrame *pFrame = (CRccFrame * )(m_RecvFrameBuffer + RCC_SIZE_HEADER);
            CRccFrameHelper::unpackage_header((BYTE *) pFrame, m_com_pubkey_bsg, m_hot_bhost);
            if (!CRccFrameHelper::IsValidHead((BYTE *) pFrame)) {
                dcf_output("[%s]bsc failed to decrypt frame!\r\n", dcf_get_time_string_unsafe());
                continue;
            }
            dcf_output("[%d]TryRecvSocketMsg-gateway_recv_msg_entry-1!\r\n", pHeader->Msgtype);
            gateway_recv_msg_entry();
        } else if ((dataIP == m_dwServiceIP) && (dataPort == m_wServicePort)) {
            CRccFrame *pFrame = (CRccFrame * )(m_RecvFrameBuffer + RCC_SIZE_HEADER);
            CRccFrameHelper::unpackage_header((BYTE *) pFrame, m_com_pubkey_bss, m_hot_bhost);
            if (!CRccFrameHelper::IsValidHead((BYTE *) pFrame)) {
                dcf_output("[%s]bsc failed to decrypt frame!\r\n", dcf_get_time_string_unsafe());
                continue;
            }
            dcf_output("[%s][%d]TryRecvSocketMsg-server_recv_msg_entry-2!\r\n", dcf_get_time_string_unsafe(), pHeader->Msgtype);
            server_recv_msg_entry();
        } else {
            dcf_output("[%s]bsc recv error ip(ip:0x%08x,port:%d) data\r\n", ntohl(dataIP), ntohs(dataPort));
        }
    }
}

/****************************************************************
*功能描述 : 发送给网关命令
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-07-02  10:42:57
****************************************************************/
DWORD CBSClient::gateway_send_cmd(WORD Cmd, ...) {
    BYTE chPara[128] = {0};   /* 特别注意: 参数区不能太大，也不能太小，恰当就好*/
    const DWORD dwTotalLen = RCC_SIZE_RAW_ONETLV_MIN + sizeof(chPara);
    BYTE chMsgBuffer[dwTotalLen] = {0};  /* 参数区 */
    CRawMsgHelper::InitMsgBuffer(chMsgBuffer, RCC_CTRL_FLAG_CMD_NOSURE, dwTotalLen, 0, m_sbg_cirtifyid);

    // 构建命令包
    CRM_CMD cmd;
    cmd.wCmdIdx = ++m_gate_cmdidx;
    cmd.wCmd = Cmd;
    cmd.pbyPara = chPara;
    cmd.wParaLen = sizeof(chPara);
    cmd.wCtrlCmd = CRM_CTRL_CMD_NEEDREQ;

    // 1.先构建参数部分
    WORD Offset = 0;
    DWORD dwRet = 0;
    va_list marker;
    va_start(marker, Cmd);
    dwRet = dcf_para_fmt::PacketParam(cmd, Offset, marker);
    va_end(marker);
    ASSERT(cmd.wParaLen < sizeof(chPara));
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "AddToPacket");

    // 2.再构建命令包
    Offset = 0;
    dwRet = CRawMsgHelper::AddToPacket(chMsgBuffer, dwTotalLen, Offset, cmd, 1);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "AddToPacket");
    // 3.封包尾并加密
    WORD MsgLen = 0;
    if (Cmd == MOD_CMD_BSG_LOGIN) {
        // 登录命令用公钥加密
        CRawMsgHelper::PacketFrame(chMsgBuffer, MsgLen, m_com_pubkey_bsg, 1);
    } else {
        // 其它命令用私钥加密
        CRawMsgHelper::PacketFrame(chMsgBuffer, MsgLen, m_com_selfkey, 1);
    }

    // 4.填写frame帧信息
    CRccHeader *pHead = (CRccHeader *) chMsgBuffer;
    CRccFrame *pFrame = (CRccFrame *) CRccFrameHelper::GetBufHeader(chMsgBuffer);
    pFrame->CurFrames = 1;
    pFrame->send_times = 1;
    pFrame->wMsg1 = Cmd;      /* 填写命令字 */
    pFrame->SequenceNumber = 0;

    if (Cmd == MOD_CMD_BSG_LOGIN) {
        /* 登录命令，填写设备id */
        pFrame->cirtifyid = m_dev_id;
    }

    /* 5.frame帧封帧 */
    pHead->Secret_Ver = RCC_CRY_VER_FRAME;
    CRccFrameHelper::package_header((BYTE *) pFrame, m_com_pubkey_bsg, 1);

    // 6.写rcc层
    CRccHeaderHelper::package_header((BYTE *) chMsgBuffer, MsgLen, RCC_MSG_TYPE_RAW_BSG_CRM, 1);
    // 7.发送
    return m_psocket->SendFrame(chMsgBuffer, MsgLen, m_dwGateWayIP, m_wGateWayPort);
}

/****************************************************************
*功能描述 : 登陆网关
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  17:58:59
****************************************************************/
void CBSClient::gateway_login() {
    m_server_state = 0;
    m_gateway_state = 0;
    dwLoginTimeout = 0;
    m_times_noack_with_server = 0;
    BYTE data[16];


    // 对公钥用私钥加密
//    dcf_tools_ces128_encrypt((BYTE *) m_com_pubkey_bsg, 16, data, m_com_selfkey);
    dcf_tools_ces128_encrypt((BYTE *) m_com_selfkey, 16, data, m_com_pubkey_bsg);
    gateway_send_cmd(MOD_CMD_BSG_LOGIN, m_dev_name, m_dev_cirtify, data, m_version, m_address);
    m_gateway_state = 1;
}

/****************************************************************
*功能描述 : 处理从gateway的登录鉴权响应
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-04  13:44:20
****************************************************************/
void CBSClient::gateway_rsp_login(CRM_CMD &Rsp) {
    dcf_para_out out(Rsp.pbyPara, Rsp.wParaLen);
    DWORD dwIP, bs_id, bs_sub_id, dev_id, verify_code, opt_id;
    WORD wPort, wError, dev_no;
    out >> wError >> dwIP >> bs_id >> bs_sub_id >> dev_id >> verify_code >> wPort >> dev_no >> opt_id;
    if (out.have_error() || wError) {
        m_server_state = 3;
        dcf_output("[%s]bsc recv error data param!wError[%d]\r\n", dcf_get_time_string_unsafe(), wError);
        zhw_bs_quit();
        loginError(wError);
        return;
    }

    char chbuffer[16];
    // 添加一个打印
    //dcf_output("[%s]bsc recv bsg response,then login bss(IP:%s,Port:%d)!\r\n", dcf_get_time_string_unsafe(),
    //           dcf_strtools::IPtoStr(ntohl(dwIP), chbuffer), ntohs(wPort));

    // 登录bss
    m_dwServiceIP = dwIP;
    m_wServicePort = wPort;
    m_sbg_cirtifyid = verify_code;  /* 目前没有什么其它交互，先保存这个 */
    m_bsid = bs_id;
    m_dev_id = dev_id;
    m_dev_no = dev_no;
    m_dev_opt_related = opt_id;

    dcf_id_set(BS_ID_BS, bs_id);
    dcf_id_set(BS_ID_DEV, dev_id);
    dcf_id_set(BS_ID_DEV_SN, dev_no);
    dcf_id_set(BS_ID_BS_SUB, bs_sub_id);

    m_gateway_state = 2;
    m_server_state = 1;
    dwLoginTimeout = 0;
    dcf_time_sleep(1000);   /* 因为bsg和bss通信走mail,会慢一点，因此这里需要等待 */
    server_send_cmd(MOD_CMD_BSS_LOGIN, m_bsid, verify_code);
}


/****************************************************************
*功能描述 : 退出登录 ,直接退出
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     yjh
*创建日期 : 2017-07-04  13:44:20
****************************************************************/
void CBSClient::gateway_rsp_logout(CRM_CMD &Rsp) {
    dcf_output("-----------logout\r\n");
    zhw_bs_recv_logout();
}

/****************************************************************
*功能描述 : 网关消息的总入口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  18:26:5
****************************************************************/
void CBSClient::gateway_recv_msg_entry() {
    CRccHeader *pHeader = (CRccHeader *) m_RecvFrameBuffer;
    CRccFrame *pFrame = (CRccFrame * )(m_RecvFrameBuffer + RCC_SIZE_HEADER);
    // 1.解密和转换内容帧
//    CRawMsgHelper::UnpacketFrame(m_RecvFrameBuffer, m_com_selfkey, m_hot_bhost);
    CRawMsgHelper::UnpacketFrame(m_RecvFrameBuffer, m_com_selfkey, m_hot_bhost);

    // 2.获取CRM命令
    WORD Offset = 0;
    CRM_CMD Rsp;
    DWORD dwRet = CRawMsgHelper::GetFromPacket(m_RecvFrameBuffer, m_hot_msg_len, Offset, Rsp, m_hot_bhost);
    if (dwRet) {
        dcf_output("[%s]bsc recv error data from gate dwRet:[%d]\r\n", dcf_get_time_string_unsafe(), dwRet);
//        return;
    }

    if (pFrame->IsCmdRspFrame()) {
        switch (Rsp.wCmd) {
            case MOD_CMD_BSG_LOGIN:
                m_stop_work = false;
                dcf_output("gateway_recv_msg_entry-login\r\n");
                gateway_rsp_login(Rsp);
                break;
            case MOD_CMD_BSG_LOGOUT:
                m_stop_work = true;
                dcf_output("gateway_recv_mod_cmd_bsg_logout\r\n");
                gateway_rsp_logout(Rsp);
                break;
            case MOD_CMD_BSG_LOGIN_ERR:
                m_stop_work = true;
                dcf_output("gateway_recv_mod_cmd_bsg_login_err\r\n");
                gateway_rsp_login(Rsp);
                break;
            default:
                dcf_output("[%s]bsc recv error rsp from gate(0x%x)\r\n", dcf_get_time_string_unsafe(), Rsp.wCmd);
                break;
        }
    } else {

    }
}

/****************************************************************
*功能描述 : 给服务器发送命令
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-04  13:54:42
****************************************************************/
void CBSClient::server_send_cmd(WORD Cmd, ...) {
    BYTE chPara[128] = {0};   /* 特别注意: 参数区不能太大，也不能太小，恰当就好*/
    const DWORD dwTotalLen = RCC_SIZE_RAW_ONETLV_MIN + sizeof(chPara);
    BYTE chMsgBuffer[dwTotalLen] = {0};  /* 参数区 */
    CRawMsgHelper::InitMsgBuffer(chMsgBuffer, RCC_CTRL_FLAG_CMD_NOSURE, dwTotalLen, 0, m_sbs_cirtifyid);

    // 构建命令包
    CRM_CMD cmd;
    cmd.wCmdIdx = ++m_sbs_cmdidx;
    cmd.wCmd = Cmd;
    cmd.pbyPara = chPara;
    cmd.wParaLen = sizeof(chPara);
    cmd.wCtrlCmd = CRM_CTRL_CMD_NEEDREQ;

    // 1.先构建参数部分
    WORD Offset = 0;
    DWORD dwRet = 0;
    va_list marker;
    va_start(marker, Cmd);
    dwRet = dcf_para_fmt::PacketParam(cmd, Offset, marker);
    va_end(marker);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "AddToPacket");

    // 2.再构建命令包
    dwRet = CRawMsgHelper::AddToPacket(chMsgBuffer, dwTotalLen, Offset, cmd, 1);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "AddToPacket");
    // 3.封包尾并加密
    WORD MsgLen = 0;
    CRawMsgHelper::PacketFrame(chMsgBuffer, MsgLen, m_com_selfkey, 1);

    // 4.填写frame帧信息
    CRccHeader *pHead = (CRccHeader *) chMsgBuffer;
    CRccFrame *pFrame = (CRccFrame *) CRccFrameHelper::GetBufHeader(chMsgBuffer);
    pFrame->CurFrames = 1;
    pFrame->send_times = 1;
    pFrame->wMsg1 = Cmd;      /* 填写命令字 */
    pFrame->SequenceNumber = 0;

    if (Cmd == MOD_CMD_BSS_LOGIN) {
        /* 登录命令，填写设备id */
        pFrame->cirtifyid = m_dev_id;
    }

    /* 5.frame帧封帧 */
    pHead->Secret_Ver = RCC_CRY_VER_FRAME;
    CRccFrameHelper::package_header((BYTE *) pFrame, m_com_pubkey_bss, 1);

    // 6.写rcc层
    CRccHeaderHelper::package_header((BYTE *) chMsgBuffer, MsgLen, RCC_MSG_TYPE_RAW_BSS_CRM, 1);
    // 7.发送
    dwRet = m_psocket->SendFrame(chMsgBuffer, MsgLen, m_dwServiceIP, m_wServicePort);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "SendFrame");

}

/****************************************************************
*功能描述 : 服务器保活
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  18:9:57
****************************************************************/
void CBSClient::server_active() {
    dcf_output("[%s] bs发送保活帧", dcf_get_time_string_unsafe());
    server_send_cmd(MOD_CMD_BSS_ACTIVE, m_bsid);
}

/****************************************************************
*功能描述 : 服务器端socket报文的处理总入口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  18:28:7
****************************************************************/
void CBSClient::server_recv_msg_entry() {
    CRccHeader *pHeader = (CRccHeader *) m_RecvFrameBuffer;
    CRccFrame *pFrame = (CRccFrame * )(m_RecvFrameBuffer + RCC_SIZE_HEADER);

//    dcf_output("[%d] 收到socket报文的处理总入口", pHeader->wMsgLen);

    if (pFrame->ctrl_get(RCC_CTRL_BIT_SURE)) {
        // 是可靠性帧
        if ((!m_SafeFrame_Sender) || (!m_SafeFrame_Recver)) {
            // 必须是先登录后创建了可靠帧
            dcf_output("[%s]bsc has not login sever?\r\n", dcf_get_time_string_unsafe());
            return;
        }

        if (pFrame->ctrl_get(RCC_CTRL_BIT_CHANNEL)) {
            // 是命令的应答
            //dcf_output("[%s]m_SafeFrame_Recver->ProcRecvMsg is start\r\n", dcf_get_time_string_unsafe());
            m_SafeFrame_Recver->ProcRecvMsg(m_RecvFrameBuffer, m_hot_msg_len, m_hot_bhost);
            //dcf_output("[%s]m_SafeFrame_Recver->ProcRecvMsg is end\r\n", dcf_get_time_string_unsafe());
        } else {
            // 是接收的消息
            //dcf_output("[%s]m_SafeFrame_Sender->ProcRecvMsg is start\r\n", dcf_get_time_string_unsafe());
            m_SafeFrame_Sender->ProcRecvMsg(m_RecvFrameBuffer, m_hot_msg_len, m_hot_bhost);
            //dcf_output("[%s]m_SafeFrame_Sender->ProcRecvMsg is end\r\n", dcf_get_time_string_unsafe());
        }

        return;
    }
    /* 下面是非确认消息 */
    server_recv_unsafe_entry();
}

/****************************************************************
*功能描述 :
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-29  18:34:22
****************************************************************/
void CBSClient::server_recv_unsafe_entry() {
    CRccHeader *pHeader = (CRccHeader *) m_RecvFrameBuffer;
    CRccFrame *pFrame = (CRccFrame * )(m_RecvFrameBuffer + RCC_SIZE_HEADER);
    // 1.解密和转换内容帧
    CRawMsgHelper::UnpacketFrame(m_RecvFrameBuffer, m_com_selfkey, m_hot_bhost);

    if (pFrame->wMsg1 != INFO_TYPE_BSS_CHANNEL) {
        dcf_output("[%s]bsc recv error msg1(0x%x) from bss\r\n", dcf_get_time_string_unsafe(), pFrame->wMsg1);
    }

    // 2.获取CRM命令
    WORD Offset = 0;
    CRM_CMD Rsp;
    DWORD dwRet = CRawMsgHelper::GetFromPacket(m_RecvFrameBuffer, m_hot_msg_len, Offset, Rsp, m_hot_bhost);
    if (dwRet) {
        dcf_output("[%s]bsc recv error data from bss\r\n", dcf_get_time_string_unsafe());
        return;
    }

    if (Rsp.wCmd != MOD_CMD_BSS_LOGIN) {
        if (pFrame->cirtifyid != m_sbs_cirtifyid) {
            dcf_output("[%s]bsc recv error cirtifyid from bss(%d,%d)\r\n", dcf_get_time_string_unsafe(), pFrame->cirtifyid, m_sbs_cirtifyid);
            return;
        }
    }

    if (pFrame->IsCmdRspFrame()) {
        switch (Rsp.wCmd) {
            case MOD_CMD_BSS_LOGIN:
                dcf_output("server_recv_unsafe_entry-login\r\n");
                server_rsp_login(Rsp);
                break;
            case MOD_CMD_BSS_LOGOUT:
                server_rsp_logout(Rsp);
                break;
            case MOD_CMD_BSS_ACTIVE:
                server_rsp_active(Rsp);
                break;
            default:
                dcf_output("[%s]bsc recv error rsp from bss(0x%x)\r\n", dcf_get_time_string_unsafe(), Rsp.wCmd);
                break;
        }
    } else {

    }

}

/****************************************************************
*功能描述 : 收到server端的 强制下线
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-08-05  14:16:40
****************************************************************/
void CBSClient::server_rsp_logout(CRM_CMD &Rsp) {
    m_gateway_state = 1;
    m_server_state = 0;
    dwLoginTimeout = 9000;
    dcf_output("[%s]dev logout wCmd[0x%x]\r\n", dcf_get_time_string_unsafe(), Rsp.wCmd);
}

/****************************************************************
*功能描述 : 收到server端的应答
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-08-05  14:16:40
****************************************************************/
void CBSClient::server_rsp_login(CRM_CMD &Rsp) {
    dcf_para_out out(Rsp.pbyPara, Rsp.wParaLen);

    dcf_output("[%d] 收到的数据长度", Rsp.wParaLen);

    WORD wError, func_bytes_length;
    DWORD verify_code, bsdb_syn, psdb_syn;
    out >> wError >> verify_code >> bsdb_syn >> psdb_syn >> func_bytes_length;
    if (out.have_error() || wError) {
        m_server_state = 3;
        dcf_output("[%s]bsc recv error data param!\r\n", dcf_get_time_string_unsafe());
        return;
    }

    // 展开功能列表
    m_func_length = func_bytes_length / sizeof(func_bytes_length);
    m_func_list = new WORD[m_func_length];
    for (int i = 0; i < m_func_length && !out.have_error(); i++) {
        out >> m_func_list[i];
        // 转换成主机字节序
        m_func_list[i] = ntohs(m_func_list[i]);
    }

    // 登录bss
    m_sbs_cirtifyid = verify_code;
    m_server_state = 2;
    dwLoginTimeout = 0;

    if (m_SafeFrame_Sender) {
        m_SafeFrame_Sender->ClearAll();
        delete m_SafeFrame_Sender;
        m_SafeFrame_Sender = NULL;
    }
    if (m_SafeFrame_Recver) {
        m_SafeFrame_Recver->ClearAll();
        delete m_SafeFrame_Recver;
        m_SafeFrame_Recver = NULL;
    }

    // 创建可靠帧管理对象
    COMM_CHANNEL channel = {0};
    channel.cirtifyid = m_sbs_cirtifyid;
    channel.dstIP = m_dwServiceIP;
    channel.dstPort = m_wServicePort;
    channel.Secret_Ver = 0;
    channel.Sender = &safe_frame_send_callback;
    channel.Recver = &safe_frame_recv_callback;
    channel.Response = &safe_frame_Response_callback;
    channel.Progress = &safe_frame_Progress_callback;
    channel.pThis = this;   /* 填写设备指针 */
    m_SafeFrame_Sender = new CMsgSender(channel);
    m_SafeFrame_Recver = new CMsgRecver(channel);

    if (bsdb_syn && (bsdb_syn != m_bsdb_synidx)) {
        m_bsdb_synidx = bsdb_syn;
    }
    if (psdb_syn && (psdb_syn != m_psdb_synidx)) {
        m_psdb_synidx = psdb_syn;
    }

    // 通知相关周边接收信息
    zhw_bs_recv_db_idx(m_bsdb_synidx, m_psdb_synidx);

    dcf_output("[%s]bsc sucess to login in bss!(cirtify:0x%x)\r\n", dcf_get_time_string_unsafe(), m_sbs_cirtifyid);
}

/****************************************************************
*功能描述 : 保活的应答
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-07-05  18:10:9
****************************************************************/
void CBSClient::server_rsp_active(CRM_CMD &Rsp) {
    dcf_para_out out(Rsp.pbyPara, Rsp.wParaLen);

    dcf_output("[%d] 收到保活的数据长度", Rsp.wParaLen);

    WORD wError;
    DWORD bsdb_syn, psdb_syn;
    out >> wError >> bsdb_syn >> psdb_syn;
    if (out.have_error()) {
        dcf_output("[%s]bsc recv error data param!\r\n", dcf_get_time_string_unsafe());
        return;
    }

    m_times_noack_with_server = 0;
    dcf_output("[%s]bsc recv bss active ack!\r\n", dcf_get_time_string_unsafe());

    if (bsdb_syn && (bsdb_syn != m_bsdb_synidx)) {
        m_bsdb_synidx = bsdb_syn;
    }
    if (psdb_syn && (psdb_syn != m_psdb_synidx)) {
        m_psdb_synidx = psdb_syn;
    }
    // 通知相关周边接收信息
    zhw_bs_recv_db_idx(m_bsdb_synidx, m_psdb_synidx);
}


/****************************************************************
*功能描述 : 提供给可靠帧发送的回调接口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-24  10:34:52
****************************************************************/
DWORD CBSClient::safe_frame_send_callback(void *pMsg, WORD MsgLen, DWORD dstIP, WORD dstPort, WORD MsgType, BYTE bHost, void *pThis) {
    ASSERT(g_bsc_rpc != NULL);
    ASSERT(g_bsc_rpc->m_psocket != NULL);
    /* 1.frame帧封帧 */
    CRccHeader *pHead = (CRccHeader *) pMsg;
    CRccFrame *pFrame = (CRccFrame *) CRccFrameHelper::GetBufHeader((BYTE *) pMsg);
    pHead->Secret_Ver = RCC_CRY_VER_FRAME;
    CRccFrameHelper::package_header((BYTE *) pFrame, g_bsc_rpc->m_com_pubkey_bss, 1);

    // 2.写rcc层
    WORD Len = 0;
    CRccHeaderHelper::package_header((BYTE *) pMsg, Len, MsgType, 1);
    // 3.发送
    return g_bsc_rpc->m_psocket->SendFrame(pMsg, MsgLen, g_bsc_rpc->m_dwServiceIP, g_bsc_rpc->m_wServicePort);
}

/****************************************************************
*功能描述 : 提供给可靠帧发送的回调接口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-24  10:34:52
****************************************************************/
DWORD CBSClient::safe_frame_recv_callback(MSG_RPC &msg, void *pThis) {
    // 消息指针
    ASSERT(pThis != NULL);

    /* 数据解密 解密之后的长度? */
    DWORD dwRet = dcf_tools_ces128_decrypt(msg.pMsg, msg.MsgLen, msg.pMsg, g_bsc_rpc->m_com_selfkey);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "decrypt");

    // 构建消息报文
    MSG_RPC *pRPCMsg = (MSG_RPC *) dcf_mem_malloc(sizeof(MSG_RPC));
    memcpy(pRPCMsg, &msg, sizeof(MSG_RPC));

    // 这块内存由信息处理方接管
    msg.pMsg = NULL;

    // 将消息发给回调
    zhw_bs_recv_callback(*pRPCMsg);

    /* 2017-9-22 收到有效报文，则清0 */
    g_bsc_rpc->m_times_noack_with_server = 0;

    return dwRet;
}

/****************************************************************
*功能描述 : 提供给可靠帧发送的回调接口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     hhw
*创建日期 : 2023-07-10  10:17:24
****************************************************************/
DWORD CBSClient::safe_frame_Response_callback(MSG_RPC &msg, void *pThis) {
    // 消息指针
    ASSERT(pThis != NULL);

    /* 数据解密 解密之后的长度? */
    DWORD dwRet = dcf_tools_ces128_decrypt(msg.pMsg, msg.MsgLen, msg.pMsg, g_bsc_rpc->m_com_selfkey);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, BSC_MOD_ID_RPC, "decrypt");

    char chPara[128];
    const WORD wTotalLen = CRM_SIZE_ONETLV_MIN + sizeof(chPara);
    char chMsgBuffer[wTotalLen];
    memset(chMsgBuffer, 0, wTotalLen);
    BYTE* pMsg = msg.pMsg;
    BDT_TLV tlv;
    /* 1.取大数据报文中的 */
    CBDTMsgHelper::GetFromPacket(pMsg, tlv);
    CRM_CMD Rsp = { tlv.wCmdIdx,CRM_CTRL_RSP_END,tlv.wCmd,sizeof(chPara),(BYTE*)chPara };
    WORD Offset = 0; WORD flag = 0;
    BYTE* pCur = Rsp.pbyPara;
    memcpy(pCur, &flag, sizeof(WORD));
    Rsp.wParaLen = sizeof(WORD);
    CCRMMsgHelper::AddToPacket((BYTE*)chMsgBuffer, wTotalLen, Offset, Rsp, 1);
    Offset = ((Offset + 15) / 16) * 16;
    ASSERT(Offset < wTotalLen);
    msg.pMsg = (BYTE*)dcf_mem_malloc(Offset);
    memset(msg.pMsg, 0, Offset);
    memcpy(msg.pMsg, chMsgBuffer, Offset);
    msg.MsgLen = Offset;

    // 构建消息报文
    MSG_RPC *pRPCMsg = (MSG_RPC *) dcf_mem_malloc(sizeof(MSG_RPC));
    memcpy(pRPCMsg, &msg, sizeof(MSG_RPC));

    // 这块内存由信息处理方接管
    msg.pMsg = NULL;

    // 将消息发给回调
    zhw_bs_recv_callback(*pRPCMsg);

    /* 2017-9-22 收到有效报文，则清0 */
    g_bsc_rpc->m_times_noack_with_server = 0;

    return dwRet;
}

/****************************************************************
*功能描述 : 提供给可靠帧进度条的回调接口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     hhw
*创建日期 : 2023-12-04  16:16:34
****************************************************************/
void CBSClient::safe_frame_Progress_callback(WORD modId, WORD curBegin, WORD totalFrame) {
    dcf_output("[%s]bsc recv frame_Progress modId[%u] totalFrame[%u] curBegin[%u]!\r\n", dcf_get_time_string_unsafe(), modId, totalFrame, curBegin);
    zhw_bc_recv_progress(modId, curBegin, totalFrame);
}