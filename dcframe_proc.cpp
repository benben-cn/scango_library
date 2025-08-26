//
// Created by zhedao on 2023/2/1.
//
#include "dcframe_proc.h"
#include <iostream>

pthread_mutex_t mut_callback;
onLogout gonLogout;
onDatabaseIdxRecv gonDatabaseIdxRecv;
onError gonError;
onLoginError gonLoginError;
onProgress gonProgress;
onCRMRecv gonCRMRecv;
newTempFile gnewTempFile;
onFileRecv gonFileRecv;

void ProcDatabaseIdx(DWORD baseDbIdx, DWORD psDbIdx) {

    pthread_mutex_lock(&mut_callback);

    //Page_Login::m_Login->CallbackZHWLogin(baseDbIdx,psDbIdx);

    //if (callback_jobject) {
    //    JNIEnv *env = NULL;
    //    if (getJavaVM()->AttachCurrentThread(&env, NULL) == JNI_OK) {
    //        callback_jclass = env->GetObjectClass(callback_jobject);
    //        jmethodID onDatabaseIdxRecv_jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_DB_IDX_RECV, JMETHOD_SIGN_ON_DB_IDX_RECV);

    //        env->CallVoidMethod(callback_jobject, onDatabaseIdxRecv_jmethod, (jlong) baseDbIdx, (jlong) psDbIdx);

    //        env->DeleteLocalRef(callback_jclass);
    //        callback_jclass = NULL;

    //        getJavaVM()->DetachCurrentThread();
    //    }
    //}
    if (gonDatabaseIdxRecv) {
        gonDatabaseIdxRecv(baseDbIdx, psDbIdx); // 调用回调函数
    }

    pthread_mutex_unlock(&mut_callback);
}

void bs_proc_resp(MSG_RPC *msg) {
    pthread_mutex_lock(&mut_callback);

    ProcRsp(msg);
    /* 释放内存 */
    dcf_mem_free((void *&) msg->pMsg);
    dcf_mem_free((void *&) msg);

    pthread_mutex_unlock(&mut_callback);
}

/****************************************************************
*功能描述 : 处理向企业服务端订阅回来的响应消息
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-22  23:43:20
****************************************************************/
void ProcRsp(MSG_RPC *pRPCMsg) {
    // 校验一下数据是否正确
    if (!pRPCMsg->pMsg) {
        dcf_output("recv error packet!\r\n");
        return;
    }

    BYTE *pMsg = pRPCMsg->pMsg;
    if (pRPCMsg->wMsgType == RCC_MSG_TYPE_RAW_BC_BIGDATA || pRPCMsg->wMsgType == RCC_MSG_TYPE_RAW_BSS_BIGDATA) {
        BDT_TLV tlv;
        /* 1.先要进行字节序转换 */
        CBDTMsgHelper::UnpacketFrame(pMsg, NULL, pRPCMsg->MsgLen, pRPCMsg->bhost);
        /* 2.取大数据报文中的 */
        CBDTMsgHelper::GetFromPacket(pMsg, tlv);
        switch (tlv.wCmd) {
            case MOD_CMD_BS_DB_QUERY_BS:
            case MOD_CMD_BSS_DB_QUERY_BSS:
            //case MOD_CMD_BSS_DB_QUERY_QR:
            case MOD_CMD_BS_IN_QUERYNEW:
            case MOD_CMD_BS_STOCK_QUERY:
            case MOD_CMD_BS_DB_QUERY_PS:
            case MOD_CMD_BSS_PSDB_QUERY:
            case MOD_CMD_BSS_QUERY_FARM_DB:
            case MOD_CMD_BSS_DB_PLINE_QR:
                ProcRsp_DBFile(pRPCMsg->wMsg2, tlv, pRPCMsg->bhost);
                break;
            default:
                dcf_output("recv unknown cmd:0x%x\r\n", tlv.wCmd);
                //ProcRsp_Other(pRPCMsg);
                break;
        }
    } else {
        CRM_CMD Rsp = {0};
        WORD Offset = 0;
        DWORD dwRet = CCRMMsgHelper::GetFromPacket(pMsg, (WORD) pRPCMsg->MsgLen, Offset, Rsp, pRPCMsg->bhost);
        dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, pRPCMsg->wMsg2, "GetFromPacket");
        if (!dwRet) {
            switch (Rsp.wCmd) {
                case MOD_CMD_BS_DB_QUERY_BS:
                case MOD_CMD_BSS_DB_QUERY_BSS:
                //case MOD_CMD_BSS_DB_QUERY_QR:
                case MOD_CMD_BSS_QUERY_FARM_DB:
                case MOD_CMD_BS_DB_QUERY_PS:
                case MOD_CMD_BSS_PSDB_QUERY:
                case MOD_CMD_BSS_DB_PLINE_QR:
                    ProcRsp_DBIndex(pRPCMsg->wMsg2, Rsp);
                    break;
                case MOD_CMD_BS_IN_QUERYNEW:
                case MOD_CMD_BS_STOCK_QUERY:
                case MOD_CMD_BSS_IN_CMT_F:
                case MOD_CMD_BS_IN_COMMIT:
                case MOD_CMD_BS_COMB_SPLIT:
                case MOD_CMD_BS_DAMAGED:
                case MOD_CMD_BSS_BRS_MODIFY_F:
                case MOD_CMD_BSS_DAMAGED:
                case MOD_CMD_BSS_BRS_DELETE:
                case MOD_CMD_BSS_PIN_CMT:
                case MOD_CMD_BSS_CANCEL_IN_CMT:
                case MOD_CMD_BS_STOCK_OUT:
                case MOD_CMD_BSS_OUT_CMT:
                case MOD_CMD_BSS_PCRG_CREATE_P:
                case MOD_CMD_BSS_PCRG_CREATE_C:
                case MOD_CMD_BSS_PLANTAREA:
                case MOD_CMD_BSS_PLANTAREA_DEL:
                case MOD_CMD_BSS_GROWTH:
                case MOD_CMD_BSS_GROWTH_DEL:
                case MOD_CMD_BSS_GROWTH_FINISH:
                case MOD_CMD_BSS_GROWING_PHOTO:
                case MOD_CMD_BSS_GROWING_PHOTO_DEL:
                case MOD_CMD_BSS_PLANT_LOG:
                case MOD_CMD_BSS_PLANT_LOG_DEL:
                case MOD_CMD_BSS_HARVEST:
                case MOD_CMD_BSS_HARVEST_DEL:
                case MOD_CMD_BSS_UP_ASSERTS:
                case MOD_CMD_BSS_PCI_QT_COMMIT:
                case MOD_CMD_BSS_PCI_RELATION:
                //case MOD_CMD_BSS_IN_CMT_TASK:
                //case MOD_CMD_BSS_IN_CMT_S:
                    ProcRsp_CMD(pRPCMsg->wMsg2, Rsp);
                    break;
                //case MOD_CMD_BSS_QRGET_GDSI:
                //    ProcQRGetRsp_GetGDSI(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_QRGET_RELATED_S:
                //    ProcQRGetRsp_GetQrRelated_Simple(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_QRGET_RELATED_D:
                //    ProcQRGetRsp_GetQrRelated_Detail(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_QRGET_3RD://与服务端数据传输格式对不上
                //    ProcQRGetRsp_GetQrRelated_3RD(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_QRGET_RELATED_SFB_D:
                //    ProcQRGetRsp_GetQrRelated_SmallFindBig(pRPCMsg->wMsg2, Rsp);
                //    break;
                ////case MOD_CMD_BSS_QRINFO_RELATED_INDEX: //服务器端没有这个命令
                //    ProcQRGetRsp_GetQrInfo_Detail(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_AREA_PRE_ALLOC:
                //    ProcFieldRsp_AllocID(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_QUREY_ASSETS:
                //    ProcAssetRsp_query(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_PCI_QT_QUERY:
                //    ProcQualityReport_ContainerCodeQuery(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_RQUERY_BYQR_C:
                //    ProcFarmCodeRelationRsp_ContainerCodeQuery(pRPCMsg->wMsg2, Rsp);
                //    break;
                //case MOD_CMD_BSS_RQUERY_BYQR_F:
                //    ProcFarmCodeRelationRsp_SaleCodeQuery(pRPCMsg->wMsg2, Rsp);
                //    break;
                default:
                    dcf_output("recv unknown cmd:0x%x\r\n", Rsp.wCmd);
                    //ProcRsp_Other(pRPCMsg);
                    break;
            }
        }
    }
}

//void ProcRsp_Other(MSG_RPC *pRPCMsg) {//当cmd控制字未知时的处理函数
    //jint ctrl = (jint) pRPCMsg->ctrl;
    //jint bhost = (jint) pRPCMsg->bhost;
    //jint msgSecretVer = (jint) pRPCMsg->Msg_Secret_Ver;
    //jint wMsg1 = (jint) pRPCMsg->wMsg1;
    //jint wMsg2 = (jint) pRPCMsg->wMsg2;
    //jint wMsgType = (jint) pRPCMsg->wMsgType;
    //jint msgPort = (jint) pRPCMsg->MsgPort;
    //jlong msgIP = (jlong) pRPCMsg->MsgIP;
    //jlong msgLen = (jlong) pRPCMsg->MsgLen;
    //jbyteArray msgBytes = toJbyteArray(env, pRPCMsg->pMsg, sizeof(pRPCMsg->pMsg));

    //jmethodID jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_MSG_RECV, JMETHOD_SIGN_ON_MSG_RECV);
    //env->CallVoidMethod(
    //        callback_jobject,
    //        jmethod,
    //        ctrl,
    //        bhost,
    //        msgSecretVer,
    //        wMsg1,
    //        wMsg2,
    //        wMsgType,
    //        msgPort,
    //        msgIP,
    //        msgLen,
    //        msgBytes
    //);
    //env->DeleteLocalRef(msgBytes);
//}

/**
 * 退出登录
 */
void ProcLogout() {//接收到退出登入命令时的处理函数
    pthread_mutex_lock(&mut_callback);

    //MainWindow::m_mainwindow->CallbackZHWLoginOut();

    //if (callback_jobject) {
    //    JNIEnv *env = NULL;
    //    if (getJavaVM()->AttachCurrentThread(&env, NULL) == JNI_OK) {
    //        callback_jclass = env->GetObjectClass(callback_jobject);
    //        jmethodID jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_LOGOUT, JMETHOD_ON_LOGOUT_RECV);

    //        env->CallVoidMethod(callback_jobject, jmethod);

    //        env->DeleteLocalRef(callback_jclass);
    //        callback_jclass = NULL;

    //        getJavaVM()->DetachCurrentThread();
    //    }
    //}
    if (gonLogout) {
        gonLogout(); // 调用回调函数
    }

    pthread_mutex_unlock(&mut_callback);
}

/**
 * 退出登录
 */
void loginError(WORD code) {//登入服务器失败后的处理函数
    pthread_mutex_lock(&mut_callback);

    //Page_Login::m_Login->CallbackZHWLoginErr(code);

    //if (callback_jobject) {
    //    JNIEnv *env = NULL;
    //    if (getJavaVM()->AttachCurrentThread(&env, NULL) == JNI_OK) {
    //        callback_jclass = env->GetObjectClass(callback_jobject);
    //        jmethodID jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_LOGIN_ERROR, JMETHOD_ON_LOGIN_ERROR_RECV);

    //        env->CallVoidMethod(callback_jobject, jmethod, code);

    //        env->DeleteLocalRef(callback_jclass);
    //        callback_jclass = NULL;

    //        getJavaVM()->DetachCurrentThread();
    //    }
    //}
    if (gonLoginError) {
        gonLoginError(code); // 调用回调函数
    }

    pthread_mutex_unlock(&mut_callback);
}

void ProcOnProgress(WORD modId ,WORD curBegin, WORD totalFrame) {
    pthread_mutex_lock(&mut_callback);
    //if (callback_jobject) {
    //    JNIEnv* env = NULL;
    //    if (getJavaVM()->AttachCurrentThread(&env, NULL) == JNI_OK) {
    //        callback_jclass = env->GetObjectClass(callback_jobject);
    //        jmethodID jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_Progress, JMETHOD_SIGN_ON_Progress);

    //        env->CallVoidMethod(callback_jobject, jmethod, modId, curBegin, totalFrame);

    //        env->DeleteLocalRef(callback_jclass);
    //        callback_jclass = NULL;

    //        getJavaVM()->DetachCurrentThread();
    //    }
    //}
    //dcf_output("[%s]ProcOnProgress modId[%u] curBegin[%u] totalFrame[%u]!\r\n", dcf_get_time_string_unsafe(), modId, curBegin, totalFrame);
    if (gonProgress) {
        gonProgress(modId, curBegin, totalFrame); // 调用回调函数
    }
    pthread_mutex_unlock(&mut_callback);
}

/*
 * 响应格式:<ok>
*/
void ProcRsp_CMD(WORD modId, CRM_CMD& Rsp) {
    dcf_para_out out(Rsp.pbyPara, Rsp.wParaLen);
    WORD error = 0;
    out >> error;
    if (out.have_error() || error) {
        dcf_output("[%s]操作失败(%d)\r\n", dcf_get_time_string_unsafe(), error);

        //jstring msg = env->NewStringUTF("操作失败");
        //env->CallVoidMethod(callback_jobject, onError_jmethod, modId, msg);
        //env->DeleteLocalRef(msg);
        char errmsg[128] = { 0 };
        sprintf_s(errmsg, 128, "The operation failed\r\n");
        if (gonError) {
            gonError(modId, errmsg); // 调用回调函数
        }

        return;
    }

    dcf_output("[%s]ProcRsp_CMD操作成功,wCmd[0x%x],modId[%u]\r\n", dcf_get_time_string_unsafe(), Rsp.wCmd, modId);
    //jmethodID jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_CRM_RECV, JMETHOD_SIGN_ON_CRM_RECV);
    //env->CallVoidMethod(callback_jobject, jmethod, modId, NULL);
    if (gonCRMRecv) {
        char cjson[20] = { 0 };
        sprintf_s(cjson, 20, "{\"modId\":%d}", modId);
        gonCRMRecv(cjson); // 调用回调函数
    }
}

/****************************************************************
*功能描述 : 大数据格式传输
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-12  14:16:58
*更新时间 : 2023-01-30  09:43:50
****************************************************************/
void ProcRsp_DBFile(WORD modId, BDT_TLV& tlv, BYTE bHost)
{
    //jmethodID jmethod;

    dcf_para_out out(tlv.pPara, tlv.DataLen);
    DWORD org_len = 0, zlib_len = 0;
    if (!bHost)
    {
        // 需要对压缩头部进行字节序转换
        WORD offset = 0;
        dcf_pkg_tool::cvt_fmt_ntoh(tlv.pPara, struct_fmt_gzip_header, tlv.pPara, offset, RCC_SIZE_GZIP_HEAD, RCC_SIZE_GZIP_HEAD);
    }

    /* 取结构体 */
    out >> org_len >> zlib_len;
    BYTE* byZlib = out.get_para_ptr(zlib_len);
    if ((out.have_error()) || (!byZlib))
    {
        dcf_output("[%s]无法读取数据\r\n", dcf_get_time_string_unsafe());

        //jstring msg = env->NewStringUTF("无法读取数据");
        //env->CallVoidMethod(callback_jobject, onError_jmethod, modId, msg);
        //env->DeleteLocalRef(msg);
        char errmsg[128] = { 0 };
        sprintf_s(errmsg, 128, "Unable to read data\r\n");
        if (gonError) {
            gonError(modId, errmsg); // 调用回调函数
        }

        return;
    }

    //pda创建文件并返回该文件路径,window需自己创建创建文件选择路径
    //jmethod = env->GetMethodID(callback_jclass, JMETHOD_NEW_TEMP_FILE, JMETHOD_SIGN_NEW_TEMP_FILE);
    //jstring dstFile = (jstring)env->CallObjectMethod(callback_jobject, jmethod, modId);
    //if (dstFile == NULL || env->GetStringLength(dstFile) == 0) {
    //    jstring msg = env->NewStringUTF("输出路径有误");
    //    env->CallVoidMethod(callback_jobject, onError_jmethod, modId, msg);

    //    env->DeleteLocalRef(dstFile);
    //    env->DeleteLocalRef(msg);

    //    return;
    //}
    //const char *dstfilename = env->GetStringUTFChars(dstFile, NULL);
    char* dstfilename = { 0 };
    if (gnewTempFile) {
        //gnewTempFile(modId, dstfilename, sizeof(dstfilename)); // 调用回调函数
        dstfilename = gnewTempFile(modId); // 调用回调函数
    }
    printf("---dstfilename:%s---\r\n", dstfilename);
    if (dstfilename == NULL || sizeof(dstfilename) == 0) {
        char errmsg[128] = { 0 };
        sprintf_s(errmsg, 128, "The output path is incorrect.\r\n");
        if (gonError) {
            gonError(modId, errmsg); // 调用回调函数
        }

        return;
    }
    DWORD dwRet = dcf_zlib_uncompress_mtof(byZlib, zlib_len, dstfilename);
    //env->ReleaseStringUTFChars(dstFile, dstfilename);
    dcf_sys_checkerr(dwRet, LEVEL_COMM_ERROR, modId, "dcf_zlib_uncompress_mtof");

    if (dwRet)
    {
        dcf_output("[%s]解压数据时出错(len:%d)\r\n", dcf_get_time_string_unsafe(), zlib_len);

        //jstring msg = env->NewStringUTF("解压数据时出错");
        //env->CallVoidMethod(callback_jobject, onError_jmethod, modId, msg);
        //env->DeleteLocalRef(msg);
        char errmsg[256] = { 0 };
        sprintf_s(errmsg, 256, "An error occurred while extracting the data,dstfilename:%s\r\n", dstfilename);
        if (gonError) {
            gonError(modId, errmsg); // 调用回调函数
        }
    }
    else {
        //把接收到的文件地址传递出去（或处理该文件）
        //jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_FILE_RECV, JMETHOD_SIGN_ON_FILE_RECV);
        //env->CallVoidMethod(callback_jobject, jmethod, modId, dstFile);
        if (gonFileRecv) {
            gonFileRecv(modId, dstfilename); // 调用回调函数
        }
    }

    //env->DeleteLocalRef(dstFile);
}

/****************************************************************
*功能描述 : BS以CRM报文格式返回查询结果
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-12  14:22:26
****************************************************************/
void ProcRsp_DBIndex(WORD modId, CRM_CMD& Rsp)
{
    dcf_para_out out(Rsp.pbyPara, Rsp.wParaLen);
    WORD error = 0;
    DWORD db_idx = 0;
    out >> error >> db_idx;
    if (out.have_error() || error)
    {
        dcf_output("[%s]同步{数据库}时出错\r\n", dcf_get_time_string_unsafe());
        char errmsg[128] = { 0 };
        sprintf_s(errmsg, 128, "Error with error code:%d\r\n", error);
        if (gonError) {
            gonError(modId, errmsg); // 调用回调函数
        }
        return;
    }

    //返回查询结果db_idx，db_idx干什么的？
    //jclass longCls = env->FindClass("java/lang/Long");
    //jobjectArray args = env->NewObjectArray(1, longCls, jlongToObj(env, db_idx));

    //jmethod = env->GetMethodID(callback_jclass, JMETHOD_ON_CRM_RECV, JMETHOD_SIGN_ON_CRM_RECV);
    //env->CallVoidMethod(callback_jobject, jmethod, modId, args);

    //env->DeleteLocalRef(longCls);
    //env->DeleteLocalRef(args);
    if (gonCRMRecv) {
        char cjson[64] = { 0 };
        sprintf_s(cjson, 20, "{\"modId\":%d,\"db_idx\":\"%u\"}", modId, db_idx);
        gonCRMRecv(cjson); // 调用回调函数
    }
}