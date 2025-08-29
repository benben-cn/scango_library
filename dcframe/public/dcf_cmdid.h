#ifndef Dcf_cmdid_h
#define Dcf_cmdid_h
/****************************************************************
*文件范围 : 各个模块的命令字定义，便于收发方对照解析
                   命令字是否需要统一编排?基于技术角度，无需编排，但基于维护的角度，一个系统有唯一的命令字，会方便很多，通过命令字就可以搜索消息发送和处理的地方
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  14:54:9
****************************************************************/
#include "dcf_def.h"

/* 新增命令的区段之后，一定要在dcf_pkghelper.cpp中的区段中增加相关信息 */

// 按照下面区段的样式进行统一分配
const WORD MOD_CMD_RPCA_BEGIN = 0x100;  //
const WORD MOD_CMD_RPCA_LOGIN = MOD_CMD_RPCA_BEGIN + 1;     /* 登录命令 */
const WORD MOD_CMD_RPCA_ACTIVE = MOD_CMD_RPCA_BEGIN + 2;   /* 保活命令 */
const WORD MOD_CMD_RPCA_END = 0x120;

/* 企业服务器支持的相关命令 */
const WORD MOD_CMD_BSRPC_BEGIN = 0x120;     //
const WORD MOD_CMD_BSRPC_LOGIN = MOD_CMD_BSRPC_BEGIN + 1;     /* 登录命令 */
const WORD MOD_CMD_BSRPC_ACTIVE = MOD_CMD_BSRPC_BEGIN + 2;   /* 保活命令 */
const WORD MOD_CMD_BSRPC_ERR_CIRTIFY = MOD_CMD_BSRPC_BEGIN + 3;  /* 2017-09-05  16:33:3 增加一个错误的认证号，因为是内网，所以可以增加一个错误响应，避免持续等待*/
const WORD MOD_CMD_BSRPC_END = 0x130;

/* 企业服务器产生二维码命令 带F为流水线命令 */
const WORD MOD_CMD_BS_CDG_BEGIN = 0x130;
const WORD MOD_CMD_BS_CDG_GEN_F = MOD_CMD_BS_CDG_BEGIN + 1;   /* 生成二维码 */
const WORD MOD_CMD_BS_CDG_END = 0x135;

/* 企业服务器二维码管理 带F为流水线命令 */
const WORD MOD_CMD_BS_CD_BEGIN = 0x135;
const WORD MOD_CMD_BS_CD_COMMIT_F = MOD_CMD_BS_CD_BEGIN + 1;     /* 控制台提交二维码 */
const WORD MOD_CMD_BS_CD_COMMIT_BcS_F = MOD_CMD_BS_CD_BEGIN + 2;   /* 控制台提交箱码和单品码的关联 */
const WORD MOD_CMD_BS_CD_COMMIT_BcX_F = MOD_CMD_BS_CD_BEGIN + 3;   /* 控制台提交垛码和箱码/单品码的关联 */
const WORD MOD_CMD_BS_CD_END = 0x13A;

/* 企业服务器二维码入库 */
const WORD MOD_CMD_BS_IN_BEGIN = 0x13A;
const WORD MOD_CMD_BS_IN_COMMIT = MOD_CMD_BS_IN_BEGIN + 1;         /* 入库提交命令 */
const WORD MOD_CMD_BS_IN_QUERYNEW = MOD_CMD_BS_IN_BEGIN + 2;   /* 查询new表命令，放在这里的目的是为了降低对qrcode进程的影响 */
const WORD MOD_CMD_BS_IN_END = 0x140;

/* 企业服务器二维码库存 */
const WORD MOD_CMD_BS_STOCK_BEGIN = 0x140;
const WORD MOD_CMD_BS_STOCK_QUERY = MOD_CMD_BS_STOCK_BEGIN + 1;         /* 查询库存命令 */
const WORD MOD_CMD_BS_STOCK_OUT = MOD_CMD_BS_STOCK_BEGIN + 2;         /* 出库 */
const WORD MOD_CMD_BS_COMB_SPLIT = MOD_CMD_BS_STOCK_BEGIN + 3;        /* 重新组合/分拆 */
const WORD MOD_CMD_BS_DAMAGED = MOD_CMD_BS_STOCK_BEGIN + 4;        /* 报损 */
const WORD MOD_CMD_BS_STOCK_END = 0x145;

/* 企业服务器基本信息管理 */
const WORD MOD_CMD_BS_DB_BEGIN = 0x160;
const WORD MOD_CMD_BS_DB_QUERY_BS = MOD_CMD_BS_DB_BEGIN + 1;     /* 获取基本信息 */
const WORD MOD_CMD_BS_DB_QUERY_PS = MOD_CMD_BS_DB_BEGIN + 2;     /* 获取待收发货信息 */
const WORD MOD_CMD_BS_DB_END = 0x165;

/* bs与bs界面信息同步 */
const WORD MOD_CMD_BS_DB_INTERFACE_BEGIN = 0x170;
const WORD MOD_CMD_BS_DB_INTERFACE_QUERY_BS = MOD_CMD_BS_DB_INTERFACE_BEGIN + 1;     /* bs界面从bs获取基本信息 */
const WORD MOD_CMD_BS_DB_INTERFACE_SET_BS = MOD_CMD_BS_DB_INTERFACE_BEGIN + 2;      /* bs界面提交后，将数据表覆盖原来的数据表 */
const WORD MOD_CMD_BS_DB_INTERFACE_END = 0x175;


/* 网关支持的相关命令 */
const WORD MOD_CMD_BSG_BEGIN = 0x200;   //
const WORD MOD_CMD_BSG_LOGIN = MOD_CMD_BSG_BEGIN + 1;/* 登录网关命令 */
const WORD MOD_CMD_BSG_LOGOUT = MOD_CMD_BSG_BEGIN + 2;/* 登出网关命令 */
const WORD MOD_CMD_BSG_LOGIN_ERR = MOD_CMD_BSG_BEGIN + 3; /* 登录错误命令 */
const WORD MOD_CMD_BSG_END = 0x210;

/* 服务端支持的相关命令 */
const WORD MOD_CMD_BSS_BEGIN = 0x210;
const WORD MOD_CMD_BSS_SYN_LOGDT = MOD_CMD_BSS_BEGIN + 1;   /* 同步客户端登录数据命令 */
const WORD MOD_CMD_BSS_LOGIN = MOD_CMD_BSS_BEGIN + 2;             /* 登录服务进程命令 */
const WORD MOD_CMD_BSS_ACTIVE = MOD_CMD_BSS_BEGIN + 3;             /* 企业设备和服务端保活命令 */
const WORD MOD_CMD_BSS_LOGOUT = MOD_CMD_BSS_BEGIN + 4;             /* 重复登录,须要退出*/
const WORD MOD_CMD_BSS_END = 0x220;

/* 服务端支持的业务相关命令 */
/* 生产企业提交入库 */
const WORD MOD_CMD_BSS_IN_BEGIN = 0x220;
const WORD MOD_CMD_BSS_IN_CMT_F = MOD_CMD_BSS_IN_BEGIN + 1;   /* 生产企业批量提交入库数据 */
const WORD MOD_CMD_BSS_IN_CMT_AF = MOD_CMD_BSS_IN_BEGIN + 2;   /* 生产企业批量自动提交入库数据 */
const WORD MOD_CMD_BSS_IN_CMT_TASK = MOD_CMD_BSS_IN_BEGIN + 3;   /* 任务ID,商品ID,灭菌批号, 生产批号, 生产日期, 有效期至 */
const WORD MOD_CMD_BSS_IN_CMT_S = MOD_CMD_BSS_IN_BEGIN + 4;   /* 二维码类型,商品ID,随机码,二维码, 任务ID */
const WORD MOD_CMD_BSS_IN_END = 0x230;

/* 企业服务器基本信息管理 */
const WORD MOD_CMD_BSS_DB_BEGIN = 0x230;
const WORD MOD_CMD_BSS_DB_QUERY_BSS = MOD_CMD_BSS_DB_BEGIN + 1; /* 获取基本信息 */
const WORD MOD_CMD_BSS_DB_QUERY_QR = MOD_CMD_BSS_DB_BEGIN + 2; /* 获取盘点一物一码 */
const WORD MOD_CMD_BSS_DB_PLINE_QR = MOD_CMD_BSS_DB_BEGIN + 3; /* 获取白码 */
const WORD MOD_CMD_BSS_DB_END = 0x235;

/* 喆道处理企业出库操作 */
const WORD MOD_CMD_BSS_OUT_BEGIN = 0x235;
const WORD MOD_CMD_BSS_OUT_CMT = MOD_CMD_BSS_OUT_BEGIN + 1;         /* 出库 */
const WORD MOD_CMD_BSS_OUT_END = 0x240;

/* 喆道处理经销商采购入库 */
const WORD MOD_CMD_BSS_PIN_BEGIN = 0x240;
const WORD MOD_CMD_BSS_PIN_CMT = MOD_CMD_BSS_PIN_BEGIN + 1;         /* 采购入库 */
const WORD MOD_CMD_BSS_CANCEL_IN_CMT = MOD_CMD_BSS_PIN_BEGIN + 2;   /* 退货调拨入库 */
const WORD MOD_CMD_BSS_PIN_END = 0x245;

/* 二维码重关联 */
const WORD MOD_CMD_BSS_BRS_BEGIN = 0x245;
const WORD MOD_CMD_BSS_BRS_MODIFY_F = MOD_CMD_BSS_BRS_BEGIN + 1;  /* 离线批量修改 */
const WORD MOD_CMD_BSS_BRS_MODIFY = MOD_CMD_BSS_BRS_BEGIN + 2;      /* 单条命令修改 */
const WORD MOD_CMD_BSS_BRS_DELETE = MOD_CMD_BSS_BRS_BEGIN + 3;      /* 2018-01-16  11:7:22 删除二维码，同时删除与之关联的信息 */
const WORD MOD_CMD_BSS_BRS_END = 0x24a;

/* 实时查询二维码信息 */
const WORD MOD_CMD_BSS_QRGET_BEGIN = 0x24a;
const WORD MOD_CMD_BSS_QRGET_GDSI = MOD_CMD_BSS_QRGET_BEGIN + 1;           /* 查询二维码对应的商品信息，生产任务信息，可以返回商品id */
const WORD MOD_CMD_BSS_QRGET_RELATED_S = MOD_CMD_BSS_QRGET_BEGIN + 2;      /* 查询其关联的二维码精简信息(二维码商品id和对应数量) */
const WORD MOD_CMD_BSS_QRGET_RELATED_D = MOD_CMD_BSS_QRGET_BEGIN + 3;      /* 查询其关联的二维码详细信息(二维码、商品id) */
const WORD MOD_CMD_BSS_QRGET_RELATED_SFB_D = MOD_CMD_BSS_QRGET_BEGIN + 4;   /* 查询其关联的二维码详细信息(二维码、商品id),从小单位到大单位（由单品码找箱码） */
const WORD MOD_CMD_BSS_QRGET_3RD = MOD_CMD_BSS_QRGET_BEGIN + 5;            /* 查询第三方码(非喆道QR码)信息 */
const WORD MOD_CMD_BSS_QRGET_END = 0x250;

/* 供货商和客户圈信息维护 */
const WORD MOD_CMD_BSS_PCRG_BEGING = 0x250;
const WORD MOD_CMD_BSS_PCRG_CREATE_P = MOD_CMD_BSS_PCRG_BEGING + 1;    /* 增加供货商 */
const WORD MOD_CMD_BSS_PCRG_CREATE_C = MOD_CMD_BSS_PCRG_BEGING + 2;    /* 增加客户 */
const WORD MOD_CMD_BSS_PCRG_END = 0x255;

/* 待收发货信息同步 */
const WORD MOD_CMD_BSS_PSDB_BEGIN = 0x255;
const WORD MOD_CMD_BSS_PSDB_QUERY = MOD_CMD_BSS_PSDB_BEGIN + 1;    /* 查询待收发货 */
const WORD MOD_CMD_BSS_PSDB_END = 0x25a;

/* 报损信息同步 */
const WORD MOD_CMD_BSS_DAMAGED_BEGIN = 0x25a;
const WORD MOD_CMD_BSS_DAMAGED = MOD_CMD_BSS_DAMAGED_BEGIN + 1;    /* 报损业务 */
const WORD MOD_CMD_BSS_DAMAGED_END = 0x260;


/* 农产品种植 */
const WORD MOD_CMD_BSS_FPLANT_BEGIN = 0x260;
const WORD MOD_CMD_BSS_PLANTAREA = MOD_CMD_BSS_FPLANT_BEGIN + 1;    /* 农产品场地  增加与修改*/
const WORD MOD_CMD_BSS_PLANTAREA_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 2;    /* 农产品场地  删除*/
const WORD MOD_CMD_BSS_AREA_PRE_ALLOC = MOD_CMD_BSS_FPLANT_BEGIN + 3;  /* 预分配一个场地码所对应的aid */
// const WORD MOD_CMD_BSS_CROP_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 4;          /* 农产品作物 删除*/
const WORD MOD_CMD_BSS_GROWTH = MOD_CMD_BSS_FPLANT_BEGIN + 5;        /* 农产品种生长 增加与修改*/
const WORD MOD_CMD_BSS_GROWTH_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 6;        /* 农产品种生长删除*/
const WORD MOD_CMD_BSS_GROWTH_FINISH = MOD_CMD_BSS_FPLANT_BEGIN + 7;     /* 农产品正常结束*/

const WORD MOD_CMD_BSS_GROWING_PHOTO = MOD_CMD_BSS_FPLANT_BEGIN + 8;    /* 农产品种生长图片 增加与修改*/
const WORD MOD_CMD_BSS_GROWING_PHOTO_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 9;    /* 农产品种生长图片 删除*/
const WORD MOD_CMD_BSS_PLANT_LOG = MOD_CMD_BSS_FPLANT_BEGIN + 10;        /* 农事活动 增加与修改*/
const WORD MOD_CMD_BSS_PLANT_LOG_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 11;        /* 农事活动 删除*/
const WORD MOD_CMD_BSS_HARVEST = MOD_CMD_BSS_FPLANT_BEGIN + 12;    /* 农产品种收割 增加与修改*/
const WORD MOD_CMD_BSS_HARVEST_DEL = MOD_CMD_BSS_FPLANT_BEGIN + 13;    /* 农产品种收割 删除*/
const WORD MOD_CMD_BSS_QUERY_FARM_DB = MOD_CMD_BSS_FPLANT_BEGIN + 14;    /* 获取农场信息 */
const WORD MOD_CMD_BSS_UP_ASSERTS = MOD_CMD_BSS_FPLANT_BEGIN + 15;     /* 更新资产码 */
const WORD MOD_CMD_BSS_QUREY_ASSETS = MOD_CMD_BSS_FPLANT_BEGIN + 16;
const WORD MOD_CMD_BSS_FPLANT_END = 0x2A0;


/* 农销码关联 */
const WORD MOD_CMD_BSS_FCIRCULATION_BEGIN = 0x2A0;
const WORD MOD_CMD_BSS_RQUERY_BYQR_C = MOD_CMD_BSS_FCIRCULATION_BEGIN + 1;    /* 根据农筐码(C码)查询关联信息 */
const WORD MOD_CMD_BSS_PCI_RELATION = MOD_CMD_BSS_FCIRCULATION_BEGIN + 2;    /* 农产品提交*/
const WORD MOD_CMD_BSS_PCI_QT_QUERY = MOD_CMD_BSS_FCIRCULATION_BEGIN + 3;    /* 质检消息查询*/
const WORD MOD_CMD_BSS_PCI_QT_COMMIT = MOD_CMD_BSS_FCIRCULATION_BEGIN + 4;    /* 质检图片id上传*/
const WORD MOD_CMD_BSS_RQUERY_BYQR_F = MOD_CMD_BSS_FCIRCULATION_BEGIN + 5;     /* 根据农销码(F码)查询关联信息 */
const WORD MOD_CMD_BSS_FCIRCULATION_END = 0x2B0;

/* 二维码信息(赋码信息,物流信息,支付信息) */
const WORD MOD_CMD_BSS_QRINFO_BEGIN = 0x300;
const WORD MOD_CMD_BSS_QRINFO_RELATED_INDEX = MOD_CMD_BSS_QRINFO_BEGIN + 1;  /* 查询全部信息(赋码信息,物流信息,支付信息)  */
const WORD MOD_CMD_BSS_QRINFO_END = 0x350;

#endif
