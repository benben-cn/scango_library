#pragma once
/****************************************************************
*文件范围 : bs系统模块头文件定义
*设计说明 : A.数据库设计
                   1.基本配置:zhw_base.db
                      含有企业信息、企业设备信息、控制台登录信息、网关信息、设备秘钥
                   2.商品配置:bs_gds_base.db
                      含有商品、规格、生产任务信息
                      设备登录和上面数据同步后，bs会将该库发送给客户端
                   3.二维码生产线流水库:bs_gds_qrc_new.db
                      各个bc产生的数据，直接写该库
                   4.移动设备入/出库扫码动作库:bs_gds_opt.db
                      扫码的各种动作和对应的二维码
                      移动终端，直接提交该数据给bs，bs解析(读bs_gds_qrc_new.db表，并置对应标记 1，
                      并以bs_gds_qrc_new.db为模板，将本次提交的记录存入模板中)之后提交到bss，成功
                      之后将bs_gds_qrc_new.db中对应1置2，同时将这些数据提交到bs_gds_stock.db
                      bs_gds_qrc_new.db中对应的
                   5.企业库存表:bs_gds_stock.db
                     可以给移动终端直接同步该库
                   6.企业出库:bs_gds_opt.db
                     扫码的各种动作和对应的二维码
                     移动终端，直接提交该数据给bs，bs解析(读bs_gds_stock.db表，并置对应标记1，
                     并以bs_gds_out.db为模板，将本次提交的记录存入模板中)，提交到bss，成功之后
                     讲bs_gds_stock.db中对应标记1置2
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-08  16:28:28
****************************************************************/
#include "dcf_def.h"

/* 统一的密钥宏,位置不能乱调整，必须和数据库对应起来和zhw_base中的zhw_key.type对应 */
enum
{
    KEY_PUB_COM_BSG = 0,        /* BSG通信公钥 */
    KEY_PUB_COM_BSS,               /* BSS通信公钥 */
    KEY_SELF_COM_BS,                /* BS自己的通信私钥 */
    KEY_PUB_COM_QRCODE,      /* QRCODE公钥 */
    KEY_SELF_QRCODE_BS,         /* 企业设备的QRCODE私钥 */
    KEY_MAX
};
extern const char *dcf_key_get(WORD wType,WORD wVer);

/* 统一的编号宏 */
enum
{
    BS_ID_BS = 0,         /* 企业id */
    BS_ID_BS_SUB,      /* 企业生产分厂id */
    BS_ID_DEV,             /* 本服务器的设备id */
    BS_ID_DEV_SN,     /* 本服务器设备编号 */
    BS_ID_MAX
};
extern void dcf_id_set(WORD wType,DWORD id);
extern DWORD dcf_id_get(WORD wType);

