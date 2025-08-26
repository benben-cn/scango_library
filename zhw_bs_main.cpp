
#include "bs_def.h"
#include "bsc_rpc.h"
#include "dcframe_proc.h"
#include "dcf_string.h"
#include "looper.h"
#include "zhw_pdalib.h"


extern CBSClient *g_bsc_rpc;

class BsRespLooper : public looper {
    virtual void handle(int what, void *obj);
};

BsRespLooper *g_bs_resp_looper = NULL;

void BsRespLooper::handle(int what, void *obj) {
    switch (what) {
        case RESP_MSG_RPC: {
            bs_proc_resp((MSG_RPC *) obj);
            break;
        }
        case RESP_DATABASE_IDX: {
            DatabaseIdx *idx = (DatabaseIdx *) obj;
            ProcDatabaseIdx(idx->baseDbIdx, idx->psDbIdx);
            break;
        }
        case RESP_LOGOUT: {
            ProcLogout();
            break;
        }
        case RESP_PROGRESS: {
            OnProgress* progress = (OnProgress*)obj;
            ProcOnProgress(progress->modId, progress->curBegin, progress->totalFrame);
            break;
        }
        default: {
            dcf_output("what unknown cmd:0x%x\r\n", what);
            break;
        }
    }
}
#define DEV_INFO_LEN 17 //密码最长为16位
static char g_key[KEY_MAX][DEV_INFO_LEN] = {0};

const char *dcf_key_get(WORD wType, WORD wVer) {
    ASSERT(wType < KEY_MAX);
    return &g_key[wType][0];
}

static DWORD g_bs_id[BS_ID_MAX] = {0};

void dcf_id_set(WORD wType, DWORD id) {
    ASSERT(wType < BS_ID_MAX);
    g_bs_id[wType] = id;
}

DWORD dcf_id_get(WORD wType) {
    ASSERT(wType < BS_ID_MAX);
    return g_bs_id[wType];
}

DWORD zhw_bs_initialize(
        const char *recv_addr,
        const char *gateway_addr,
        const char *dev_name,
        const char *dev_cirtify,
        DWORD bs_id,
        DWORD dev_id,
        WORD dev_no,
        DWORD dev_opt_related,
        const char *key_pub_com_bsg,
        const char *key_pub_com_bss,
        const char *key_self_com_bs,
        const char *key_pub_com_qrcode,
        const char *key_self_qrcode_bs,
        const char *version,
        const char *address
) {
    if (g_bsc_rpc) {
        dcf_output("[%s]CBSClient already created!\r\n", dcf_get_time_string_unsafe());
        return 1;
    } else {
        dcf_output("[%s]begin to create CBSClient!\r\n", dcf_get_time_string_unsafe());
        g_bsc_rpc = new CBSClient(
                (char *) recv_addr,
                (char *) gateway_addr,
                (char *) dev_name,
                (char *) dev_cirtify,
                bs_id,
                dev_id,
                dev_no,
                dev_opt_related,
                (char *) version,
                (char *) address
        );
    }

    // 先获取各种密码
    dcf_strtools::strcpy_s(&g_key[KEY_PUB_COM_BSG][0], key_pub_com_bsg, DEV_INFO_LEN);
    dcf_strtools::strcpy_s(&g_key[KEY_PUB_COM_BSS][0], key_pub_com_bss, DEV_INFO_LEN);
    dcf_strtools::strcpy_s(&g_key[KEY_SELF_COM_BS][0], key_self_com_bs, DEV_INFO_LEN);
    dcf_strtools::strcpy_s(&g_key[KEY_PUB_COM_QRCODE][0], key_pub_com_qrcode, DEV_INFO_LEN);
    dcf_strtools::strcpy_s(&g_key[KEY_SELF_QRCODE_BS][0], key_self_qrcode_bs, DEV_INFO_LEN);

    dcf_output("[%s]begin to init CBSClient!\r\n", dcf_get_time_string_unsafe());

    bool init = g_bsc_rpc->Initialize();

    if (init && !g_bs_resp_looper) {
        g_bs_resp_looper = new BsRespLooper();
    }

    return init ? 0 : 2;
}

DWORD zhw_bs_quit() {
    if (g_bsc_rpc) {
        g_bsc_rpc->~CBSClient();
        delete g_bsc_rpc;
        g_bsc_rpc = NULL;
    }
    if (g_bs_resp_looper) {
        g_bs_resp_looper->quit();
//        g_bsc_rpc->~CBSClient();
        delete g_bs_resp_looper;
        g_bs_resp_looper = NULL;
    }
    return 0;
}

DWORD zhw_bs_get_server_state() {
    if (g_bsc_rpc) {
        return g_bsc_rpc->GetSeverState();
    }
    return 0;
}

WORD zhw_bs_get_get_func_list(WORD *funcList) {
    WORD length = g_bsc_rpc->GetFuncList(funcList);

    return length;
}

bool zhw_bs_active_server() {
    if (2 == zhw_bs_get_server_state()) {// 通讯正常的情况下才发送保活帧
        g_bsc_rpc->server_active();
        return true;
    }
    return false;
}

void zhw_bs_recv_db_idx(DWORD baseDbIdx, DWORD psDbIdx) {
    DatabaseIdx *idx = new DatabaseIdx();
    idx->baseDbIdx = baseDbIdx;
    idx->psDbIdx = psDbIdx;
    g_bs_resp_looper->post(RESP_DATABASE_IDX, idx);
}

void zhw_bs_recv_callback(MSG_RPC &msg) {
    g_bs_resp_looper->post(RESP_MSG_RPC, &msg);
}

void zhw_bc_recv_progress(WORD modId, WORD curBegin, WORD totalFrame) {
    OnProgress* progress = new OnProgress();
    progress->modId = modId;
    progress->curBegin = curBegin;
    progress->totalFrame = totalFrame;
    g_bs_resp_looper->post(RESP_PROGRESS, progress);
}

DWORD zhw_bs_send_msg(MSG_RPC &msg, DWORD *pSurredtHandle, DWORD timeout, DWORD resendtimes) {
    return g_bsc_rpc->SendMessage(msg);
}

DWORD zhw_bs_direct_send_msg(void *pMsg, DWORD dstIP, WORD Port) {
    return g_bsc_rpc->DirectSendMessage(pMsg, dstIP, Port);
}


/**
 * 当数据量大时,分批上传数据
 * @return
 */
DWORD batches_sendRawMessage(DWORD fileLen_J, DWORD size, WORD cmd_I, BYTE *pData, WORD infoType_I, WORD modId_I, DWORD dwRet) {

    DWORD dwMsgLen = RCC_SIZE_BIG_HEAD + sizeof(GZIP_INFO) + size;
    dwMsgLen = ((dwMsgLen + 15) / 16) * 16;
    BYTE *pMsg = (BYTE *) dcf_mem_malloc(dwMsgLen);
    memset(pMsg, 0, dwMsgLen);
    CBDTMsgHelper::InitMsgBuffer(pMsg, CRM_CTRL_CMD_NEEDREQ, cmd_I, dwMsgLen);
    BDT_TLV *pHead = (BDT_TLV *) pMsg;
    GZIP_INFO *pzlibHead = (GZIP_INFO * )(pMsg + RCC_SIZE_BIG_HEAD);
    pzlibHead->org_len = 0;
    pzlibHead->zlib_len = fileLen_J;

    memcpy(pMsg + RCC_SIZE_BIG_HEAD + RCC_SIZE_GZIP_HEAD, pData, size);

    MSG_RPC msg = {0};
    msg.ctrl = RCC_CTRL_FLAG_CMD_SURE;
    msg.bhost = 1;
    msg.MsgLen = dwMsgLen;
    msg.wMsgType = RCC_MSG_TYPE_RAW_BSS_BIGDATA;
    msg.pMsg = pMsg;
    msg.Msg_Secret_Ver = 0;
    msg.wMsg1 = infoType_I;  /* 这个是到达服务端的服务类型 */
    msg.wMsg2 = modId_I;   /* 这个是服务端将消息发送回来的，如果填写错误，则收不到响应 */

    dwRet = zhw_bs_send_msg(msg);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, (WORD) modId_I, "SendMessage");
    if (msg.pMsg) {
        // 对方未释放内存
        dcf_mem_free((void *&) msg.pMsg);
        msg.pMsg = NULL;
    }
    return dwRet;
}


/**
 * 退出登录
 */
void zhw_bs_recv_logout() {
    g_bs_resp_looper->quit();
    delete g_bs_resp_looper;
    g_bs_resp_looper = NULL;
    g_bsc_rpc->~CBSClient();
    delete g_bsc_rpc;
    g_bsc_rpc = NULL;
    //    g_bs_resp_looper->post(RESP_LOGOUT, NULL);
    ProcLogout();

}
