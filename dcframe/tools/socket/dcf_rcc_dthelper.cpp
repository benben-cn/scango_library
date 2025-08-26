/****************************************************************
*文件范围 : 通信包头的一些API实现函数
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-18  9:23:50
****************************************************************/

#include "dcf_rcc_dthelper.h"
#include "extern_api.h"
#include "dcf_pkghelper.h"
#include "dcf_err.h"

union RCC_MAGIC {
    CHAR chMagic[4];  /* 魔术字 "DCF\0"*/
    DWORD dwMagic;
};

const RCC_MAGIC RCCFLAG_BIG = {'D', 'C', 'F', '\0'};
const DWORD RCCFLAG_HOST = ((((DWORD) 'D') << 24) | (((DWORD) 'C') << 16) | (((DWORD) 'F') << 8));

DWORD CRccHeaderHelper::get_rcc_magic_flag_host() {
    return RCCFLAG_HOST;
}

DWORD CRccHeaderHelper::get_rcc_magic_flag_net() {
    return RCCFLAG_BIG.dwMagic;
}

/****************************************************************
*功能描述 : 识别包头是否为主机字节序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-18  9:28:6
****************************************************************/
bool CRccHeaderHelper::IsValidHead() {
    // 1.标记必须相同
    if ((m_pHead->dwMagic != get_rcc_magic_flag_host()) && (m_pHead->dwMagic != get_rcc_magic_flag_net())) {
        return false;
    }

    /* 2.版本兼容判断 */
    if ((m_pHead->wVer & 0xff00) != (RCC_VER & 0xff00)) {
        /* 大版本必须相同 */
        return false;
    }

    /* 3.通信帧类型判断 */
    if (m_pHead->Msgtype > RCC_MSG_TYPE_MAX) {
        return false;
    }

    /* 4.帧长度判断 */
    if ((m_pHead->PackageLen + RCC_SIZE_HEADER) > RCC_SIZE_TOTAL) {
        return false;
    }

    /* 5.头部CRC校验 */
    CRccHeader *pHead = (CRccHeader *) m_pHead;
    WORD wOldCrc16 = pHead->HeaderCRC16;
    pHead->HeaderCRC16 = 0;
    WORD wCalCrc = dcf_tools_crc16_get((BYTE *) pHead, sizeof(CRccHeader));
    pHead->HeaderCRC16 = wOldCrc16;
    if (wCalCrc != wOldCrc16) {
        return false;
    }

    return true;
}

/****************************************************************
*功能描述 : 在缓存pData中找有效帧头部，并将字节序转换后的头部输出到pOutHeader
*输入参数 : pData:用户数据
*输出参数 : pOutHeader有效的头部
                   offset 距离FromPt的偏移
*返回参数 : 0成功 其它失败
*作   者 : zjb
*创建日期 : 2017-05-18  14:2:6
****************************************************************/
WORD CRccHeaderHelper::search_header(BYTE *pData, BYTE *pOutHeader, WORD FromPt, WORD DataLen) {
    if (DataLen < sizeof(DWORD)) {
        // 数据内容太少了，直接返回
        return FromPt;
    }

            ASSERT(pData != NULL);
            ASSERT(pOutHeader != NULL);
    BYTE *pCur = pData + FromPt;
    CRccHeader *pHead = NULL;

    /* 要留4个字节，否则下面第一条语句会越界访问 */
    WORD wEndPt = DataLen - sizeof(DWORD);
    WORD wCurPt = FromPt;
    WORD wOffset = 0;
    for (; wCurPt <= wEndPt; wCurPt++) {
        // 先找标记头部
        if ((*((DWORD *) pCur) != RCCFLAG_BIG.dwMagic) && (*((DWORD *) pCur) != RCCFLAG_HOST)) {
            pCur++;
            continue;
        }

        // 再看一下长度还是否够
        if (((FromPt + wCurPt) + sizeof(CRccHeader)) > DataLen) {
            // 长度不够了
            break;
        }

        // 将数据转换到pOutHeader，然后再校验
        // 转换为主机字节序
        wOffset = 0;
        if (*((DWORD *) pCur) != RCCFLAG_HOST) {
            // 和自己的字节序不同
            (void) dcf_pkg_tool::cvt_fmt_ntoh(pCur, struct_fmt_rcc_header, pOutHeader, wOffset, sizeof(CRccHeader), sizeof(CRccHeader));
        } else {
            memcpy(pOutHeader, pCur, sizeof(CRccHeader));
        }

        pHead = (CRccHeader *) pOutHeader;
        CRccHeaderHelper helper(pHead);
        if (helper.IsValidHead()) {
            // 找对了
            break;
        }

        // 继续找后面的
        pHead = NULL;
    }

    return wCurPt;
}

/****************************************************************
*功能描述 : 数据封帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-18  16:47:36
****************************************************************/
void CRccHeaderHelper::package_header(BYTE *pData, WORD &MsgLen, WORD msgType, BYTE bHost) {
            ASSERT(pData != NULL);
    CRccHeader *pHead = (CRccHeader *) pData;
    // pHead->PackageLen = TotalLen - sizeof(CRccHeader);
    pHead->DataCrc16 = dcf_tools_crc16_get(pData + sizeof(CRccHeader), pHead->PackageLen);
    pHead->dwMagic = RCCFLAG_HOST;
    pHead->Msgtype = msgType;
    pHead->wVer = RCC_VER;
    pHead->Frame_Secret_ver = RCC_CRY_VER_FRAME;
    pHead->HeaderCRC16 = 0;
    // 对头部进行CRC16计算
    pHead->HeaderCRC16 = dcf_tools_crc16_get(pData, sizeof(CRccHeader));

    MsgLen = pHead->PackageLen + RCC_SIZE_HEADER;
    // 转换字节序
    if (!bHost) {
        WORD wOffset = 0;
        (void) dcf_pkg_tool::cvt_fmt_hton(pData, struct_fmt_rcc_header, pData, wOffset, sizeof(CRccHeader), sizeof(CRccHeader));
    }
}

/****************************************************************
*功能描述 : 解包头部
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-20  15:30:3
****************************************************************/
void CRccHeaderHelper::unpackage_header(BYTE *pData, BYTE &bHost) {
            ASSERT(pData != NULL);
    CRccHeader *pHead = (CRccHeader *) pData;

    bHost = 1;
    if (pHead->dwMagic != RCCFLAG_HOST) {
        bHost = 0;
        WORD wOffset = 0;
        (void) dcf_pkg_tool::cvt_fmt_ntoh(pData, struct_fmt_rcc_header, pData, wOffset, sizeof(CRccHeader), sizeof(CRccHeader));
    }
}


union FRM_MAGIC {
    CHAR chMagic[2];  /* 魔术字 "FM"*/
    WORD wMagic;
};

const FRM_MAGIC FMFLAG_BIG = {'F', 'M'};
const WORD FMFLAG_HOST = (((WORD) 'F') << 8) | ((WORD) 'M');

WORD CRccFrameHelper::get_rcc_magic_flag_host() {
    return FMFLAG_HOST;
}

WORD CRccFrameHelper::get_rcc_magic_flag_net() {
    return FMFLAG_BIG.wMagic;
}

bool CRccFrameHelper::IsValidHead(BYTE *pFrameData) {
    CRccFrame *pFrame = (CRccFrame *) pFrameData;
    if ((pFrame->Flag != FMFLAG_HOST) && (pFrame->Flag != FMFLAG_BIG.wMagic)) {
        return false;
    }

    return true;
}

/****************************************************************
*功能描述 : 返回frame的首地址
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-23  9:54:7
****************************************************************/
BYTE *CRccFrameHelper::GetBufHeader(BYTE *pBuf) {
    return pBuf + RCC_SIZE_HEADER;
}

/****************************************************************
*功能描述 : frame层头部封装
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-20  16:22:50
****************************************************************/
void CRccFrameHelper::package_header(BYTE *pframeData, const char *key, BYTE bHost) {
    // 转换字节序
    if (!bHost) {
        WORD wOffset = 0;
        (void) dcf_pkg_tool::cvt_fmt_hton(pframeData, struct_fmt_rcc_frame, pframeData, wOffset, sizeof(CRccFrame), sizeof(CRccFrame));
    }

    // 加密frame层
    if (key) {
        DWORD dwOutLen = sizeof(CRccFrame);
        dcf_tools_ces128_encrypt(pframeData, sizeof(CRccFrame), pframeData, key);
    }
}

/****************************************************************
*功能描述 : frame层解封装
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-20  16:23:17
****************************************************************/
void CRccFrameHelper::unpackage_header(BYTE *pframeData, const char *key, BYTE &bHost) {
    // 解密frame层
    if (key) {
        dcf_tools_ces128_decrypt(pframeData, sizeof(CRccFrame), pframeData, key);
    }

    CRccFrame *pFrame = (CRccFrame *) pframeData;
    bHost = 1;
    if (pFrame->Flag != FMFLAG_HOST) {
        bHost = 0;
        WORD wOffset = 0;
        (void) dcf_pkg_tool::cvt_fmt_ntoh(pframeData, struct_fmt_rcc_frame, pframeData, wOffset, sizeof(CRccFrame), sizeof(CRccFrame));
    }
}

DWORD CRawMsgHelper::InitMsgBuffer(BYTE *pMsg, BYTE ctrl, WORD TotalLen, WORD DataType, DWORD cirtifyid) {
    CATCH_ERR_RET(TotalLen < RCC_SIZE_RAW_HEAD, DCF_ERR_PARAM);
    CRccHeader *pHead = (CRccHeader *) pMsg;
    memset(pMsg, 0, TotalLen);
    pHead->dwMagic = RCCFLAG_HOST;
    TotalLen = TotalLen - RCC_SIZE_HEADER;
    pHead->wMsgLen = (TotalLen / 16) * 16;
    pHead->PackageLen = pHead->wMsgLen;
    CRccFrame *pFrame = (CRccFrame *) CRccFrameHelper::GetBufHeader(pMsg);
    pFrame->Flag = FMFLAG_HOST;
    pFrame->ctrl = ctrl;
    pFrame->wMsg1 = DataType;
    pFrame->cirtifyid = cirtifyid;
    return 0;
}


/****************************************************************
*功能描述 : TLV命令打包
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-22  16:28:2
****************************************************************/
DWORD CRawMsgHelper::AddToPacket(BYTE *pMsg, WORD TotalLen, WORD &Offset, CRM_CMD &cmd, BYTE bHost) {
    CATCH_ERR_RET(!pMsg, DCF_ERR_PARAM);
    CATCH_ERR_RET(TotalLen < RCC_SIZE_RAW_ONETLV_MIN, DCF_ERR_PARAM);

    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg + RCC_SIZE_RAW_HEAD);
    if ((!Offset) || (!pCrmHead->CmdNums)) {
        // 是第一包
        pCrmHead->CmdNums = 0;
        pCrmHead->Len = RCC_SIZE_CRM_HEAD;
        Offset = RCC_SIZE_RAW_HEAD + RCC_SIZE_CRM_HEAD;
    }

    if ((Offset + RCC_SIZE_CRM_TLV + cmd.wParaLen) > TotalLen) {
        // 无法填下命令
        return DCF_ERR_FULL;
    }

    // 1.转换命令头
    BYTE *pCur = pMsg + Offset;
    DWORD dwRet = 0;

    // 使用网络字节序打包
    if (bHost) {
        // 使用主机序打包,完全无需转换
        memcpy(pCur, &cmd, RCC_SIZE_CRM_TLV);
        pCur += RCC_SIZE_CRM_TLV;
        memcpy(pCur, cmd.pbyPara, cmd.wParaLen);
        Offset += RCC_SIZE_CRM_TLV + cmd.wParaLen;
    } else {
        dwRet = dcf_pkg_tool::cvt_tlv_hton(cmd, pCur, Offset, TotalLen - Offset);
    }

    // 3.填写Head
    pCrmHead->CmdNums++;
    pCrmHead->Len += RCC_SIZE_CRM_TLV + cmd.wParaLen;
    return dwRet;
}

/****************************************************************
*功能描述 : 从消息中取命令
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-22  16:47:8
****************************************************************/
DWORD CRawMsgHelper::GetFromPacket(BYTE *pMsg, WORD TotalLen, WORD &Offset, CRM_CMD &cmd, BYTE bHost) {
    CATCH_ERR_RET(!pMsg, DCF_ERR_PARAM);
    CATCH_ERR_RET(TotalLen < RCC_SIZE_RAW_ONETLV_MIN, DCF_ERR_PARAM);

    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg + RCC_SIZE_RAW_HEAD);
    if (!Offset) {
        // 是第一包
        Offset = RCC_SIZE_RAW_HEAD;
        // CRM包头转换在 DecryptFrame 中处理了
        Offset += RCC_SIZE_CRM_HEAD;
    }

    if (!pCrmHead->CmdNums) {
        // 没有命令了
        return RCC_ERR_EMPTY;
    }

    // 1.命令计数消减
    pCrmHead->CmdNums--;

    // 2.获取TLV头部
    BYTE *pCur = pMsg + Offset;
    if (bHost) {

        memcpy(&cmd, pCur, RCC_SIZE_CRM_TLV);
        Offset += (RCC_SIZE_CRM_TLV + cmd.wParaLen);

//        dcf_para_out out(cmd.pbyPara, cmd.wParaLen);
//        WORD error = 0;
//        out >> error;
//        if (error) {
//            dcf_output("登录错误(%d)", error);
//        }

        dcf_output("解析数据Offset:[%d]-TotalLen[%d]", Offset,TotalLen);

        if (Offset > TotalLen) {
            return DCF_ERR_EMPTY;
        }
        pCur += RCC_SIZE_CRM_TLV;
        cmd.pbyPara = pCur;
        return 0;
    } else {
        return dcf_pkg_tool::cvt_tlv_ntoh(pCur, cmd, Offset, TotalLen - Offset);
    }
}

/****************************************************************
*功能描述 : 封装TLV包头，并对应用层加密
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-23  9:11:48
****************************************************************/
void CRawMsgHelper::PacketFrame(BYTE *pMsg, WORD &TotalLen, const char *key, BYTE bHost) {
            ASSERT(pMsg != NULL);
    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg + RCC_SIZE_RAW_HEAD);
    WORD Len = pCrmHead->Len;
    // 1.先对头部进行转换
    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_hton((BYTE *) pCrmHead, struct_fmt_crm_header, (BYTE *) pCrmHead, Offset, RCC_SIZE_CRM_HEAD, RCC_SIZE_CRM_HEAD);
    }

    // 2.填写数据
    CRccHeader *pHead = (CRccHeader *) pMsg;
    if (key) {
        DWORD crylen = ((Len + 15) / 16) * 16;
        if (pHead->PackageLen >= (crylen + RCC_SIZE_FRAME)) {
            dcf_tools_ces128_encrypt((BYTE *) pCrmHead, crylen, (BYTE *) pCrmHead, key); // zjb p2 Len<->crylen
            pHead->PackageLen = (WORD) crylen + RCC_SIZE_FRAME;
            pHead->wMsgLen = Len + RCC_SIZE_FRAME;
        } else {
            dcf_output("error data 1\r\n");
        }
    } else {
        pHead->PackageLen = (WORD) Len + RCC_SIZE_FRAME;
        pHead->wMsgLen = Len + RCC_SIZE_FRAME;
    }

    TotalLen = pHead->wMsgLen;
}

/****************************************************************
*功能描述 : 解密再解TLV包头
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-23  9:11:48
****************************************************************/
void CRawMsgHelper::UnpacketFrame(BYTE *pMsg, const char *key, BYTE bHost) {
            ASSERT(pMsg != NULL);

    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg + RCC_SIZE_RAW_HEAD);
    CRccHeader *pHead = (CRccHeader *) pMsg;
    // 1.先解密
    if (key) {
        dcf_tools_ces128_decrypt((BYTE *) pCrmHead, pHead->PackageLen - RCC_SIZE_FRAME, (BYTE *) pCrmHead, key);
    }

    // 2.转换包头
    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_ntoh((BYTE *) pCrmHead, struct_fmt_crm_header, (BYTE *) pCrmHead, Offset, RCC_SIZE_CRM_HEAD, RCC_SIZE_CRM_HEAD);
    }
}

/****************************************************************
*功能描述 : 邮箱头部的信息字节序主机序转网络序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-26  11:49:22
****************************************************************/
void CMBMsgHelper::cvt_hton(BYTE *pMbHeader) {
    WORD Offset = 0;
    (void) dcf_pkg_tool::cvt_fmt_hton((BYTE *) pMbHeader, struct_fmt_rcc_mailbox, (BYTE *) pMbHeader, Offset, RCC_SIZE_MB_HEAD, RCC_SIZE_MB_HEAD);
}

/****************************************************************
*功能描述 : 邮箱头部的信息字节序主机序转网络序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-26  13:49:22
****************************************************************/
void CMBMsgHelper::cvt_ntoh(BYTE *pMbHeader) {
    WORD Offset = 0;
    (void) dcf_pkg_tool::cvt_fmt_ntoh((BYTE *) pMbHeader, struct_fmt_rcc_mailbox, (BYTE *) pMbHeader, Offset, RCC_SIZE_MB_HEAD, RCC_SIZE_MB_HEAD);
}

/****************************************************************
*功能描述 : 打包函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-23  13:54:54
****************************************************************/
DWORD CCRMMsgHelper::AddToPacket(BYTE *pMsg, WORD MsgLen, WORD &Offset, CRM_CMD &cmd, BYTE bHost) {
    CATCH_ERR_RET(!pMsg, DCF_ERR_PARAM);
    CATCH_ERR_RET(MsgLen < CRM_SIZE_ONETLV_MIN, DCF_ERR_PARAM);

    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg);
    if ((!Offset) || (!pCrmHead->CmdNums)) {
        // 是第一包
        pCrmHead->CmdNums = 0;
        pCrmHead->Len = RCC_SIZE_CRM_HEAD;
        Offset = RCC_SIZE_CRM_HEAD;
    }

    if ((Offset + RCC_SIZE_CRM_TLV + cmd.wParaLen) > MsgLen) {
        // 无法填下命令
        return DCF_ERR_FULL;
    }

    // 1.转换命令头
    BYTE *pCur = pMsg + Offset;
    DWORD dwRet = 0;

    // 使用网络字节序打包
    if (bHost) {
        // 使用主机序打包,完全无需转换
        memcpy(pCur, &cmd, RCC_SIZE_CRM_TLV);
        pCur += RCC_SIZE_CRM_TLV;
        if (cmd.pbyPara && cmd.wParaLen) {
            memcpy(pCur, cmd.pbyPara, cmd.wParaLen);
            Offset += RCC_SIZE_CRM_TLV + cmd.wParaLen;
        }
    } else {
        dwRet = dcf_pkg_tool::cvt_tlv_hton(cmd, pCur, Offset, MsgLen - Offset);
    }

    // 3.填写Head
    pCrmHead->CmdNums++;
    pCrmHead->Len += RCC_SIZE_CRM_TLV + cmd.wParaLen;
    return dwRet;
}

/****************************************************************
*功能描述 : 解包
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-23  14:0:5
****************************************************************/
DWORD CCRMMsgHelper::GetFromPacket(BYTE *pMsg, WORD MsgLen, WORD &Offset, CRM_CMD &cmd, BYTE bHost) {
    CATCH_ERR_RET(!pMsg, DCF_ERR_PARAM);
    CATCH_ERR_RET(MsgLen < CRM_SIZE_ONETLV_MIN, DCF_ERR_PARAM);

    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg);
    if (!Offset) {
        // 是第一包
        // CRM包头转换在 DecryptFrame 中处理了
        Offset = RCC_SIZE_CRM_HEAD;
    }

    if (!pCrmHead->CmdNums) {
        // 没有命令了
        return DCF_ERR_EMPTY;
    }

    if ((Offset + RCC_SIZE_CRM_TLV + cmd.wParaLen) > MsgLen) {
        return DCF_ERR_INVALID_DATA;
    }

    // 1.命令计数消减
    pCrmHead->CmdNums--;

    // 2.获取TLV头部
    BYTE *pCur = pMsg + Offset;
    if (bHost) {
        memcpy(&cmd, pCur, RCC_SIZE_CRM_TLV);
        pCur += RCC_SIZE_CRM_TLV;
        cmd.pbyPara = pCur;
        Offset += RCC_SIZE_CRM_TLV + cmd.wParaLen;
        return 0;
    } else {
        return dcf_pkg_tool::cvt_tlv_ntoh(pCur, cmd, Offset, MsgLen - Offset);
    }
}

/****************************************************************
*功能描述 : 将报文最后封帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-23  14:2:37
****************************************************************/
void CCRMMsgHelper::PacketFrame(BYTE *pMsg, WORD buf_len, WORD &MsgLen, const char *key, BYTE bHost) {
            ASSERT(pMsg != NULL);
    CRM_HEAD *pCrmHead = (CRM_HEAD *) (pMsg);
    WORD Len = pCrmHead->Len;
    // 1.先对头部进行转换
    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_hton((BYTE *) pCrmHead, struct_fmt_crm_header, (BYTE *) pCrmHead, Offset, RCC_SIZE_CRM_HEAD, RCC_SIZE_CRM_HEAD);
    }

    // 2.填写数据
    if (key) {
        DWORD crylen = ((Len + 15) / 16) * 16;
        if (buf_len >= crylen) {
            dcf_tools_ces128_encrypt((BYTE *) pCrmHead, crylen, (BYTE *) pCrmHead, key); // zjb p2 Len<->crylen
            MsgLen = (WORD) crylen;
        } else {
            dcf_output("error data 3\r\n");
        }
    } else {
        MsgLen = (WORD) Len;
    }
}

/****************************************************************
*功能描述 : 解帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-23  14:9:11
****************************************************************/
void CCRMMsgHelper::UnpacketFrame(BYTE *pMsg, const char *key, WORD MsgLen, BYTE bHost) {
            ASSERT(pMsg != NULL);

    // 1.先解密
    if (key) {
        dcf_tools_ces128_decrypt(pMsg, MsgLen, pMsg, key);
    }

    // 2.转换包头
    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_ntoh(pMsg, struct_fmt_crm_header, pMsg, Offset, RCC_SIZE_CRM_HEAD, RCC_SIZE_CRM_HEAD);
    }
}


void CBDTMsgHelper::InitMsgBuffer(BYTE *pMsg, WORD ctrl, WORD cmd, DWORD buf_len) {
            ASSERT(buf_len >= RCC_SIZE_BIG_HEAD);
    BDT_TLV *pHead = (BDT_TLV *) pMsg;
    pHead->wCmdIdx = 0;
    pHead->wCtrl = ctrl;
    pHead->wCmd = cmd;
    pHead->wParaExt = 0;
    pHead->DataLen = buf_len - RCC_SIZE_BIG_HEAD;   /* 初始化为buf_len */
}

void CBDTMsgHelper::PacketFrame(BYTE *pMsg, DWORD buf_len, DWORD &MsgLen, const char *key, BYTE bHost) {
            ASSERT(buf_len >= RCC_SIZE_BIG_HEAD);
    BDT_TLV *pHead = (BDT_TLV *) pMsg;
    DWORD Len = pHead->DataLen + RCC_SIZE_BIG_HEAD;

    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_hton(pMsg, struct_fmt_bdt_header, pMsg, Offset, RCC_SIZE_BIG_HEAD, RCC_SIZE_BIG_HEAD);
    }

    if (key) {
        DWORD crylen = ((Len + 15) / 16) * 16;
        if (buf_len >= crylen) {
            dcf_tools_ces128_encrypt(pMsg, crylen, pMsg, key); // zjb p2 Len<->crylen
            MsgLen = (WORD) crylen;
        } else {
            dcf_output("error data 4\r\n");
        }
    } else {
        MsgLen = (WORD) Len;
    }
}

void CBDTMsgHelper::UnpacketFrame(BYTE *pMsg, const char *key, DWORD MsgLen, BYTE bHost) {
            ASSERT(pMsg != NULL);

    // 1.先解密
    if (key) {
        dcf_tools_ces128_decrypt(pMsg, MsgLen, pMsg, key);
    }

    // 2.转换包头
    if (!bHost) {
        WORD Offset = 0;
        (void) dcf_pkg_tool::cvt_fmt_ntoh(pMsg, struct_fmt_bdt_header, pMsg, Offset, RCC_SIZE_BIG_HEAD, RCC_SIZE_BIG_HEAD);
    }
}

void CBDTMsgHelper::GetFromPacket(BYTE *pMsg, BDT_TLV &tlv) {
    memcpy(&tlv, pMsg, RCC_SIZE_BIG_HEAD);
    tlv.pPara = pMsg + RCC_SIZE_BIG_HEAD;
}



