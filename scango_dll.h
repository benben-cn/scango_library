#pragma once

#ifndef SCANGODLL_d
#define SCANGODLL_d

//宏定义导出
// #ifdef SCANGODLL__//如果没有定义DLLH 就定义 DLLH __declspec(dllexport)
// #define SCANGODLL __declspec(dllexport)//导出
// #else
// #define SCANGODLL __declspec(dllimport)//导入
// #endif // DLLH__//如果没有定义DLLH 就定义 DLLH 
#define SCANGODLL __attribute__((visibility("default")))

using namespace std;


#include<vector> 

typedef unsigned int       DWORD;
typedef unsigned short      WORD;

//一物一码解码函数返回结构体
struct qrcode_decrypt_data {
    DWORD table_id; //数据库svp_cfg_base_20170421001，bs_qr_records表id
    DWORD random; //产码时生成的随机数
};

//回调函数----------------------------------------------------------------------------------------
//退出登入
typedef void (*onLogout)();
extern SCANGODLL void SetonLogout(onLogout cb);
//登入成功后实时同步主数据
typedef void (*onDatabaseIdxRecv)(DWORD baseDbIdx, DWORD psDbIdx);
extern SCANGODLL void SetonDatabaseIdxRecv(onDatabaseIdxRecv cb);
//错误报错 modId_I：自定义模块id，errmsg：错误信息
typedef void (*onError)(WORD modId, char* errmsg);
extern SCANGODLL void SetonError(onError cb);
//登入失败报错 code：错误代码
typedef void (*onLoginError)(WORD code);
extern SCANGODLL void SetonLoginError(onLoginError cb);
//上传文件进度条 modId_I：自定义模块id，curBegin：当前进度，totalFrame：总进度
typedef void (*onProgress)(WORD modId, WORD curBegin, WORD totalFrame);
extern SCANGODLL void SetonProgress(onProgress cb);
//接收信息 cjson：json类型存储的服务器返回信息
typedef void (*onCRMRecv)(char* cjson);
extern SCANGODLL void SetonCRMRecv(onCRMRecv cb);
//通过模块id获取文件保存路径 返回值：文件保存的路径，modId：自定义模块id
typedef char* (*newTempFile)(WORD modId);
extern SCANGODLL void SetnewTempFile(newTempFile cb);
//文件保存后返回模块id以及文件完整路径 modId：自定义模块id，dstFile：文件完整路径
typedef void (*onFileRecv)(WORD modId, char* dstFile);
extern SCANGODLL void SetonFileRecv(onFileRecv cb);

//对外接口----------------------------------------------------------------------------------------
//设备登入
extern SCANGODLL DWORD smartdog_scango_dcframe_BSClient_init(
        const char* recv_addr,          //本机IP地址加空闲udp端口号，例（0.0.0.0:49152）
        const char* gateway_addr,       //网关地址:固定端口4021，例（xxx.xxx.xxx.xxx:4021）
        const char* dev_name,           //账号名称
        const char* dev_cirtify,        //MAC地址
        DWORD bs_id,                    //默认填0
        DWORD dev_id,                   //默认填0
        WORD dev_no,                    //默认填0
        DWORD dev_opt_related,          //默认填0
        const char* key_pub_com_bsg,    //默认填zhw_pubkey
        const char* key_pub_com_bss,    //默认填zhw_pubkey
        const char* key_self_com_bs,    //账号密码
        const char* key_pub_com_qrcode, //默认填“”
        const char* key_self_qrcode_bs, //默认填“”
        const char* version,            //软件版本号
        const char* address);           //网关地址
//退出登入
extern SCANGODLL DWORD smartdog_scango_dcframe_BSClient_quit();
//获取和服务端的状态 0:初始化，等待和网关通讯正常 1:正在登陆 2:通讯正常
extern SCANGODLL DWORD smartdog_scango_dcframe_BSClient_getServerState();
//获取设备功能权限
extern SCANGODLL vector<WORD> smartdog_scango_dcframe_BSClient_getFuncList();
//手动发送心跳，通讯正常返回true，通讯错误返回false（可忽略）
extern SCANGODLL bool smartdog_scango_dcframe_BSClient_sendHeartBeatPacket();
//发送信息 返回值0：发送成功，modId_I：自定义模块id，infoType_I：信息帧类型，cmd_I：各个模块的命令字定义，marker：json类型存储的信息
extern SCANGODLL DWORD smartdog_scango_dcframe_BSClient_sendCMD (WORD modId_I, WORD infoType_I, WORD cmd_I, const char* marker);
//上传文件 返回值0：发送成功，modId_I：自定义模块id，infoType_I：信息帧类型，cmd_I：各个模块的命令字定义，srcPath：文件路径，fileLen：文件大小
extern SCANGODLL DWORD smartdog_scango_dcframe_BSClient_sendRawMessage(WORD modId_I, WORD infoType_I, WORD cmd_I, const char* srcPath, DWORD fileLen);
//将任意长度的随机码转换为4位随机码
extern SCANGODLL DWORD smartdog_scango_qrcode_security_ZheDaoRandomCodeTool_convertRandomIdNew(DWORD randomId);
//8位一物一码解码
extern SCANGODLL qrcode_decrypt_data smartdog_scango_qrcode_ZheDaoQRCodeTool_nativeParseGoodsQRCodeNew(const char* fullQRCode);


/****************************************************************
*文件范围 : 各个模块的命令字定义，便于收发方对照解析
****************************************************************/

/* 网关支持的相关命令 */
//const WORD MOD_CMD_BSG_BEGIN = 0x200;
//const WORD MOD_CMD_BSG_LOGIN = MOD_CMD_BSG_BEGIN + 1;/* 登录网关命令 */
//const WORD MOD_CMD_BSG_LOGOUT = MOD_CMD_BSG_BEGIN + 2;/* 退出登录网关命令 */
//const WORD MOD_CMD_BSG_LOGIN_ERR = MOD_CMD_BSG_BEGIN + 3;/* 登录错误命令 */
//const WORD MOD_CMD_BSG_END = 0x210;

/* 服务端支持的相关命令 */
//const WORD MOD_CMD_BSS_BEGIN = 0x210;
//const WORD MOD_CMD_BSS_SYN_LOGDT = MOD_CMD_BSS_BEGIN + 1;   /* 同步客户端登录数据命令 */
//const WORD MOD_CMD_BSS_LOGIN = MOD_CMD_BSS_BEGIN + 2;   /* 登录服务进程命令 */
//const WORD MOD_CMD_BSS_ACTIVE = MOD_CMD_BSS_BEGIN + 3;   /* 企业设备和服务端保活命令 */
//const WORD MOD_CMD_BSS_LOGOUT = MOD_CMD_BSS_BEGIN + 4;   /* 退出登录服务进程命令 */
//const WORD MOD_CMD_BSS_END = 0x220;

/* 服务端支持的业务相关命令 */
/* 生产企业提交入库 */
//const WORD MOD_CMD_BSS_IN_BEGIN = 0x220;
//const WORD MOD_CMD_BSS_IN_CMT_F = MOD_CMD_BSS_IN_BEGIN + 1;   /* 生产企业批量提交入库数据 */
//const WORD MOD_CMD_BSS_IN_CMT_AF = MOD_CMD_BSS_IN_BEGIN + 2;   /* 生产企业批量自动提交入库数据 */
//const WORD MOD_CMD_BSS_IN_CMT_TASK = MOD_CMD_BSS_IN_BEGIN + 3;    /* PDA 提交单品码 */
//const WORD MOD_CMD_BSS_IN_CMT_S = MOD_CMD_BSS_IN_BEGIN + 4;    /* PDA 提交单品码 */
//const WORD MOD_CMD_BSS_IN_END = 0x230;

/* 企业服务器基本信息管理 */
//const WORD MOD_CMD_BSS_DB_BEGIN = 0x230;
//const WORD MOD_CMD_BSS_DB_QUERY_BSS = MOD_CMD_BSS_DB_BEGIN + 1; /* 获取基本信息 */
//const WORD MOD_CMD_BSS_DB_QUERY_QR = MOD_CMD_BSS_DB_BEGIN + 2; /* 获取QR所有信息 */
//const WORD MOD_CMD_BSS_DB_PLINE_QR = MOD_CMD_BSS_DB_BEGIN + 3;  /* 获取产线白码 */
//const WORD MOD_CMD_BSS_DB_END = 0x235;

/* 喆道处理企业出库操作 */
//const WORD MOD_CMD_BSS_OUT_BEGIN = 0x235;
//const WORD MOD_CMD_BSS_OUT_CMT = MOD_CMD_BSS_OUT_BEGIN + 1;         /* 出库 */
//const WORD MOD_CMD_BSS_OUT_END = 0x240;

/* 喆道处理经销商采购入库 */
//const WORD MOD_CMD_BSS_PIN_BEGIN = 0x240;
//const WORD MOD_CMD_BSS_PIN_CMT = MOD_CMD_BSS_PIN_BEGIN + 1;         /* 采购入库 */
//const WORD MOD_CMD_BSS_CANCEL_IN_CMT = MOD_CMD_BSS_PIN_BEGIN + 2;   /* 退货调拨入库 */
//const WORD MOD_CMD_BSS_PIN_END = 0x245;

/* 实时查询二维码信息 */
//const WORD MOD_CMD_BSS_QRGET_BEGIN = 0x24a;
//const WORD MOD_CMD_BSS_QRGET_GDSI = MOD_CMD_BSS_QRGET_BEGIN + 1;           /* 查询二维码对应的商品信息，生产任务信息，可以返回商品id */
//const WORD MOD_CMD_BSS_QRGET_RELATED_S = MOD_CMD_BSS_QRGET_BEGIN + 2;      /* 查询其关联的二维码精简信息(二维码商品id和对应数量) */
//const WORD MOD_CMD_BSS_QRGET_RELATED_D = MOD_CMD_BSS_QRGET_BEGIN + 3;      /* 查询其关联的二维码详细信息(二维码、商品id) */
//const WORD MOD_CMD_BSS_QRGET_RELATED_SFB_D = MOD_CMD_BSS_QRGET_BEGIN + 4;  /* 查询其关联的二维码详细信息(二维码、商品id),从小单位到大单位（由单品码找箱码） */
//const WORD MOD_CMD_BSS_QRGET_3RD = MOD_CMD_BSS_QRGET_BEGIN + 5;            /* 查询第三方码(非喆道QR码)信息 */
//const WORD MOD_CMD_BSS_QRGET_END = 0x250;

/* 盘点业务 */
//const WORD MOD_CMD_BSS_DAMAGED_BEGIN = 0x25a;
//const WORD MOD_CMD_BSS_DAMAGED = MOD_CMD_BSS_DAMAGED_BEGIN + 1;    /* 盘点业务 */
//const WORD MOD_CMD_BSS_DAMAGED_END = 0x260;

/****************************************************************
*文件范围 : 信息帧类型
****************************************************************/
/* 信息帧类型 为了防止和别的应用混淆，信息帧从0x8000开始编排*/
//const WORD INFO_TYPE_BSS_INSTORE = 0x8205;        /* 生产企业商品入库服务 */
//const WORD INFO_TYPE_BSS_BSDB = 0x8206;               /* 企业基本信息查询 */
//const WORD INFO_TYPE_BSS_OUTSTORE = 0x8207;     /* 企业商品出库服务 */
//const WORD INFO_TYPE_BSS_PINSTORE = 0x8208;      /* 企业采购入库服务 */
//const WORD INFO_TYPE_BSS_QRGET = 0x820a;            /* 命令查询QR信息 */
//const WORD INFO_TYPE_BSS_DAMAGED = 0x820d;        /* 盘点业务 */

/****************************************************************
*文件范围 : 与服务器通讯参数描述对象
****************************************************************/
//struct TLV_CMD_PKG_INFO
//{
//    const char* cmd_stru_script;  /* 发送给服务器的参数描述对象 */
//    const char* rsp_stru_script;  /* 响应的参数描述对象，特别注意:前面有一个WORD的错误码 */
//    WORD        cmd;                /* 各个模块的命令字定义 */
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsg[] = {
//    {"s16-s16-f16-s16-s16", "w-d-d-d-d-d-w-w-d", MOD_CMD_BSG_LOGIN}, //最后:s16-s16表示:版本,spc可访问IP
//    {"s16-s16-f16",      "w-d-d-d-d-d-w-w-d",    MOD_CMD_BSG_LOGOUT},
//    {"s16-s16-f16",      "w-d-d-d-d-d-w-w-d",    MOD_CMD_BSG_LOGIN_ERR},
//
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss[] = {
//    {"d-d-d-d-w-s16",  "w",                MOD_CMD_BSS_SYN_LOGDT}, //最后:s16表示:版本
//    {"d-d",            "w-d-d-d-w-v",      MOD_CMD_BSS_LOGIN},   /* 2017-08-23  16:48:14 增加数据库更新序号的同步 */
//    {"d",              "w-d-d",            MOD_CMD_BSS_ACTIVE},   /* 2017-08-23  16:48:42 增加数据库更新序号的同步 */
//    {"d-d",            "w-d-d-d-w-v",      MOD_CMD_BSS_LOGOUT},   /* 2017-08-23  16:48:14 退出登录 */
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_in[] = {
//    {"d",      "w",    MOD_CMD_BSS_IN_CMT_F},
//    {"d",      "w",    MOD_CMD_BSS_IN_CMT_AF},
//    {"q-q-d-d-d-s20-s20-s20-s10-s20",  "w",    MOD_CMD_BSS_IN_CMT_TASK}, //任务ID,生产订单号,商品ID(灭菌前的),商品ID(灭菌后的), 行项目号, 生产批号,生产日期,有效期至,班次,操作员
//    {"q-d-w-w-d-d-s64",                "w",    MOD_CMD_BSS_IN_CMT_S}, //任务ID,商品ID,是否y码,二维码类型,随机码,商品ID(灭菌后的),二维码
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_bsdb[] = {
//    {"d",			 "w-d",      MOD_CMD_BSS_DB_QUERY_BSS},
//    {"d-s32-s32",    "w-d",      MOD_CMD_BSS_DB_QUERY_QR},
//    {"d-s2",         "w-d",      MOD_CMD_BSS_DB_PLINE_QR}, //参数1:请求方(数量,类型i/k/p); 参数2:给出反馈
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_out[] = {
//    {"d",      "w",    MOD_CMD_BSS_OUT_CMT},
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_pin[] = {
//    {"d",      "w",    MOD_CMD_BSS_PIN_CMT},
//    {"d",      "w",    MOD_CMD_BSS_CANCEL_IN_CMT},
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_qrget[] = {
//    {"s64",  "w-s64-d-d-s32-s128-s64-s64-s32",    MOD_CMD_BSS_QRGET_GDSI},/* 响应信息:自己的二维码,商品id,批次id,任务id,任务简称,生产日期,序列号,赋码时间 */
//    {"s64",  "w-s64-d-w-v",           MOD_CMD_BSS_QRGET_RELATED_S},    /* 响应信息:自己的二维码,商品信息,分组数,[商品id,数量] */
//    {"s64",  "w-s64-s20-s20-s32-d-w-w-v", MOD_CMD_BSS_QRGET_RELATED_D},    /* 响应信息:自己的二维码,效期,box_number,灭菌批号,商品信息,体积,分组数,[子商品id,二维码] */
//    {"s64",  "w-s64-d-w-v",           MOD_CMD_BSS_QRGET_RELATED_SFB_D},   /* 小单位查找大单位 响应信息:自己的二维码,商品信息,分组数,[商品id,二维码] */
//    {"s64",  "w-s64-d-s16-d",         MOD_CMD_BSS_QRGET_3RD},
//};
//
//static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_damaged[] = {
//    { "d",      "w-d",    MOD_CMD_BSS_DAMAGED},
//};

#endif
