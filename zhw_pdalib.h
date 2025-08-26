#pragma once
/****************************************************************
*文件范围 : 本文件是提供给PDA端的接口头文件
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-06  10:17:42
****************************************************************/
#ifdef PDALIB_EXPORTS
#define PDALIB_API __declspec(dllexport)
#else
#define PDALIB_API __declspec(dllimport)
#endif

#ifndef ZHW_PDALIB_H
#define ZHW_PDALIB_H
#include "dcf_def.h"
#include "dcf_i_rcc_dth.h"
#include "dcf_string.h"
#include "extern_hook_api.h"
#endif //ZHW_PDALIB_H

const int RESP_MSG_RPC = 0;
const int RESP_DATABASE_IDX = 1;
const int RESP_LOGOUT = 2;
const int RESP_PROGRESS = 3;

typedef struct DatabaseIdx
{
    DWORD baseDbIdx;
    DWORD psDbIdx;
} DatabaseIdx;

typedef struct OnProgress
{
    WORD modId;
    WORD curBegin;
    WORD totalFrame;
} OnProgress;

typedef struct FuncList
{
    WORD length;
    WORD *list;
} FuncList;

/*
这个是企业终端和企业服务器间通信层的组件初始化
1.参数说明:
   local_comm_config:本地ip和端口配置,如"192.168.0.23:3521"
   bs_comm_config:   本地连接bs的端口配置,如"192.168.0.24:3520"
   dev_config:设备用户信息(用户名,密码),如"jiadebao,13612345678"
*/
DWORD zhw_bc_initialize(
        const char *local_comm_config,
        const char *bs_comm_config,
        const char *username,
        const char *password
);
/* 设备退出 */
DWORD zhw_bc_quit();
/* 查询设备状态 0:未初始化 1:正在连接 2:连接正常*/
DWORD zhw_bc_get_server_state();
/* 发送保活帧 */
bool zhw_bc_active_server();
void zhw_bc_recv_callback(MSG_RPC &msg);
void zhw_bc_recv_db_idx(DWORD baseDbIdx, DWORD psDbIdx);
void zhw_bc_recv_progress(WORD modId, WORD curBegin, WORD totalFrame); //add by hhw 2023.12.4 用于pda数据发送数据时上报进度
/* 
发送消息 :
msg:消息内容
msg_len:消息长度
msg_type:消息类型
*/
DWORD zhw_bc_send_msg(const unsigned char *msg, unsigned int msg_len, unsigned short msg_type,
                      unsigned short msg1, unsigned short msg2);
DWORD zhw_bc_send_msg(MSG_RPC &msg, DWORD *pSurredtHandle = NULL, DWORD timeout = 0/* 总超时 */,
                      DWORD resendtimes = 0/* 重传次数 */);
DWORD zhw_bc_direct_send_msg(void *pMsg, DWORD dstIP, WORD Port);

/*
这个是企业服务器和喆道服务器间通信层的组件初始化
    key_pub_com_bsg : BSG通信公钥
    key_pub_com_bss : BSS通信公钥
    key_pub_com_bss : BS自己的通信私钥
    key_pub_com_qrcode : QRCODE公钥
    key_self_qrcode_bs : 企业设备的QRCODE私钥
*/
DWORD zhw_bs_initialize(
        const char* recv_addr,
        const char* gateway_addr,
        const char* dev_name,
        const char* dev_cirtify,
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

);
/* 设备退出 */
DWORD zhw_bs_quit();
/* 查询设备状态 0:未初始化 1:正在连接 2:连接正常*/
DWORD zhw_bs_get_server_state();
/* 查询功能列表 */
WORD zhw_bs_get_get_func_list(WORD *funcList);
/* 发送保活帧 */
bool zhw_bs_active_server();
void zhw_bs_recv_db_idx(DWORD baseDbIdx, DWORD psDbIdx);
void zhw_bs_recv_callback(MSG_RPC &msg);

//退出登录
void zhw_bs_recv_logout();
/*
发送消息 :
msg:消息内容
msg_len:消息长度
msg_type:消息类型
*/
DWORD zhw_bs_send_msg(MSG_RPC &msg, DWORD *pSurredtHandle = NULL, DWORD timeout = 0/* 总超时 */,
                      DWORD resendtimes = 0/* 重传次数 */);
DWORD zhw_bs_direct_send_msg(void *pMsg, DWORD dstIP, WORD Port);

DWORD batches_sendRawMessage(DWORD fileLen_J,DWORD size, WORD cmd_I, BYTE *pData, WORD infoType_I, WORD modId_I, DWORD dwRet);