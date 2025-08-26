/****************************************************************
*文件范围 : 中海塆服务客户端的初始化部分
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  13:51:57
****************************************************************/
#include "bsc_rpc.h"
#include "extern_api.h"
#include "dcf_string.h"
#include "bs_def.h"

CBSClient *g_bsc_rpc = NULL;

/****************************************************************
*功能描述 : 客户端构造函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  14:43:35
****************************************************************/
CBSClient::CBSClient(
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

) {
    m_psocket = NULL;
    m_RecvFrameBuffer = (BYTE *) dcf_mem_malloc(RCC_SIZE_TOTAL);
    m_SafeFrame_Sender = NULL;
    m_SafeFrame_Recver = NULL;

    m_sbg_cirtifyid = 0;
    m_sbs_cirtifyid = 0;
    m_gate_cmdidx = 0;
    m_sbs_cmdidx = 0;

    //m_thread = 0;
    m_stop_work = false;

    char *pEnd = NULL;
    m_dwRecvDataIP = htonl(dcf_strtools::StrtoIP(recv_addr, &pEnd));
    if (pEnd && (*pEnd == ':')) {
        pEnd++;
        m_wRecvDataPort = htons((WORD) dcf_strtools::strtol(pEnd, 0, &pEnd));
    }

    /* 获取网关服务器的IP地址和端口 */
    m_dwGateWayIP = htonl(dcf_strtools::StrtoIP(gateway_addr, &pEnd));
    if (pEnd && (*pEnd == ':')) {
        pEnd++;
        m_wGateWayPort = htons((WORD) dcf_strtools::strtol(pEnd, 0, &pEnd));
    }

    /* 公钥和私钥 */
    dcf_strtools::strcpy_s(m_dev_name, dev_name, sizeof(m_dev_name));
    dcf_strtools::strcpy_s(m_version, version, sizeof(m_version));
    dcf_strtools::strcpy_s(m_address, address, sizeof(m_address));
    dcf_strtools::strcpy_s(m_dev_cirtify, dev_cirtify, sizeof(m_dev_cirtify));

    m_com_pubkey_bsg = dcf_key_get(KEY_PUB_COM_BSG, 0);
    m_com_pubkey_bss = dcf_key_get(KEY_PUB_COM_BSS, 0);
    m_com_selfkey = dcf_key_get(KEY_SELF_COM_BS, 0);

    /* 设备属性 */
    m_bsid = bs_id;
    m_dev_id = dev_id;
    m_dev_no = dev_no;
    m_dev_opt_related = dev_opt_related;
}

CBSClient::~CBSClient() {
    m_stop_work = true;
    pthread_join(m_thread, NULL);

    if (m_psocket) {
        delete m_psocket;
        m_psocket = NULL;
    }
    if (m_RecvFrameBuffer) {
        dcf_mem_free((void *&) m_RecvFrameBuffer);
        m_RecvFrameBuffer = NULL;
    }
    if (m_SafeFrame_Sender) {
        delete m_SafeFrame_Sender;
        m_SafeFrame_Sender = NULL;
    }
    if (m_SafeFrame_Recver) {
        delete m_SafeFrame_Recver;
        m_SafeFrame_Recver = NULL;
    }
}

/****************************************************************
*功能描述 : 初始化构造函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-29  15:27:28
****************************************************************/
bool CBSClient::Initialize() {
    m_psocket = new CDcfUDP();
    if (m_psocket->Initialize(m_dwRecvDataIP, m_wRecvDataPort)) {
        dcf_output("initialize socket failed!\r\n");
        return false;
    }

    //if (!m_thread) {
        pthread_attr_t threadAttr;
        pthread_attr_init(&threadAttr);
        pthread_attr_setstacksize(&threadAttr ,512 * 1024);
        //threadAttr.stack_size = 512 * 1024;
        int result = pthread_create(&m_thread, &threadAttr, &CBSClient::taskEntry, this);
        if (result != 0) {
            return false;
        }
    //}

    return true;
}

void *CBSClient::taskEntry(void *p) {
    CBSClient *pThis = (CBSClient *) p;
    pThis->MainLoop();
    return 0;
}
