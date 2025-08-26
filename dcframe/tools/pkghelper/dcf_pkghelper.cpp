
#include "dcf_pkghelper.h"
#include "dcf_modid.h"
#include "dcf_sort.h"
#include "dcf_cmdid.h"
#include "dcf_err.h"
#include "dcf_string.h"
#include "extern_api.h"

/* 信息提供注册 */
struct INFO_PSV
{
    WORD info_type;
    WORD mod_id;
    BYTE    entry_id;
    BYTE    bReg;
    WORD wRes;
};

/* 需要添加的业务节点 */
INFO_PSV g_bs_info_provider_list[] = {
    {INFO_TYPE_BS_GENCODE,    BS_MOD_ID_GENCODE , 1 , 0 ,0},
    {INFO_TYPE_BS_CODE,  BS_MOD_ID_CODE , 1 , 0 , 0},
    {INFO_TYPE_BS_BSDB,  BS_MOD_ID_DBMS, 1 , 0 , 0},
    {INFO_TYPE_BS_INSTORE,  BS_MOD_ID_INSTORE, 1 , 0 , 0},
    {INFO_TYPE_BS_STOCK,  BS_MOD_ID_STOCK, 1 , 0 , 0},
    {INFO_TYPE_BS_INTERFACE, BS_MOD_ID_INTERFACE, 1 , 0 , 0 },
    {INFO_TYPE_BSS_INSTORE,  BSS_MOD_ID_INSTORE, 1 , 0 , 0},
    {INFO_TYPE_BSS_BSDB,  BSS_MOD_ID_BSDB, 1 , 0 , 0},
    {INFO_TYPE_BSS_OUTSTORE,  BSS_MOD_ID_OUTSTORE, 1 , 0 , 0},
    {INFO_TYPE_BSS_PINSTORE,  BSS_MOD_ID_PINSTORE, 1 , 0 , 0},
    {INFO_TYPE_BSS_RELATED , BSS_MOD_ID_RELATED ,1 , 0 , 0},
    {INFO_TYPE_BSS_QRGET , BSS_MOD_ID_QRGET,1 , 0 , 0},
    {INFO_TYPE_BSS_PCRING , BSS_MOD_ID_PCRING,1 , 0 , 0},
    {INFO_TYPE_BSS_PSDB,  BSS_MOD_ID_PSDB, 1 , 0 , 0},
    {INFO_TYPE_BSS_DAMAGED, BSS_MOD_ID_DAMAGED, 1 , 0 , 0 },
    {INFO_TYPE_BSS_FPLANT,  BSS_MOD_ID_FPLANT, 1 , 0 , 0 },
    {INFO_TYPE_BSS_FCIRCULATION,  BSS_MOD_ID_FCIRCULATION, 1 , 0 , 0 },
};
DWORD g_bs_info_provider_nums = sizeof(g_bs_info_provider_list) / sizeof(INFO_PSV);
DWORD dcf_sv_info_provider_register(WORD info_type, WORD mod_id, BYTE entry_id)
{
    for (DWORD i = 0; i < g_bs_info_provider_nums; i++)
    {
        if (g_bs_info_provider_list[i].info_type == info_type)
        {
            if (g_bs_info_provider_list[i].bReg)
            {
                // 只能注册1次
                return DCF_ERR_REPEAT;
            }

            g_bs_info_provider_list[i].mod_id = mod_id;
            g_bs_info_provider_list[i].entry_id = entry_id;
            g_bs_info_provider_list[i].bReg = 1;
            return 0;
        }
    }
    return DCF_ERR_NOT_EXIST;
}

DWORD dcf_sv_info_provider_get(WORD info_type, WORD &mod_id, BYTE &entry_id)
{
    for (DWORD i = 0; i < g_bs_info_provider_nums; i++)
    {
        if (g_bs_info_provider_list[i].info_type == info_type)
        {
            if (!g_bs_info_provider_list[i].bReg)
            {
                // 还没有注册
                return DCF_ERR_SYS_NONE_INIT;
            }

            mod_id = g_bs_info_provider_list[i].mod_id;
            entry_id = g_bs_info_provider_list[i].entry_id;
            return 0;
        }
    }
    return DCF_ERR_NOT_EXIST;
}

/* 2017-05-24  11:14:46 */
/* 在版本中需要将该命令模块对应关系表单独封装成库编译,系统层和应用层在一起填写 */
static TLV_CMD_PKG_INFO  g_cmd_struc_trans_rpca[] = {
    {"D",     "w-d",        MOD_CMD_RPCA_LOGIN,            DCF_MOD_ID_RPCA   },
    {"D",    "w-d",         MOD_CMD_RPCA_ACTIVE,          DCF_MOD_ID_RPCA   },
};

static TLV_CMD_PKG_INFO  g_cmd_struc_trans_bsrpc[] = {
    {"S16-S16",      "w-d-d-d",    MOD_CMD_BSRPC_LOGIN,         BS_MOD_ID_RPC   },     /* 2017-08-23  18:1:31 增加db同步序号 */
    {"d",                  "w-d-d-d",    MOD_CMD_BSRPC_ACTIVE,       BS_MOD_ID_RPC   },     /* 2017-08-23  18:1:31 增加db同步序号 */
    {"d",                  "w",                MOD_CMD_BSRPC_ERR_CIRTIFY,BS_MOD_ID_RPC}    /* 2017-09-05  16:34:18 增加一个错误响应*/
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bscodegen[] = {
    {"w-d-d",      "w",    MOD_CMD_BS_CDG_GEN_F,         BS_MOD_ID_GENCODE},  //产码类型，产码数量，商品ID
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bscode[] = {
    {"w-d-d-s10-s20-s20-s20-s16-s16",      "w",    MOD_CMD_BS_CD_COMMIT_F,BS_MOD_ID_CODE},   // 二维码类型,随机码,商品ID,二维码,序列号,有效期至,打码时间,任务ID,用户名
    {"d-s16-s16",            "w",    MOD_CMD_BS_CD_COMMIT_BcS_F,     BS_MOD_ID_CODE},   // 规格的商品id，箱码，单品码
    {"s16-s16",              "w",    MOD_CMD_BS_CD_COMMIT_BcX_F,     BS_MOD_ID_CODE},   // 箱码，单品码
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsdb[] = {
    {"d",      "w-d",    MOD_CMD_BS_DB_QUERY_BS,      BS_MOD_ID_DBMS},   // 查询基本信息(命令:当前序号  响应:成功,序号)
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsdb_ic[] = {
    { "d",      "w-d",    MOD_CMD_BS_DB_INTERFACE_QUERY_BS,      BS_MOD_ID_INTERFACE },   // 查询bs中的基本信息(命令:当前序号  响应:成功,序号)
};


static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsin[] = {
     {"d",                 "w",    MOD_CMD_BS_IN_COMMIT,           BS_MOD_ID_INSTORE},   // 命令参数为一个时间戳，表示一个序号，响应码为成功失败
     {"s32-s32",      "w",    MOD_CMD_BS_IN_QUERYNEW,      BS_MOD_ID_INSTORE},   // 参数为一个时间范围
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsstock[] = {
     {"s32-s32",      "w",    MOD_CMD_BS_STOCK_QUERY,      BS_MOD_ID_STOCK},   // 命令参数为一个时间戳，表示一个序号，响应码为成功失败
     {"d",                 "w",    MOD_CMD_BS_STOCK_OUT,      BS_MOD_ID_STOCK},       // 命令是bc发过来的大包，响应是CRM
     {"d",                 "w",    MOD_CMD_BS_COMB_SPLIT,    BS_MOD_ID_STOCK},       // 命令是bc发过来的大包，响应是CRM
     { "d",                "w",    MOD_CMD_BS_DAMAGED,    BS_MOD_ID_STOCK },       //  命令是bc发过来的大包，响应是CRM
};


static TLV_CMD_PKG_INFO g_cmd_struc_trans_bsg[] = {
    {"s16-s16-f16-s16-s16", "w-d-d-d-d-d-w-w-d", MOD_CMD_BSG_LOGIN,     BSG_MOD_ID_RPC}, //最后:s16-s16表示:版本,spc可访问IP
    {"s16-s16-f16",      "w-d-d-d-d-d-w-w-d",    MOD_CMD_BSG_LOGOUT,        BSG_MOD_ID_RPC},
    {"s16-s16-f16",      "w-d-d-d-d-d-w-w-d",    MOD_CMD_BSG_LOGIN_ERR,     BSG_MOD_ID_RPC},

};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss[] = {
    {"d-d-d-d-w-s16",  "w",                MOD_CMD_BSS_SYN_LOGDT,           BSS_MOD_ID_RPC}, //最后:s16表示:版本
    {"d-d",            "w-d-d-d-w-v",      MOD_CMD_BSS_LOGIN,               BSS_MOD_ID_RPC},   /* 2017-08-23  16:48:14 增加数据库更新序号的同步 */
	{"d",              "w-d-d",            MOD_CMD_BSS_ACTIVE,              BSS_MOD_ID_RPC},   /* 2017-08-23  16:48:42 增加数据库更新序号的同步 */
	{"d-d",            "w-d-d-d-w-v",      MOD_CMD_BSS_LOGOUT,              BSS_MOD_ID_RPC},   /* 2017-08-23  16:48:14 退出登录 */
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_in[] = {
    {"d",      "w",    MOD_CMD_BSS_IN_CMT_F,         BSS_MOD_ID_INSTORE},
    {"d",      "w",    MOD_CMD_BSS_IN_CMT_AF,        BSS_MOD_ID_INSTORE},
	{"q-q-d-d-d-s20-s20-s20-s10-s20",  "w",    MOD_CMD_BSS_IN_CMT_TASK,   BSS_MOD_ID_INSTORE}, //任务ID,生产订单号,商品ID(灭菌前的),商品ID(灭菌后的), 行项目号, 生产批号,生产日期,有效期至,班次,操作员
	{"q-d-w-w-d-d-s64",                "w",    MOD_CMD_BSS_IN_CMT_S,      BSS_MOD_ID_INSTORE}, //任务ID,商品ID,是否y码,二维码类型,随机码,商品ID(灭菌后的),二维码
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_bsdb[] = {
    {"d",			 "w-d",      MOD_CMD_BSS_DB_QUERY_BSS,  BSS_MOD_ID_BSDB},
	{"d-s32-s32",    "w-d",      MOD_CMD_BSS_DB_QUERY_QR,   BSS_MOD_ID_BSDB},
	{"d-s2",         "w-d",      MOD_CMD_BSS_DB_PLINE_QR,   BSS_MOD_ID_BSDB}, //参数1:请求方(数量,类型i/k/p); 参数2:给出反馈
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_out[] = {
    {"d",      "w",    MOD_CMD_BSS_OUT_CMT,         BSS_MOD_ID_OUTSTORE},
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_pin[] = {
    {"d",      "w",    MOD_CMD_BSS_PIN_CMT,         BSS_MOD_ID_PINSTORE},
    {"d",      "w",    MOD_CMD_BSS_CANCEL_IN_CMT,   BSS_MOD_ID_PINSTORE},    
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_related[] = {
    {"d",      "w",    MOD_CMD_BSS_BRS_MODIFY_F,     BSS_MOD_ID_RELATED},
    {"b-v",  "w",    MOD_CMD_BSS_BRS_MODIFY,         BSS_MOD_ID_RELATED},    /* 变长命令(不对参数区进行字节序转换)，不适合传统的方式 */
    {"s64",  "w",     MOD_CMD_BSS_BRS_DELETE,         BSS_MOD_ID_RELATED},    /* 2018-01-16  11:10:50 删除对应的二维码 */
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_qrget[] = {
    {"s64",  "w-s64-d-d-s32-s128-s64-s64-s32",    MOD_CMD_BSS_QRGET_GDSI,         BSS_MOD_ID_QRGET},/* 响应信息:自己的二维码,商品id,批次id,任务id,任务简称,生产日期,序列号,赋码时间 */
    {"s64",  "w-s64-d-w-v",           MOD_CMD_BSS_QRGET_RELATED_S,     BSS_MOD_ID_QRGET},    /* 响应信息:自己的二维码,商品信息,分组数,[商品id,数量] */
    {"s64",  "w-s64-s20-s20-s32-d-w-w-v", MOD_CMD_BSS_QRGET_RELATED_D,     BSS_MOD_ID_QRGET},    /* 响应信息:自己的二维码,效期,box_number,灭菌批号,商品信息,体积,分组数,[子商品id,二维码] */
    {"s64",  "w-s64-d-w-v",           MOD_CMD_BSS_QRGET_RELATED_SFB_D, BSS_MOD_ID_QRGET },   /* 小单位查找大单位 响应信息:自己的二维码,商品信息,分组数,[商品id,二维码] */
    {"s64",  "w-s64-d-s16-d",         MOD_CMD_BSS_QRGET_3RD,           BSS_MOD_ID_QRGET },
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_pcring[] = {
    {"d",      "w",    MOD_CMD_BSS_PCRG_CREATE_P,     BSS_MOD_ID_PCRING},
    {"d",      "w",    MOD_CMD_BSS_PCRG_CREATE_C,         BSS_MOD_ID_PCRING},
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_psdb[] = {
    {"d",      "w-d",    MOD_CMD_BSS_PSDB_QUERY,     BSS_MOD_ID_BSDB},
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_damaged[] = {
    { "d",      "w-d",    MOD_CMD_BSS_DAMAGED,     BSS_MOD_ID_DAMAGED },
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_fplant[] = {
    { "d",      "w",    MOD_CMD_BSS_PLANTAREA,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_PLANTAREA_DEL,     BSS_MOD_ID_FPLANT },
    { "s64",    "w-d",    MOD_CMD_BSS_AREA_PRE_ALLOC,     BSS_MOD_ID_FPLANT },
    // { "d",      "w-d",    MOD_CMD_BSS_CROP_DEL,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_GROWTH,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_GROWTH_DEL,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_GROWTH_FINISH,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_GROWING_PHOTO,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_GROWING_PHOTO_DEL,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_PLANT_LOG,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_PLANT_LOG_DEL,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_HARVEST,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_HARVEST_DEL,     BSS_MOD_ID_FPLANT },
    { "d",      "w-d",  MOD_CMD_BSS_QUERY_FARM_DB,     BSS_MOD_ID_FPLANT },
    { "d",      "w",    MOD_CMD_BSS_UP_ASSERTS,     BSS_MOD_ID_FPLANT },
    { "w-d-d-d-s64","w-w-w-v",MOD_CMD_BSS_QUREY_ASSETS,   BSS_MOD_ID_FPLANT },
};

static TLV_CMD_PKG_INFO g_cmd_struc_trans_bss_fcirculation[] = {
    { "d-d-s64",      "w-s64-s16-d-d",    MOD_CMD_BSS_RQUERY_BYQR_C,     BSS_MOD_ID_FCIRCULATION },
    { "d",      "w-d",    MOD_CMD_BSS_PCI_RELATION, BSS_MOD_ID_FCIRCULATION },
    { "s64",    "w-s64-d-d-s16",    MOD_CMD_BSS_PCI_QT_QUERY,     BSS_MOD_ID_FCIRCULATION },
    { "d-s64",      "w-d",    MOD_CMD_BSS_PCI_QT_COMMIT, BSS_MOD_ID_FCIRCULATION },
    { "s64",      "w-d-d",    MOD_CMD_BSS_RQUERY_BYQR_F,     BSS_MOD_ID_FCIRCULATION },
};

// 大家必须按照顺序放
static TLV_MOD_CMD_REG  g_module_cmd_reg[] = {
    {DCF_MOD_ID_RPCA,         sizeof(g_cmd_struc_trans_rpca) / sizeof(TLV_CMD_PKG_INFO),              g_cmd_struc_trans_rpca},
    {BS_MOD_ID_RPC,              sizeof(g_cmd_struc_trans_bsrpc) / sizeof(TLV_CMD_PKG_INFO),            g_cmd_struc_trans_bsrpc},
    {BS_MOD_ID_GENCODE,    sizeof(g_cmd_struc_trans_bscodegen) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bscodegen},
    {BS_MOD_ID_CODE,    sizeof(g_cmd_struc_trans_bscode) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bscode},
    {BS_MOD_ID_DBMS,    sizeof(g_cmd_struc_trans_bsdb) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bsdb},
    {BS_MOD_ID_INSTORE,    sizeof(g_cmd_struc_trans_bsin) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bsin},
    {BS_MOD_ID_STOCK,    sizeof(g_cmd_struc_trans_bsstock) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bsstock},
    {BS_MOD_ID_INTERFACE,    sizeof(g_cmd_struc_trans_bsdb_ic) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bsdb_ic },
    {BSS_MOD_ID_RPC,    sizeof(g_cmd_struc_trans_bss) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss},
    {BSG_MOD_ID_RPC,    sizeof(g_cmd_struc_trans_bsg) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bsg},
    {BSS_MOD_ID_INSTORE,    sizeof(g_cmd_struc_trans_bss_in) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_in},
    {BSS_MOD_ID_OUTSTORE,    sizeof(g_cmd_struc_trans_bss_out) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_out},
    {BSS_MOD_ID_BSDB,    sizeof(g_cmd_struc_trans_bss_bsdb) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_bsdb},
    {BSS_MOD_ID_PINSTORE,    sizeof(g_cmd_struc_trans_bss_pin) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_pin},
    {BSS_MOD_ID_RELATED,    sizeof(g_cmd_struc_trans_bss_related) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_related},
    {BSS_MOD_ID_QRGET,    sizeof(g_cmd_struc_trans_bss_qrget) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_qrget},
    {BSS_MOD_ID_PCRING,    sizeof(g_cmd_struc_trans_bss_pcring) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_pcring},
    {BSS_MOD_ID_PSDB,    sizeof(g_cmd_struc_trans_bss_psdb) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_psdb},
    {BSS_MOD_ID_DAMAGED,    sizeof(g_cmd_struc_trans_bss_damaged) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_damaged },
    {BSS_MOD_ID_FPLANT,    sizeof(g_cmd_struc_trans_bss_fplant) / sizeof(TLV_CMD_PKG_INFO),   g_cmd_struc_trans_bss_fplant },
    {BSS_MOD_ID_FCIRCULATION,    sizeof(g_cmd_struc_trans_bss_fcirculation) / sizeof(TLV_CMD_PKG_INFO),  g_cmd_struc_trans_bss_fcirculation },

};

const DWORD MOD_TLV_REG_SIZE = sizeof(g_module_cmd_reg) / sizeof(TLV_MOD_CMD_REG);

/****************************************************************
CRM中取命令包时，外部无法传递命令注册所属模块，必须弄一组命令和模块的关联关系
****************************************************************/

struct CMD_NODE_AREA
{
    WORD wCmdBegin;
    WORD wCmdEnd;
    WORD wModid;
    WORD wRes;
};

/* 添加命令字段和模块的对应关系 */
CMD_NODE_AREA g_CmdAreaList[] = {
    {MOD_CMD_RPCA_BEGIN  ,  MOD_CMD_RPCA_END , DCF_MOD_ID_RPCA,0},
    {MOD_CMD_BSRPC_BEGIN  ,  MOD_CMD_BSRPC_END , BS_MOD_ID_RPC,0},
    {MOD_CMD_BS_CDG_BEGIN  ,  MOD_CMD_BS_CDG_END , BS_MOD_ID_GENCODE,0},
    {MOD_CMD_BS_CD_BEGIN  ,  MOD_CMD_BS_CD_END , BS_MOD_ID_CODE,0},
    {MOD_CMD_BS_IN_BEGIN  ,  MOD_CMD_BS_IN_END , BS_MOD_ID_INSTORE,0},
    {MOD_CMD_BS_STOCK_BEGIN  ,  MOD_CMD_BS_STOCK_END , BS_MOD_ID_STOCK,0},
    {MOD_CMD_BS_DB_BEGIN  ,  MOD_CMD_BS_DB_END , BS_MOD_ID_DBMS,0},
    {MOD_CMD_BS_DB_INTERFACE_BEGIN, MOD_CMD_BS_DB_INTERFACE_END , BS_MOD_ID_INTERFACE,0 },
    {MOD_CMD_BSG_BEGIN  ,  MOD_CMD_BSG_END , BSG_MOD_ID_RPC,0},
    {MOD_CMD_BSS_BEGIN  ,  MOD_CMD_BSS_END , BSS_MOD_ID_RPC,0},
    {MOD_CMD_BSS_IN_BEGIN,  MOD_CMD_BSS_IN_END, BSS_MOD_ID_INSTORE,0},
    {MOD_CMD_BSS_DB_BEGIN,  MOD_CMD_BSS_DB_END, BSS_MOD_ID_BSDB,0},
    {MOD_CMD_BSS_OUT_BEGIN,  MOD_CMD_BSS_OUT_END, BSS_MOD_ID_OUTSTORE,0},
    {MOD_CMD_BSS_PIN_BEGIN,  MOD_CMD_BSS_PIN_END, BSS_MOD_ID_PINSTORE,0},
    {MOD_CMD_BSS_BRS_BEGIN,  MOD_CMD_BSS_BRS_END, BSS_MOD_ID_RELATED,0},
    {MOD_CMD_BSS_QRGET_BEGIN,  MOD_CMD_BSS_QRGET_END, BSS_MOD_ID_QRGET,0},
    {MOD_CMD_BSS_PCRG_BEGING,  MOD_CMD_BSS_PCRG_END, BSS_MOD_ID_PCRING,0},
    {MOD_CMD_BSS_PSDB_BEGIN,  MOD_CMD_BSS_PSDB_END, BSS_MOD_ID_PSDB,0},
    {MOD_CMD_BSS_DAMAGED_BEGIN,  MOD_CMD_BSS_DAMAGED_END, BSS_MOD_ID_DAMAGED,0 },
    {MOD_CMD_BSS_FPLANT_BEGIN,  MOD_CMD_BSS_FPLANT_END, BSS_MOD_ID_FPLANT,0 },
    {MOD_CMD_BSS_FCIRCULATION_BEGIN, MOD_CMD_BSS_FCIRCULATION_END, BSS_MOD_ID_FCIRCULATION,0 },
};

const DWORD CMD_AREA_REG_SIZE = sizeof(g_CmdAreaList) / sizeof(CMD_NODE_AREA);

class CSortFindTlv :public IUserDataProc
{
public:
    CSortFindTlv() {};
    ~CSortFindTlv() {};
    virtual DWORD GetKey(BYTE *pNode);
    void SwitchNode(BYTE *pNode1, BYTE *pNode2);
};

DWORD CSortFindTlv::GetKey(BYTE *pNode)
{
    TLV_MOD_CMD_REG *p = (TLV_MOD_CMD_REG*)pNode;
    return p->belong_module;
}

void CSortFindTlv::SwitchNode(BYTE *pNode1, BYTE *pNode2)
{
    TLV_MOD_CMD_REG *pMod1 = (TLV_MOD_CMD_REG*)pNode1;
    TLV_MOD_CMD_REG *pMod2 = (TLV_MOD_CMD_REG*)pNode2;
    TLV_MOD_CMD_REG temp = *pMod1;
    *pMod1 = *pMod2;
    *pMod2 = temp;
}


class CSortFindCmdArea :public IUserDataProc
{
public:
    CSortFindCmdArea() {};
    ~CSortFindCmdArea() {};
    virtual DWORD GetKey(BYTE *pNode);
    virtual void SwitchNode(BYTE *pNode1, BYTE *pNode2);
};

DWORD CSortFindCmdArea::GetKey(BYTE *pNode)
{
    CMD_NODE_AREA *p = (CMD_NODE_AREA*)pNode;
    return p->wCmdBegin;
}
/*  */
void CSortFindCmdArea::SwitchNode(BYTE *pNode1, BYTE *pNode2)
{
    CMD_NODE_AREA *pMod1 = (CMD_NODE_AREA*)pNode1;
    CMD_NODE_AREA *pMod2 = (CMD_NODE_AREA*)pNode2;
    CMD_NODE_AREA temp = *pMod1;
    *pMod1 = *pMod2;
    *pMod2 = temp;
}

/* 在区域中查找 */
class CSortFindCmdAreaFind :public IUserDataProc
{
public:
    CSortFindCmdAreaFind() {};
    ~CSortFindCmdAreaFind() {};
    virtual DWORD GetKey(BYTE *pNode);
    virtual void SwitchNode(BYTE *pNode1, BYTE *pNode2);
    /*
    -1:key1<key2
    0  :key1=key2
    1  :key1>key2
    */
    virtual int CompareKey(DWORD key1, DWORD key2value);
};
DWORD CSortFindCmdAreaFind::GetKey(BYTE *pNode)
{
    CMD_NODE_AREA *p = (CMD_NODE_AREA*)pNode;
    return ((((DWORD)p->wCmdBegin) << 16) | (DWORD)p->wCmdEnd);
}

int CSortFindCmdAreaFind::CompareKey(DWORD key1, DWORD key2value)
{
    WORD wBegin = key1 >> 16;
    WORD wEnd = key1 & 0xffff;
    /* 返回值必须和缺省的一致，否则方向会反 */
    if (wBegin > key2value)
    {
        return 1;
    }

    if ((wBegin <= key2value) && (wEnd >= key2value))
    {
        return 0;
    }

    return -1;
}

void CSortFindCmdAreaFind::SwitchNode(BYTE *pNode1, BYTE *pNode2)
{
    // 这个只是查找，不应该有节点交换
    ASSERT(0);
}



// 系统启动时自动对g_module_cmd_reg进行排序:采用快速排序方法
void module_cmd_pkg_helper_init()
{
    // 2017.7.30 64位下排序有问题（后面再调试，这里注册的时候注意好）
    // 命令格式排序 
    // CSortFindTlv fmttools;
    // CQuickSortTools::quickSort3Way((BYTE*)g_module_cmd_reg,sizeof(TLV_MOD_CMD_REG),0,MOD_TLV_REG_SIZE- 1,&fmttools);
    // 不排序了，改为校验
    DWORD i = 0;
    DWORD errornums = 0;
    for (i = 1; i < MOD_TLV_REG_SIZE; i++)
    {
        if (g_module_cmd_reg[i - 1].belong_module >= g_module_cmd_reg[i].belong_module)
        {
            errornums++;
            dcf_output("mod(0x%x) reg error!\r\n", g_module_cmd_reg[i - 1].belong_module);
        }
    }

    // 命令的区域排序
    CSortFindCmdArea areatools;
    // CQuickSortTools::quickSort3Way((BYTE*)g_CmdAreaList,sizeof(CMD_NODE_AREA),0,CMD_AREA_REG_SIZE- 1,&fmttools);
    for (i = 1; i < CMD_AREA_REG_SIZE; i++)
    {
        if (g_CmdAreaList[i - 1].wCmdBegin > g_CmdAreaList[i].wCmdBegin)
        {
            errornums++;
            dcf_output("mod(0x%x),begin(0x%x) reg error!\r\n", g_CmdAreaList[i - 1].wModid, g_CmdAreaList[i - 1].wCmdBegin);
        }
        if (g_CmdAreaList[i - 1].wCmdEnd > g_CmdAreaList[i].wCmdBegin)
        {
            errornums++;
            dcf_output("mod(0x%x),end(0x%x) reg error!\r\n", g_CmdAreaList[i - 1].wModid, g_CmdAreaList[i - 1].wCmdEnd);
        }
    }

    if (errornums)
    {
        ASSERT(0);
    }
}

const char *dcf_tools_get_mod_cmd_struct_trans_info(WORD modid, WORD wCmd, BYTE bcmd)
{
#if 1
    CSortFindTlv tools;
    DWORD dwPt = CQuickFindTools::quickFind2Way((BYTE*)g_module_cmd_reg, sizeof(TLV_MOD_CMD_REG), MOD_TLV_REG_SIZE, modid, &tools);
#else
    DWORD dwPt = 0;
    for (; dwPt < MOD_TLV_REG_SIZE; dwPt++)
    {
        if (modid == g_module_cmd_reg[dwPt].belong_module)
        {
            break;
        }
    }
#endif

    if (dwPt >= MOD_TLV_REG_SIZE)
    {
        return NULL;
    }

    // 再在对应模块的命令列表中遍历
    TLV_CMD_PKG_INFO *cmdlist = g_module_cmd_reg[dwPt].regs;
    WORD CmdNums = (WORD)g_module_cmd_reg[dwPt].nums;
    for (WORD i = 0; i < CmdNums; i++)
    {
        if (cmdlist[i].cmd == wCmd)
        {
            if (bcmd) return cmdlist[i].cmd_stru_script;
            return cmdlist[i].rsp_stru_script;
        }
    }
    return NULL;
}

const char *dcf_tools_get_cmd_fmt_info(WORD wCmd, BYTE bcmd)
{
    // 1.先根据命令区域找
#if 1
    CSortFindCmdAreaFind areatools;
    DWORD dwPt = CQuickFindTools::quickFind2Way((BYTE*)g_CmdAreaList, sizeof(CMD_NODE_AREA), CMD_AREA_REG_SIZE, wCmd, &areatools);
#else
    DWORD dwPt = 0;
    for (dwPt = 0; dwPt < CMD_AREA_REG_SIZE; dwPt++)
    {
        if ((wCmd >= g_CmdAreaList[dwPt].wCmdBegin) && (wCmd <= g_CmdAreaList[dwPt].wCmdEnd))
        {
            break;
        }
    }
#endif

    if (dwPt >= MOD_TLV_REG_SIZE)
    {
        return NULL;
    }
    return dcf_tools_get_mod_cmd_struct_trans_info(g_CmdAreaList[dwPt].wModid, wCmd, bcmd);
}

/****************************************************************
*功能描述 : 网络字节序转换为主机字节序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-17  22:56:39
****************************************************************/
DWORD dcf_pkg_tool::cvt_fmt_ntoh(void *psrc, const char *fmt, void *pdst, WORD &Offset, int srclen, int dstlen)
{
    BYTE *pRead = (BYTE *)psrc;
    BYTE *pWrite = (BYTE *)pdst;
    int iReadLen = 0;
    int iWriteLen = 0;
#define CHECK_READ_LEN(inc) \
        iReadLen += inc;   if (srclen && (iReadLen > srclen)) return DCF_ERR_PARAM
#undef CHECK_WRITE_LEN
#define CHECK_WRITE_LEN(inc) \
        iWriteLen += inc;   if (dstlen && (iWriteLen > dstlen)) return DCF_ERR_PARAM

    while (*fmt)
    {
        char ch = *fmt;
        switch (ch)
        {
            /* BYTE */
        case 'b':
        case 'B':
        {
            CHECK_READ_LEN(sizeof(BYTE));
            CHECK_WRITE_LEN(sizeof(BYTE));
            if (pWrite != pRead)
            {
                *pWrite = *pRead;
            }
            pWrite++;
            pRead++;
            break;
        }
        /* WORD */
        case 'w':
        case 'W':
        {
            CHECK_READ_LEN(sizeof(WORD));
            CHECK_WRITE_LEN(sizeof(WORD));
            WORD w = *(WORD*)pRead;
            *(WORD*)pWrite = dcf_ntohs(w);
            pWrite += sizeof(WORD);
            pRead += sizeof(WORD);
            break;
        }
        /* DWORD */
        case 'd':
        case 'D':
        {
            CHECK_READ_LEN(sizeof(DWORD));
            CHECK_WRITE_LEN(sizeof(DWORD));
            DWORD dw = *(DWORD*)pRead;
            *(DWORD*)pWrite = dcf_ntohl(dw);
            pWrite += sizeof(DWORD);
            pRead += sizeof(DWORD);
            break;
        }
		/* uint64 */
		case 'q':
		case 'Q':
		{
			CHECK_READ_LEN(sizeof(uint64));
			CHECK_WRITE_LEN(sizeof(uint64));
			uint64 dw = *(uint64*)pRead;
			*(uint64*)pWrite = dcf_ntohl(dw);
			pWrite += sizeof(uint64);
			pRead += sizeof(uint64);
			break;
		}
        case 's':
        case 'S':
        case 'f':
        case 'F':
        {
            /* 先得到后面指定的长度 */
            fmt++;
            char *pEnd = NULL;
            int ilen = dcf_strtools::strtol(fmt, 0, &pEnd);
            if (ilen <= 0)
            {
                return DCF_ERR_PARAM;
            }

            if (pWrite != pRead)
            {
                memmove((char*)pWrite, (char*)pRead, ilen);
            }

            pWrite += ilen;
            pRead += ilen;

            if (pEnd)
            {
                fmt = pEnd;
                fmt--; /* 后面有一个++ */
            }
            else
            {
                goto Fmtend;
            }
            break;
        }
        case 'v':
        case 'V':
        {
            /* 一定是最后一个参数 */
            ASSERT((*(fmt + 1)) == 0);
            int iLeftLen = srclen - iReadLen;
            if ((iLeftLen > 0) && (pRead != pWrite))
            {
                memcpy(pWrite, pRead, iLeftLen);
                iWriteLen += iLeftLen;
            }

            /* 最后一个跳出循环 */
            goto Fmtend;
        }
        case '-':
        case '/':
        case ' ':
        {
            // 分解符
            break;
        }

        default:
        {
            // 其它类型后面再添加
            return DCF_ERR_PARAM;
        }
        }
        fmt++;
    }

Fmtend:
    Offset += (WORD)iWriteLen;
    return 0;
}

DWORD dcf_pkg_tool::cvt_fmt_hton(void *psrc, const char *fmt, void *pdst, WORD &Offset, int srclen, int dstlen)
{
    BYTE *pRead = (BYTE *)psrc;
    BYTE *pWrite = (BYTE *)pdst;
    int iReadLen = 0;
    int iWriteLen = 0;
#define CHECK_READ_LEN(inc) \
        iReadLen += inc;   if (srclen && (iReadLen > srclen)) return DCF_ERR_PARAM
#define CHECK_WRITE_LEN(inc) \
        iWriteLen += inc;   if (dstlen && (iWriteLen > dstlen)) return DCF_ERR_PARAM

    while (*fmt)
    {
        char ch = *fmt;
        switch (ch)
        {
            /* BYTE */
        case 'b':
        case 'B':
        {
            CHECK_READ_LEN(sizeof(BYTE));
            CHECK_WRITE_LEN(sizeof(BYTE));
            if (pWrite != pRead)
            {
                *pWrite = *pRead;
            }
            pWrite++;
            pRead++;
            break;
        }
        /* WORD */
        case 'w':
        case 'W':
        {
            CHECK_READ_LEN(sizeof(WORD));
            CHECK_WRITE_LEN(sizeof(WORD));
            WORD w = *(WORD*)pRead;
            *(WORD*)pWrite = dcf_htons(w);
            pWrite += sizeof(WORD);
            pRead += sizeof(WORD);
            break;
        }
        /* DWORD */
        case 'd':
        case 'D':
        {
            CHECK_READ_LEN(sizeof(DWORD));
            CHECK_WRITE_LEN(sizeof(DWORD));
            DWORD dw = *(DWORD*)pRead;
            *(DWORD*)pWrite = dcf_htonl(dw);
            pWrite += sizeof(DWORD);
            pRead += sizeof(DWORD);
            break;
        }
		/* uint64 */
		case 'q':
		case 'Q':
		{
			CHECK_READ_LEN(sizeof(uint64));
			CHECK_WRITE_LEN(sizeof(uint64));
			uint64 dw = *(uint64*)pRead;
			*(uint64*)pWrite = dcf_htonl(dw);
			pWrite += sizeof(uint64);
			pRead += sizeof(uint64);
			break;
		}
        /* S16 */
        case 's':
        case 'S':
        case 'f':
        case 'F':
        {
            /* 先得到后面指定的长度 */
            fmt++;
            char *pEnd = NULL;
            int ilen = dcf_strtools::strtol(fmt, 0, &pEnd);
            if (ilen <= 0)
            {
                return DCF_ERR_PARAM;
            }

            if (pWrite != pRead)
            {
                memmove((char*)pWrite, (char*)pRead, ilen);
            }

            pWrite += ilen;
            pRead += ilen;

            if (pEnd)
            {
                fmt = pEnd;
                fmt--; /* 后面有一个++ */
            }
            else
            {
                goto Fmtend;
            }
            break;
        }
        case 'v':
        case 'V':
        {
            /* 一定是最后一个参数 */
            ASSERT((*(fmt + 1)) == 0);
            int iLeftLen = srclen - iReadLen;
            if ((iLeftLen > 0) && (pRead != pWrite))
            {
                memcpy(pWrite, pRead, iLeftLen);
                iWriteLen += iLeftLen;
            }

            /* 最后一个跳出循环 */
            goto Fmtend;
        }
        case '-':
        case '/':
        case ' ':
        {
            // 分解符
            break;
        }
        default:
        {
            // 其它类型后面再添加
            return DCF_ERR_PARAM;
        }
        }
        fmt++;
    }

Fmtend:

    Offset += (WORD)iWriteLen;

    return 0;
}
/****************************************************************
*功能描述 : 根据模块id将结构体从网络序转为主机序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  17:52:19
****************************************************************/
DWORD dcf_pkg_tool::cvt_mod_tlv_ntoh(void *psrc, CRM_CMD &Out, WORD wModid, WORD &Offset, int srclen)
{
    CATCH_ERR_RET(!psrc, DCF_ERR_PARAM);
    CATCH_ERR_RET(!wModid, DCF_ERR_PARAM);

    BYTE *pCur = (BYTE *)psrc;
    // 先转头部
    cvt_fmt_ntoh(pCur, struct_fmt_crm_cmd, (void*)&Out, Offset);
    BYTE bCmd = Out.IsCmd() ? 1 : 0;
    // 得到命令字之后再查找
    const char *parafmt = "w";
    if (bCmd || ((!bCmd) && (Out.wParaLen > sizeof(WORD))))
    {
        parafmt = dcf_tools_get_mod_cmd_struct_trans_info(wModid, Out.wCmd, bCmd);
    }

    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    // 校验一下参数
    if ((Out.wParaLen + RCC_SIZE_CRM_TLV) > srclen)
    {
        return DCF_ERR_INVALID_DATA;
    }

    Offset += RCC_SIZE_CRM_TLV;
    pCur += RCC_SIZE_CRM_TLV;
    Out.pbyPara = pCur;
    return cvt_fmt_ntoh(pCur, parafmt, (void*)pCur, Offset);
}

/****************************************************************
*功能描述 : 将命令转换为主机序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-24  16:40:8
****************************************************************/
DWORD dcf_pkg_tool::cvt_tlv_ntoh(void *psrc, CRM_CMD &Out, WORD &Offset, int srclen)
{
    CATCH_ERR_RET(!psrc, DCF_ERR_PARAM);

    BYTE *pCur = (BYTE *)psrc;
    // 先转头部
    cvt_fmt_ntoh(pCur, struct_fmt_crm_cmd, (void*)&Out, Offset);
    BYTE bCmd = Out.IsCmd() ? 1 : 0;
    /* 根据命令找模块id */
    // 得到命令字之后再查找
    const char *parafmt = "w";
    if (bCmd || ((!bCmd) && (Out.wParaLen > sizeof(WORD))))
    {
        parafmt = dcf_tools_get_cmd_fmt_info(Out.wCmd, bCmd);
    }

    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    // 校验一下参数
    if ((Out.wParaLen + RCC_SIZE_CRM_TLV) > srclen)
    {
        return DCF_ERR_INVALID_DATA;
    }

    // Offset += RCC_SIZE_CRM_TLV;
    pCur += RCC_SIZE_CRM_TLV;
    Out.pbyPara = pCur;
    return cvt_fmt_ntoh(pCur, parafmt, (void*)pCur, Offset);
}

/****************************************************************
*功能描述 : 直接根据格式转换为主机序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  18:2:38
****************************************************************/
DWORD dcf_pkg_tool::cvt_fmt_tlv_ntoh(void *psrc, CRM_CMD &Out, const char *fmt, WORD &Offset, int srclen)
{
    CATCH_ERR_RET(!psrc, DCF_ERR_PARAM);
    CATCH_ERR_RET(!fmt, DCF_ERR_PARAM);

    BYTE *pCur = (BYTE *)psrc;
    // 先转头部
    cvt_fmt_ntoh(pCur, struct_fmt_crm_cmd, (void*)&Out, Offset);

    // 校验一下参数
    if ((Out.wParaLen + RCC_SIZE_CRM_TLV) > srclen)
    {
        return DCF_ERR_INVALID_DATA;
    }

    Offset += RCC_SIZE_CRM_TLV;
    pCur += RCC_SIZE_CRM_TLV;
    Out.pbyPara = pCur;

    if (Out.IsRsp() && (Out.wParaLen == sizeof(WORD)))
    {
        return cvt_fmt_ntoh(pCur, "w", (void*)pCur, Offset);
    }
    return cvt_fmt_ntoh(pCur, fmt, (void*)pCur, Offset);
}

/****************************************************************
*功能描述 : 根据模块id将tlv从主机序转为网络序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  18:3:38
****************************************************************/
DWORD dcf_pkg_tool::cvt_mod_tlv_hton(CRM_CMD &In, void *pdst, WORD wModid, WORD &Offset, int dstlen)
{
    CATCH_ERR_RET(!pdst, DCF_ERR_PARAM);
    CATCH_ERR_RET(!wModid, DCF_ERR_PARAM);

    // 得到命令字之后再查找
    BYTE bCmd = In.IsCmd() ? 1 : 0;
    const char *parafmt = "w";
    if (bCmd || (In.wParaLen > sizeof(WORD)))
    {
        parafmt = dcf_tools_get_mod_cmd_struct_trans_info(wModid, In.wCmd, bCmd);
    }

    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    return cvt_fmt_tlv_hton(In, pdst, parafmt, Offset, dstlen);
}

/****************************************************************
*功能描述 : 直接根据格式将tlv从主机序转换为网络序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  18:6:44
****************************************************************/
DWORD dcf_pkg_tool::cvt_fmt_tlv_hton(CRM_CMD &In, void *pdst, const char *fmt, WORD &Offset, int dstlen)
{
    CATCH_ERR_RET(!pdst, DCF_ERR_PARAM);
    CATCH_ERR_RET(!fmt, DCF_ERR_PARAM);

    // Offset += RCC_SIZE_CRM_TLV;
    DWORD dwRet;
    // 转参数部分
    if (In.IsRsp() && (In.wParaLen == sizeof(WORD)))
    {
        // 是响应，而且只有一个错误码，则不能用缺省的响应格式
        dwRet = cvt_fmt_hton((void*)In.pbyPara, "w", (void*)((BYTE*)pdst + RCC_SIZE_CRM_TLV), Offset);
    }
    else
    {
        dwRet = cvt_fmt_hton((void*)In.pbyPara, fmt, (void*)((BYTE*)pdst + RCC_SIZE_CRM_TLV), Offset);
    }

    // 转换头部
    dwRet = (dwRet == 0) ? cvt_fmt_hton((void*)&In, struct_fmt_crm_cmd, (void*)pdst, Offset) : dwRet;
    return dwRet;
}
/****************************************************************
*功能描述 : 根据命令将信息从主机序转换为网络序
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-24  16:51:9
****************************************************************/
DWORD dcf_pkg_tool::cvt_tlv_hton(CRM_CMD &In, void *pdst, WORD &Offset, int dstlen)
{
    CATCH_ERR_RET(!pdst, DCF_ERR_PARAM);

    // 得到命令字之后再查找
    BYTE bCmd = In.IsCmd() ? 1 : 0;
    const char *parafmt = "w";
    if (bCmd || (In.wParaLen > sizeof(WORD)))
    {
        parafmt = dcf_tools_get_cmd_fmt_info(In.wCmd, bCmd);
    }

    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    return cvt_fmt_tlv_hton(In, pdst, parafmt, Offset, dstlen);
}

/****************************************************************
*功能描述 : 按照缺省格式进行命令打包
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-23  14:24:19
****************************************************************/
DWORD dcf_para_fmt::PacketParam_Mid(CRM_CMD &Cmd, WORD modid, ...)
{
    // 根据模块和命令查找格式
    BYTE bCmd = Cmd.IsCmd() ? 1 : 0;
    const char *parafmt = dcf_tools_get_mod_cmd_struct_trans_info(modid, Cmd.wCmd, bCmd);
    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    WORD Offset = 0;

    va_list marker;
    va_start(marker, modid);
    DWORD dwRet = PacketParam_Fm(Cmd, Offset, parafmt, marker);
    va_end(marker);

    // 修订长度
    Cmd.wParaLen = Offset;
    return dwRet;
}

/****************************************************************
*功能描述 : 根据命令进行封装包
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-21  11:6:31
****************************************************************/
DWORD dcf_para_fmt::PacketParam_Cmd(CRM_CMD &Cmd, BYTE bCmd, ...)
{
    const char *fmt = dcf_tools_get_cmd_fmt_info(Cmd.wCmd, bCmd);
    if (!fmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    WORD Offset = 0;
    va_list marker;
    va_start(marker, bCmd);
    DWORD dwRet = PacketParam_Fm(Cmd, Offset, fmt, marker);
    va_end(marker);

    // 修订长度
    Cmd.wParaLen = Offset;
    return dwRet;
}


DWORD dcf_para_fmt::PacketParam(CRM_CMD &Cmd, WORD &Offset, WORD modid, va_list marker)
{
    // 根据模块和命令查找格式
    BYTE bCmd = Cmd.IsCmd() ? 1 : 0;
    const char *parafmt = dcf_tools_get_mod_cmd_struct_trans_info(modid, Cmd.wCmd, bCmd);
    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }
    DWORD dwRet = PacketParam_Fm(Cmd, Offset, parafmt, marker);
    Cmd.wParaLen = Offset;
    return dwRet;
}

DWORD dcf_para_fmt::PacketParam(CRM_CMD &Cmd, WORD &Offset, va_list marker)
{
    BYTE bCmd = Cmd.IsCmd() ? 1 : 0;
    const char *parafmt = dcf_tools_get_cmd_fmt_info(Cmd.wCmd, bCmd);
    if (!parafmt)
    {
        return DCF_ERR_NOT_EXIST;
    }

    DWORD dwRet = PacketParam_Fm(Cmd, Offset, parafmt, marker);
    Cmd.wParaLen = Offset;
    return dwRet;
}

/****************************************************************
*功能描述 : 参数封装
                   特别注意事项:
                   1.linux下va_arg后的参数类型，必须最小为int，4个字节，否则会出现异常。
                      原因:linux内核的va_arg和windows不一样，它是按照类型压栈，而对于数字
                      确实是int，如果压栈时按照int压栈，读的时候按照byte读，那么就会导致访问非法地址
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-08-23  8:45:38
****************************************************************/
DWORD dcf_para_fmt::PacketParam_Fm(CRM_CMD &Cmd, WORD &Offset, const char *parafmt, va_list marker)
{
    if ((!parafmt) || (!*parafmt) || (!Cmd.pbyPara) || (!Cmd.wParaLen))
    {
        return DCF_ERR_PARAM;
    }

    BYTE *pCur = Cmd.pbyPara;
    WORD wBufLen = Cmd.wParaLen;
    WORD para_nums = 0;

#undef CHECK_WRITE_LEN
#define CHECK_WRITE_LEN(inc) \
        Offset += inc; para_nums++;  if (Offset > wBufLen) return DCF_ERR_PARAM


    while (*parafmt)
    {
        char ch = *parafmt;
        switch (ch)
        {
        case 'B':
        case 'b':
        {
            CHECK_WRITE_LEN(sizeof(BYTE));
            *pCur = (BYTE)va_arg(marker, DWORD);
            pCur++;
            break;
        }
        case 'W':
        case 'w':
        {
            CHECK_WRITE_LEN(sizeof(WORD));
            WORD w = (WORD)va_arg(marker, DWORD);
            memcpy(pCur, &w, sizeof(WORD));
            pCur += sizeof(WORD);
            if (Cmd.IsRsp() && w && (para_nums == 1))
            {
                /* 2017-08-21  10:23:18 是响应，而且失败了，则跳出其它参数处理 */
                goto Fmtend;
            }
            break;
        }
        case 'D':
        case 'd':
        {
            CHECK_WRITE_LEN(sizeof(DWORD));
            DWORD dw = va_arg(marker, DWORD);
            memcpy(pCur, &dw, sizeof(DWORD));
            pCur += sizeof(DWORD);
            break;
        }
		case 'Q':
		case 'q':
		{
			CHECK_WRITE_LEN(sizeof(uint64));
			uint64 dw = va_arg(marker, uint64);
			memcpy(pCur, &dw, sizeof(uint64));
			pCur += sizeof(uint64);
			break;
		}
        case 'S':
        case 's':
        {
            parafmt++;
            char *pEnd = NULL;
            int ilen = dcf_strtools::strtol(parafmt, 0, &pEnd);
            if (ilen <= 0)
            {
                return DCF_ERR_PARAM;
            }

            CHECK_WRITE_LEN(ilen);
            char * str = va_arg(marker, char*);
            dcf_strtools::strcpy_s((char*)pCur, str, ilen);
            pCur += ilen;

            if (pEnd)
            {
                parafmt = pEnd;
                parafmt--; /* 后面有一个++ */
            }
            else
            {
                goto Fmtend;
            }

            break;
        }
        case 'f':
        case 'F':
        {
            parafmt++;
            char *pEnd = NULL;
            int ilen = dcf_strtools::strtol(parafmt, 0, &pEnd);
            if (ilen <= 0)
            {
                return DCF_ERR_PARAM;
            }

            CHECK_WRITE_LEN(ilen);
            BYTE * ptr = va_arg(marker, BYTE*);
            memcpy(pCur, ptr, ilen);
            pCur += ilen;

            if (pEnd)
            {
                parafmt = pEnd;
                parafmt--; /* 后面有一个++ */
            }
            else
            {
                goto Fmtend;
            }

            break;
        }
        case 'v':
        case 'V':
        {
            WORD wLen = (WORD)va_arg(marker, DWORD);
            CHECK_WRITE_LEN(wLen);
            BYTE * ptr = va_arg(marker, BYTE*);
            memcpy(pCur, ptr, wLen);
            pCur += wLen;

            /* 最后一个跳出循环 */
            goto Fmtend;
        }
        case '-':
        case '/':
        case ' ':
        {
            // 分解符
            break;
        }
        default:
        {
            // 其它类型后面再添加
            return DCF_ERR_PARAM;
        }
        }
        parafmt++;
    }

Fmtend:
    return 0;
}


#define CHECK_LEN(a) \
    if ((m_cursor + (a)) > m_Len) {m_error = 1;return *this;}

dcf_para_in::dcf_para_in(BYTE *pBuf, DWORD Len)
{
    ASSERT(pBuf != NULL);
    m_buf = pBuf;
    m_Len = Len;
    m_cursor = 0;
    m_error = 0;
}

dcf_para_in &dcf_para_in::operator <<(BYTE p)
{
    CHECK_LEN(sizeof(BYTE));
    m_buf[m_cursor++] = p;
    return *this;
}
dcf_para_in &dcf_para_in::operator <<(WORD p)
{
    CHECK_LEN(sizeof(WORD));
    memcpy(m_buf + m_cursor, &p, sizeof(WORD));
    m_cursor += sizeof(WORD);
    return *this;
}
dcf_para_in &dcf_para_in::operator <<(DWORD p)
{
    CHECK_LEN(sizeof(DWORD));
    memcpy(m_buf + m_cursor, &p, sizeof(DWORD));
    m_cursor += sizeof(DWORD);
    return *this;
}

dcf_para_in &dcf_para_in::operator <<(uint64 p)
{
	CHECK_LEN(sizeof(uint64));
	memcpy(m_buf + m_cursor, &p, sizeof(uint64));
	m_cursor += sizeof(uint64);
	return *this;
}

dcf_para_in &dcf_para_in::operator<<(char *p)
{
    int ilen = (int)strlen(p) + 1;
    CHECK_LEN(ilen);
    dcf_strtools::strcpy_s((char*)(m_buf + m_cursor), p, ilen);
    m_cursor += ilen;
    return *this;
}

bool dcf_para_in::write_string_vs(char *p)
{
    int ilen = (int)strlen(p) + 1;
    if ((m_cursor + (ilen)) > m_Len)
    {
        return false;
    }

    dcf_strtools::strcpy_s((char*)(m_buf + m_cursor), p, ilen);
    m_cursor += ilen;
    return true;
}


bool dcf_para_in::write_string(char *buffer, WORD para_len)
{
    if ((para_len + m_cursor) > m_Len) return false;
    dcf_strtools::strcpy_s((char*)(m_buf + m_cursor), buffer, para_len);
    m_cursor += para_len;
    return true;
}


dcf_para_out::dcf_para_out(BYTE *pBuf, DWORD Len)
{
    ASSERT(pBuf != NULL);
    m_buf = pBuf;
    m_Len = Len;
    m_cursor = 0;
    m_error = 0;
}

dcf_para_out &dcf_para_out::operator >> (BYTE &p)
{
    CHECK_LEN(sizeof(BYTE));
    p = m_buf[m_cursor++];
    return *this;
}
dcf_para_out &dcf_para_out::operator >> (WORD &p)
{
    CHECK_LEN(sizeof(WORD));
    memcpy(&p, m_buf + m_cursor, sizeof(WORD));
    m_cursor += sizeof(WORD);
    return *this;
}
dcf_para_out &dcf_para_out::operator >> (DWORD &p)
{
    CHECK_LEN(sizeof(DWORD));
    memcpy(&p, m_buf + m_cursor, sizeof(DWORD));
    m_cursor += sizeof(DWORD);
    return *this;
}

dcf_para_out &dcf_para_out::operator >> (uint64 &p)
{
	CHECK_LEN(sizeof(uint64));
	memcpy(&p, m_buf + m_cursor, sizeof(uint64));
	m_cursor += sizeof(uint64);
	return *this;
}

dcf_para_out &dcf_para_out::operator >> (char *p)
{
    int ilen = (int)strlen((char*)m_buf + m_cursor) + 1;
    CHECK_LEN(ilen);
    memcpy(p, m_buf + m_cursor, ilen);
    m_cursor += ilen;
    return *this;
}

bool dcf_para_out::read_string_vs(char *buffer, WORD para_len)
{
    int ilen = (int)strlen((char*)m_buf + m_cursor) + 1;
    buffer[0] = 0;
    if ((ilen > para_len) || ((m_cursor + (ilen)) > m_Len))
    {
        return false;
    }

    memcpy(buffer, m_buf + m_cursor, ilen);
    m_cursor += ilen;
    return true;
}

const char* dcf_para_out::get_str_vs_ptr(WORD max_len)
{
    int ilen = (int)strlen((char*)m_buf + m_cursor) + 1;
    if ((ilen > max_len) || ((m_cursor + (ilen)) > m_Len))
    {
        return NULL;
    }
    const char *p = (const char *)(m_buf + m_cursor);
    m_cursor += ilen;
    return p;
}

bool dcf_para_out::read_string(char *buffer, WORD para_len)
{
    if ((m_cursor + para_len) > m_Len) return false;
    memcpy(buffer, m_buf + m_cursor, para_len);
    buffer[para_len - 1] = 0;
    m_cursor += para_len;
    return true;
}

BYTE* dcf_para_out::get_para_ptr(DWORD para_len)
{
    if ((m_cursor + para_len) > m_Len) return NULL;
    BYTE *p = m_buf + m_cursor;
    m_cursor += para_len;
    return p;
}


