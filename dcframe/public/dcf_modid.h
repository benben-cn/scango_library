#ifndef Dcf_modid_h
#define Dcf_modid_h

/*
文件说明: 该文件包含DCF框架自身所有模块的ID
设计思路: 为了快速查找模块id对应邮箱信息，将邮箱进行分区编排
                1.无任务模块区:                 0x8000~0xBfff   (共享回调)
                2.混合模块区:                   0xc000~0xffff(先不支持,自己有任务)
                3.自有任务模块区:               0x0000~0x7fff
作         者:zjb
时         间:2017-5-6 15:56
*/

//自有任务模块区域
#define MOD_OWN_AREA_BEGIN 0x0000
#define MOD_OWN_AREA_END     0x7FFF
// 共享任务模块区域
#define MOD_PUB_AREA_BEGIN 0x8000
#define MOD_PUB_AREA_END     0xBFFF
// 混合任务模块区域
#define MOD_MIX_AREA_BEGIN 0xC000
#define MOD_MIX_AREA_END     0xFFFF

/*自有任务模块定义区域*/
// DCF框架核心模块的ID
const WORD DCF_MOD_ID_TASKMNG = 1;   // 任务管理模块
const WORD DCF_MOD_ID_MEMC = 2;        // 邮箱通信管理模块
const WORD DCF_MOD_ID_PMMC = 3;        // PMC模块(公共邮箱调用模块)
const WORD DCF_MOD_ID_RPCA = 4;         /* 服务进程的客户端  */
const WORD DCF_MOD_ID_RPCSV = 5;      /* 服务进程的服务端 */

const WORD DCF_MOD_ID_SQL = 10;      /* 数据库连接池 */

/* 控制台相关模块 */
const WORD BC_MOD_ID_HARDCODING = 20;  /* 赋码模块 */
const WORD BC_MOD_ID_RPC = 21;                   /* 企业设备控制台 */
const WORD BC_MOD_ID_CODEMNG = 22;        /* 控制台中的二维码管理模块 */
const WORD BC_MOD_ID_DBMS = 23;                /* 控制台内提供企业基本信息查询 */
const WORD BC_MOD_ID_CPYQR = 24;              /* 外包装二维码复制打码 */
const WORD BC_MOD_ID_BOXCS = 25;             /* 箱关联单品 */
const WORD BC_MOD_ID_BATCX = 26;             /* 垛关联箱/单品 */
const WORD BC_MOD_ID_PRINT = 27;              /* 控制台连打印机 */

/* bs界面相关模块 ，相当于Bc*/
const WORD BC_MOD_ID_INTERFACE = 28;              /*bs界面 */


/* 企业服务器相关模块 */
const WORD BS_MOD_ID_RPC = 30;                   /* 企业服务通信模块 */
const WORD BS_MOD_ID_GENCODE = 31;         /* 产生二维码的模块 */
const WORD BS_MOD_ID_CODE = 32;                /* 提供二维码各种服务的模块 */  
const WORD BS_MOD_ID_DBMS = 33;                /* 提供企业基本信息查询 */
const WORD BS_MOD_ID_INSTORE = 34;          /* 提供入库服务 */
const WORD BS_MOD_ID_STOCK = 36;              /* 提供库存服务 */

const WORD BS_MOD_ID_INTERFACE = 37;          /* bs界面通信 */


/* 企业服务器和服务进程间相关模块 */
const WORD BSC_MOD_ID_RPC = 40;                 /* 中海塆系统服务通信客户端 */
const WORD BSS_MOD_ID_RPC = 41;                 /* 中海塆系统服务通信服务端 */
const WORD BSG_MOD_ID_RPC = 42;                 /* 中海塆系统服务通信网关端 */

/* 中海湾服务进程内相关模块 */
const WORD BSS_MOD_ID_INSTORE    = 50;       /* 中海湾服务进程的入库处理模块 */
const WORD BSS_MOD_ID_OUTSTORE = 51;       /* 中海湾服务进程的出库处理模块 */
const WORD BSS_MOD_ID_BSDB = 52;                /* 中海湾服务进程的基本配置处理模块 */
const WORD BSS_MOD_ID_STOCK = 53;              /* 提供库存服务 */
const WORD BSS_MOD_ID_PINSTORE = 54;       /* 经销商采购入库 */
const WORD BSS_MOD_ID_RELATED = 55;         /* 重关联 */
const WORD BSS_MOD_ID_QRGET = 56;             /* 命令查询QR信息 */
const WORD BSS_MOD_ID_PCRING = 57;           /* 供货商和客户圈维护 */
const WORD BSS_MOD_ID_PSDB = 58;                /* 待收发货表同步 */
const WORD BSS_MOD_ID_DAMAGED = 59;            /* 报损模块 */
const WORD BSS_MOD_ID_QRCREATE = 60;           /* 企业二维码生成模块,包括白码 */


const WORD BSS_MOD_ID_FPLANT = 65;            /* 农产品种植模块 */
const WORD BSS_MOD_ID_FCIRCULATION = 66;      /* 农产品流通模块 */

/*共享任务模块定义区域*/
const WORD DCF_MOD_ID_CRM = MOD_PUB_AREA_BEGIN + 1;  /* 命令打包模块 */
/*混合任务模块定义区域*/

inline bool dcf_module_have_own_task(WORD modid)
{
    if ((modid >= MOD_PUB_AREA_BEGIN) && (modid <= MOD_PUB_AREA_END))
    {
        return false;
    }

    return true;
}

inline bool dcf_module_is_mix_type(WORD modid)
{
    if (modid >= MOD_MIX_AREA_BEGIN)
    {
        return true;
    }
    return false;
}

inline bool dcf_module_can_reg_pmmc(WORD modid)
{
    if ((modid >= MOD_PUB_AREA_BEGIN) && (modid <= MOD_PUB_AREA_END))
    {
        // 暂时只允许共享模块ID注册
        return true;
    }
    return false;
}
#endif

