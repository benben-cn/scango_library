//
// Created by zhedao on 2023/2/1.
//

#ifndef ZHEDAO_DCFRAME_PROC_H
#define ZHEDAO_DCFRAME_PROC_H

#include "dcf_cmdid.h"
#include "dcf_pkghelper.h"
#include "dcf_rcc_dthelper.h"
#include "dcf_zlib.h"
#include "extern_api.h"
#include <pthread.h>

#include "scango_dll.h"

#endif

extern pthread_mutex_t mut_callback;
extern onLogout gonLogout;
extern onDatabaseIdxRecv gonDatabaseIdxRecv;
extern onError gonError;
extern onLoginError gonLoginError;
extern onProgress gonProgress;
extern onCRMRecv gonCRMRecv;
extern newTempFile gnewTempFile;
extern onFileRecv gonFileRecv;

void bs_proc_resp(MSG_RPC *msg);

void ProcDatabaseIdx(DWORD baseDbIdx, DWORD psDbIdx);

void ProcLogout();

void loginError(WORD);

void ProcRsp(MSG_RPC *pRPCMsg);
//void ProcRsp_Other(MSG_RPC *pRPCMsg);
void ProcRsp_CMD(WORD modId, CRM_CMD &Rsp);
void ProcRsp_DBFile(WORD modId, BDT_TLV &tlv, BYTE bHost);
void ProcRsp_DBIndex(WORD modId, CRM_CMD &Rsp);
void ProcOnProgress(WORD modId ,WORD curBegin , WORD totalFrame); //add by hhw 2023.12.4 用于pda数据发送数据时上报进度

void ProcQRGetRsp_GetGDSI(WORD modId, CRM_CMD &Rsp);
void ProcQRGetRsp_GetQrRelated_Simple(WORD modId, CRM_CMD &Rsp);
void ProcQRGetRsp_GetQrRelated_Detail(WORD modId, CRM_CMD &Rsp);
void ProcQRGetRsp_GetQrRelated_SmallFindBig(WORD modId, CRM_CMD &Rsp);
void ProcQRGetRsp_GetQrInfo_Detail(WORD modId, CRM_CMD &Rsp);
void ProcQRGetRsp_GetQrRelated_3RD(WORD modId, CRM_CMD &Rsp);

/*
 * 农业版
 */

void ProcFieldRsp_AllocID(WORD modId, CRM_CMD &Rsp);

void ProcQualityReport_ContainerCodeQuery(WORD modId, CRM_CMD &Cmd);
void ProcFarmCodeRelationRsp_SaleCodeQuery(WORD modId, CRM_CMD &Cmd);
void ProcFarmCodeRelationRsp_ContainerCodeQuery(WORD modId, CRM_CMD &Cmd);

/*
 * 资产版
 */
void ProcAssetRsp_query(WORD modId, CRM_CMD &Rsp);
