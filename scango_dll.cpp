#include "scango_dll.h"
#include "dcf_pkghelper.h"
#include "dcf_rcc_dthelper.h"
#include "dcf_vfs.h"
#include "dcf_zlib.h"
#include "extern_api.h"
#include "zhw_pdalib.h"
#include <stdio.h>
#include <string.h>
//#include "dcf_qrcode.h"
//#include "zhw_tools.h"
#include "dcf_base64.h"
//#include <unistd.h>

//#include<QVector> 
//#include<vector> 
#include "pthread.h"
#include "cJSON.h"
#include "dcf_err.h"

extern pthread_mutex_t mut_callback;
extern onLogout gonLogout;
extern onDatabaseIdxRecv gonDatabaseIdxRecv;
extern onError gonError;
extern onLoginError gonLoginError;
extern onProgress gonProgress;
extern onCRMRecv gonCRMRecv;
extern newTempFile gnewTempFile;
extern onFileRecv gonFileRecv;

void my_output(char* pstr)
{
    printf(pstr);
}

//bool isFileExists_fopen(const char* name) {
//    FILE* fp;
//    if (!fopen_s(&fp, name, "r")) {
//        fclose(fp);
//        return true;
//    }
//    else {
//        return false;
//    }
//}

DWORD smartdog_scango_dcframe_BSClient_init(
    const char* recv_addr,
    const char* gateway_addr,
    const char* dev_name,
    const char* dev_cirtify,
    DWORD bs_id,
    DWORD dev_id,
    WORD dev_no,
    DWORD dev_opt_related,
    const char* key_pub_com_bsg,
    const char* key_pub_com_bss,
    const char* key_self_com_bs,
    const char* key_pub_com_qrcode,
    const char* key_self_qrcode_bs,
    const char* version,
    const char* address) {

    pthread_mutex_init(&mut_callback, NULL);

    // 挂接打印函数
    g_print_debugstring = my_output;
    //char chConfigPath[128] = { 0 };
    //#if _SW_DCF_MACOS
    //getcwd(chConfigPath, sizeof(chConfigPath));
    //#else 
    //GetCurrentDirectoryA(sizeof(chConfigPath), chConfigPath);
    //dcf_strtools::replace_char(chConfigPath, '\\', '/');
    //#endif
    //dcf_vfs_set_run_root(chConfigPath);

    //char log_old_path[128] = { 0 }; char log_new_path[128] = { 0 };
    //sprintf_s(log_old_path, sizeof(log_old_path), "%s/data/log/debug.log", chConfigPath);
    //if (isFileExists_fopen(log_old_path))
    //{
    //    dcf_output("[%s]need to rename!\r\n", dcf_get_time_string_unsafe());
    //    time_t timep;
    //    struct tm p;
    //    time(&timep); //获取从1970至今过了多少秒，存入time_t类型的timep
    //    localtime_s(&p, &timep);//用localtime将秒数转化为struct tm结构体
    //    sprintf_s(log_new_path, sizeof(log_new_path), "%s/data/log/debug_%d%d%d%02d%02d%02d.log", chConfigPath, 1900 + p.tm_year, 1 + p.tm_mon, p.tm_mday, p.tm_hour, p.tm_min, p.tm_sec);
    //    dcf_output("[%s]log_new_path:%s\r\n", dcf_get_time_string_unsafe(), log_new_path);
    //    int ret = rename(log_old_path, log_new_path);
    //    if (ret == 0)
    //    {
    //        dcf_output("文件重命名成功\r\n");
    //    }
    //    else
    //    {
    //        dcf_output("错误[%d]：不能重命名该文件\r\n",ret);
    //    }
    //}

    //const char* vfs_json = "{\"type_1\":\"/data\",\"type_2\":\"\",\"type_3\":\"\",\"type_4\":\"\",\"type_5\":\"/log\",\"type_6\":\"\"}";
    //cJSON* root = cJSON_Parse(vfs_json);
    //dcf_m_vfs_init(root);
    //const char* log_json = "{\"log_type\":3}";
    //root = cJSON_Parse(log_json);
    //dcf_m_log_init(root);
    //cJSON_Delete(root);

    //extern void dcf_log_open();
    //dcf_log_open();

    DWORD rvalue = zhw_bs_initialize(
        recv_addr,
        gateway_addr,
        dev_name,
        dev_cirtify,
        bs_id,
        dev_id,
        dev_no,
        dev_opt_related,
        key_pub_com_bsg,
        key_pub_com_bss,
        key_self_com_bs,
        key_pub_com_qrcode,
        key_self_qrcode_bs,
        version,
        address
    );

    return rvalue;
}

DWORD smartdog_scango_dcframe_BSClient_quit() {
    return zhw_bs_quit();
}

DWORD smartdog_scango_dcframe_BSClient_getServerState() {
    return zhw_bs_get_server_state();
}

// vector<WORD> smartdog_scango_dcframe_BSClient_getFuncList() {
//     WORD funcList[128] = {0};
//     WORD length = zhw_bs_get_get_func_list(funcList);

//     vector<WORD> func_array;
//     for (int i = 0; i < length; i++) {
//         func_array.push_back(funcList[i]);
//     }

//     return func_array;
// }

bool smartdog_scango_dcframe_BSClient_sendHeartBeatPacket() {

    return zhw_bs_active_server();
}

uint64 uint64_strtol(const char* pstr, long defvalue, char** pEnd)
{
    uint64 lvalue = 0;
    bool bHex = false;
    pstr = dcf_strtools::skip_space(pstr);

    if ((*pstr == '0') && ((*(pstr + 1) == 'x') || (*(pstr + 1) == 'X')))
    {
        pstr += 2;
        bHex = true;
    }

    if (!(*pstr))
    {
        // 没有有效内容，填写缺省值
        lvalue = defvalue;
        goto clearn;
    }

    while (*pstr)
    {
        if ((*pstr >= '0') && (*pstr <= '9'))
        {
            if (bHex)
            {
                lvalue = (lvalue << 4) + ((BYTE)*pstr - (BYTE)'0');
            }
            else
            {
                lvalue = lvalue * 10 + ((BYTE)*pstr - (BYTE)'0');
            }
        }
        else if ((bHex) && (*pstr >= 'a') && (*pstr <= 'f'))
        {
            lvalue = (lvalue << 4) + ((BYTE)*pstr - (BYTE)'a') + 10;
        }
        else if ((bHex) && (*pstr >= 'A') && (*pstr <= 'F'))
        {
            lvalue = (lvalue << 4) + ((BYTE)*pstr - (BYTE)'A') + 10;
        }
        else
        {
            break;
        }
        pstr++;
    }
clearn:
    if (pEnd)
    {
        /* 2018-01-16  10:49:9 将空格跳过 */
        *pEnd = (*pstr) ? (char*)dcf_strtools::skip_space(pstr) : NULL;
    }
    return lvalue;
}

DWORD PacketParam_Json_Fm(CRM_CMD& Cmd, WORD& Offset, const char* parafmt, const char* marker) {
    if ((!parafmt) || (!*parafmt) || (!Cmd.pbyPara) || (!Cmd.wParaLen)) {
        return DCF_ERR_PARAM;
    }

    BYTE* pCur = Cmd.pbyPara;
    WORD wBufLen = Cmd.wParaLen;
    WORD para_nums = 0;

    cJSON* root = cJSON_Parse(marker);
    cJSON* childNode;
    WORD json_nums = 0;


#undef CHECK_WRITE_LEN
#define CHECK_WRITE_LEN(inc) \
        Offset += inc; para_nums++; json_nums++; if (Offset > wBufLen || childNode == NULL) return DCF_ERR_PARAM


    while (*parafmt) {
        char ch = *parafmt;
        switch (ch) {
        case 'B':
        case 'b': {
            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(sizeof(BYTE));
            *pCur = (BYTE)childNode->valueint;
            pCur++;
            break;
        }
        case 'W':
        case 'w': {
            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(sizeof(WORD));
            WORD w = (WORD)childNode->valueint;
            memcpy(pCur, &w, sizeof(WORD));
            pCur += sizeof(WORD);
            if (Cmd.IsRsp() && w && (para_nums == 1)) {
                /* 2017-08-21  10:23:18 是响应，而且失败了，则跳出其它参数处理 */
                goto Fmtend;
            }
            break;
        }
        case 'Q':
        case 'q': {
            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(sizeof(uint64));
            char* str = childNode->valuestring;
            char* pEnd = NULL;
            uint64 dw = uint64_strtol(str, 0, &pEnd);
            memcpy(pCur, &dw, sizeof(uint64));
            pCur += sizeof(uint64);
            break;
        }
        case 'D':
        case 'd': {
            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(sizeof(DWORD));
            char* str = childNode->valuestring;
            char* pEnd = NULL;
            DWORD dw = uint64_strtol(str, 0, &pEnd);
            memcpy(pCur, &dw, sizeof(DWORD));
            pCur += sizeof(DWORD);
            break;
        }
        case 'S':
        case 's': {
            parafmt++;
            char* pEnd = NULL;
            int ilen = dcf_strtools::strtol(parafmt, 0, &pEnd);
            if (ilen <= 0) {
                return DCF_ERR_PARAM;
            }

            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(ilen);
            char* str = childNode->valuestring;
            dcf_strtools::strcpy_s((char*)pCur, str, ilen);
            pCur += ilen;

            if (pEnd) {
                parafmt = pEnd;
                parafmt--; /* 后面有一个++ */
            }
            else {
                goto Fmtend;
            }

            break;
        }
        case 'f':
        case 'F': {
            parafmt++;
            char* pEnd = NULL;
            int ilen = dcf_strtools::strtol(parafmt, 0, &pEnd);
            if (ilen <= 0) {
                return DCF_ERR_PARAM;
            }

            childNode = cJSON_GetArrayItem(root, json_nums);
            CHECK_WRITE_LEN(ilen);
            BYTE* ptr = (BYTE*)childNode->valuestring;
            memcpy(pCur, ptr, ilen);
            pCur += ilen;

            if (pEnd) {
                parafmt = pEnd;
                parafmt--; /* 后面有一个++ */
            }
            else {
                goto Fmtend;
            }

            break;
        }
        case 'v':
        case 'V': {
            childNode = cJSON_GetArrayItem(root, json_nums);
            cJSON* len_item = cJSON_GetArrayItem(childNode, 0);
            cJSON* ptr_item = cJSON_GetArrayItem(childNode, 1);
            WORD wLen = (WORD)len_item->valueint;
            CHECK_WRITE_LEN(wLen);
            BYTE* ptr = (BYTE*)ptr_item->valuestring;
            memcpy(pCur, ptr, wLen);
            pCur += wLen;

            /* 最后一个跳出循环 */
            goto Fmtend;
        }
        case '-':
        case '/':
        case ' ': {
            // 分解符
            break;
        }
        default: {
            // 其它类型后面再添加
            return DCF_ERR_PARAM;
        }
        }
        parafmt++;
    }

Fmtend:
    cJSON_Delete(root);
    return 0;
}

DWORD PacketParam_Json(CRM_CMD& Cmd, WORD& Offset, const char* marker) {
    BYTE bCmd = Cmd.IsCmd() ? 1 : 0;
    const char* parafmt = dcf_tools_get_cmd_fmt_info(Cmd.wCmd, bCmd);
    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }
    //dcf_output("cmd[%d],bCmd[%d]\r\n", Cmd.wCmd, bCmd);
    DWORD dwRet = PacketParam_Json_Fm(Cmd, Offset, parafmt, marker);
    Cmd.wParaLen = Offset;
    return dwRet;
}

DWORD smartdog_scango_dcframe_BSClient_sendCMD(
    WORD modId_I,
    WORD infoType_I,
    WORD cmd_I,
    const char* marker
) {
    BYTE chPara[512];
    const DWORD dwTotalLen = sizeof(chPara) + RCC_SIZE_RAW_ONETLV_MIN;
    BYTE chMsgBuffer[dwTotalLen] = { 0 };  /* 参数区 */
    // 参数封包
    memset(chMsgBuffer, 0, sizeof(chMsgBuffer));
    WORD Offset = 0;

    WORD m_wCmdIdx = 0;
    // 构建命令包
    CRM_CMD Cmd = { ++m_wCmdIdx, CRM_CTRL_CMD_NEEDREQ, cmd_I, sizeof(chPara), chPara };
    // 1.先构建参数部分
    DWORD dwRet = PacketParam_Json(Cmd, Offset, marker);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, modId_I, "pack para");
    // 2.命令打包
    CCRMMsgHelper::AddToPacket(chMsgBuffer, sizeof(chMsgBuffer), Offset, Cmd, 1);
    Offset = ((Offset + 15) / 16) * 16;
    ASSERT(Offset < dwTotalLen);

    // 3.构建RPC包
    MSG_RPC msg = {
            chMsgBuffer,
            Offset,
            RCC_CTRL_FLAG_CMD_SURE,
            1,
            0,
            infoType_I,
            modId_I,
            RCC_MSG_TYPE_RAW_BSS_CRM,
            0,
            0, 0, 0, 0, 0,
            0
    };

    // 4.发包
    dwRet = zhw_bs_send_msg(msg);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, (WORD)modId_I, "SendMessage");

    return dwRet;
}


DWORD smartdog_scango_dcframe_BSClient_sendRawMessage(
    WORD modId_I,
    WORD infoType_I,
    WORD cmd_I,
    const char* srcPath,
    DWORD fileLen
) {
    WORD srcPathLen = strlen(srcPath);  //sizeof(srcPath)求出的是指针的大小，8个字节（64位）
    srcPathLen += 4; //不是3
    char* zipPath = new char[srcPathLen];
    strlcpy(zipPath, srcPath, srcPathLen);
    strlcat(zipPath, ".gz", srcPathLen);

    // 压缩源文件
    DWORD dwRet = dcf_zlib_compress_ftof(srcPath, zipPath);

    // 清理字符串
    if (dwRet) {
        // 记得清理临时文件
        remove(zipPath);
        dcf_output("[%s]压缩文件时出错!\r\n", dcf_get_time_string_unsafe());
        CATCH_ERR_RET(dwRet, dwRet);
    }
    long zipLen = 0;
    BYTE* pData = (BYTE*)dcf_file_read((char*)zipPath, zipLen);
    // 记得清理临时文件
    remove(zipPath);

    DWORD dwMsgLen = RCC_SIZE_BIG_HEAD + sizeof(GZIP_INFO) + fileLen;
    dwMsgLen = ((dwMsgLen + 15) / 16) * 16;
    BYTE* pMsg = (BYTE*)dcf_mem_malloc(dwMsgLen);
    memset(pMsg, 0, dwMsgLen);
    CBDTMsgHelper::InitMsgBuffer(pMsg, CRM_CTRL_CMD_NEEDREQ, cmd_I, dwMsgLen);
    BDT_TLV* pHead = (BDT_TLV*)pMsg;
    GZIP_INFO* pzlibHead = (GZIP_INFO*)(pMsg + RCC_SIZE_BIG_HEAD);
    pzlibHead->org_len = 0;
    pzlibHead->zlib_len = fileLen;

    memcpy(pMsg + RCC_SIZE_BIG_HEAD + RCC_SIZE_GZIP_HEAD, pData, zipLen);
    //    dcf_output("===========开始准备上传数据:[%d]", zipLen);

    MSG_RPC msg = { 0 };
    msg.ctrl = RCC_CTRL_FLAG_CMD_SURE;
    msg.bhost = 1;
    msg.MsgLen = dwMsgLen;
    msg.wMsgType = RCC_MSG_TYPE_RAW_BSS_BIGDATA;
    msg.pMsg = pMsg;
    msg.Msg_Secret_Ver = 0;
    msg.wMsg1 = (WORD)infoType_I;  /* 这个是到达服务端的服务类型 */
    msg.wMsg2 = (WORD)modId_I;   /* 这个是服务端将消息发送回来的，如果填写错误，则收不到响应 */
    dwRet = zhw_bs_send_msg(msg);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, (WORD)modId_I, "SendMessage");
    if (msg.pMsg) {
        // 对方未释放内存
        dcf_mem_free((void*&)msg.pMsg);
        msg.pMsg = NULL;
    }
    //    dcf_output("===========上传完成");
    /************************ 原代码  end ************************
    *作   者 :     yijianhong 修改
    *创建日期 : 2018-10-15
    ****************************************************************/
    //
    //    const char *srcPath = env->GetStringUTFChars(filePath_jstring, NULL);
    //    jsize srcPathLen = env->GetStringUTFLength(filePath_jstring);
    //    char zipPath[srcPathLen + 3];
    //    strcpy(zipPath, srcPath);
    //    strcat(zipPath, ".gz");
    //
    //    // 压缩源文件
    //    DWORD dwRet = dcf_zlib_compress_ftof(srcPath, zipPath);
    //
    //    // 清理字符串
    //    env->ReleaseStringUTFChars(filePath_jstring, srcPath);
    //    if (dwRet) {
    //        // 记得清理临时文件
    //        remove(zipPath);
    //        dcf_output("[%s]压缩文件时出错!\r\n", dcf_get_time_string_unsafe());
    //        CATCH_ERR_RET(dwRet, dwRet);
    //    }
    //    long zipLen = 0;
    //    BYTE *pData = (BYTE *) dcf_file_read((char *) zipPath, zipLen);
    //    // 记得清理临时文件
    //
    //
    //    DWORD BUF_SIZE = 1024 * 512;
    //    DWORD size = BUF_SIZE, total = 0;
    //    DWORD len = (DWORD) zipLen; //10240
    //    int i = 0;
    //    while (total < len) {
    //        dcf_output("===========循环上传中-本次数据len:[%d]", len);
    //        size = len - total > BUF_SIZE ? BUF_SIZE : len - total;
    //        dwRet = batches_sendRawMessage((WORD) fileLen_J,size, (WORD) cmd_I, pData + (size * i++), infoType_I, modId_I, dwRet);
    //        total += size;
    //        sleep(1);
    //    }
    //    remove(zipPath);
    return dwRet;
}

DWORD smartdog_scango_qrcode_security_ZheDaoRandomCodeTool_convertRandomIdNew(DWORD randomId) {


    //如果随机数大于6位
    if (randomId > 999999) {
        randomId = randomId & 0xF423F; //去掉高位,保留低位
    }
    //如果随机数小于6位
    if (randomId < 100000) {
        randomId = randomId | 0x20000; //改变某位,让其是6位随机数
    }

    //取前四位
    randomId = randomId / 100;

    return randomId;



    //    DWORD dResult = 0;
    //    std::stack<int> istack;
    //    while (randomId >= 10) {
    //        int ire = randomId % 10; // 0
    //        int nyu = (((3 * ire * ire * ire) % 10) + 7) % 10; //7
    //        istack.push(nyu);
    //        randomId = randomId / 10;
    //    }
    //    int nyu = (((3 * randomId * randomId * randomId) % 10) + 7) % 10;
    //    istack.push(nyu);
    //    while (!istack.empty()) {
    //        dResult = dResult * 10 + istack.top();
    //        istack.pop();
    //    }
    //    if (dResult == 0) {
    //        dResult = 47683;
    //    }
    //    return dResult;

}

void dcf_tools_string_decode(char* src) {
    if (NULL == src) return;
    int flag = -1;
    for (int i = 0; i < strlen(src); i++)
    {
        *(src + i) += flag;
        flag = -flag;
    }
};

/* 九十进制 */
int m_nDigital = 90;   //从ascii码中挑出90个可见字符
const char* m_charSet = "0123456789~!@#$%^&*()_+-=[]{}|;:,./<>?ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
//10进制转化成的90进制字符串

//90进制转为10进制
ULONGLONG dcf_tools_to_10(const char* srcx) {
    ULONGLONG dst = 0L;
    for (int i = 0; i < strlen(srcx); i++) {
        char c = srcx[i];
        for (int j = 0; j < m_nDigital; j++) {
            if (c == m_charSet[j]) {
                dst = (dst * m_nDigital) + j;
                break;
            }
        }
    }
    return dst;
}

struct qrcode_decrypt_data smartdog_scango_qrcode_ZheDaoQRCodeTool_nativeParseGoodsQRCodeNew(const char* fullQRCode) {
        //新的解码规则
        // 3.base64解码
    BYTE decode[8] = { 0 };
    dcf_tools_base64_decode((BYTE*)fullQRCode, decode, 8);

    // 4.解密
    char* temp2 = (char*)&decode[0];
    temp2[7] = '\0';
    dcf_tools_string_decode(temp2);

    //5. 解压缩: 90进制转换为10进制
    ULONGLONG id_range = dcf_tools_to_10(temp2);

    // 6.得到企业id
//    ULONGLONG random = ((id_range) & 0x7FFFF) / 2;
//    ULONGLONG table_id = ((id_range - random * 2) >> 19) - random;

    ULONGLONG random = id_range & 0xFFFFFF;
    ULONGLONG table_id = id_range >> 24;

    qrcode_decrypt_data qr_dec_data;
    qr_dec_data.random = (DWORD)random;
    qr_dec_data.table_id = (DWORD)table_id;

    return qr_dec_data;
}

//static onLogout gonLogout = nullptr;
void SetonLogout(onLogout cb) {
    gonLogout = cb;
}

//static onDatabaseIdxRecv gonDatabaseIdxRecv = nullptr;
void SetonDatabaseIdxRecv(onDatabaseIdxRecv cb) {
    gonDatabaseIdxRecv = cb;
}

//static onError gonError = nullptr;
void SetonError(onError cb) {
    gonError = cb;
}

//static onLoginError gonLoginError = nullptr;
void SetonLoginError(onLoginError cb) {
    gonLoginError = cb;
}

//static onProgress gonProgress = nullptr;
void SetonProgress(onProgress cb) {
    gonProgress = cb;
}

//static onCRMRecv gonCRMRecv = nullptr;
void SetonCRMRecv(onCRMRecv cb) {
    gonCRMRecv = cb;
}

//static newTempFile gnewTempFile = nullptr;
void SetnewTempFile(newTempFile cb) {
    gnewTempFile = cb;
}

//static onFileRecv gonFileRecv = nullptr;
void SetonFileRecv(onFileRecv cb) {
    gonFileRecv = cb;
}
