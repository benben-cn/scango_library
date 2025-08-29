
#ifndef _DCF_I_RCC_DTH_H
#define _DCF_I_RCC_DTH_H

/****************************************************************
*文件范围 : 采用RCC通信的报文通用头部，特别注意，该结构体由RCC模块进行填写和字节序转换
*设计说明 : 2017-06-22  18:15:48 
                   A)跨设备订阅服务
                   1.需要采用RAW_RCC通道的方式
                   2.需要用MSG_RPC结构体，其中
                      a)MSG_RPC.wMsg1描述信息类型("INFO_TYPE_*")
                      b)MSG_RPC.wMsg2描述模块订阅者的modid
                   3.默认要求:订阅者接收响应消息需要使用entryid为1的队列接收消息(CRccFrame实在没有更多的地方传递这一信息，必须为16字节的倍数)
                   B)设备内部同进程订阅服务
                   1.采用dmm消息格式，dmm里面有详尽的寻址方式

                   下面重点说明一下最复杂的跨设备订阅业务的设计，涉及环节多
                   A)消息达到业务提供者的方向
                   1.发送方调用本端通信层接口发送数据
                      必须提供MSG_RPC结构体，并按照如上约束填写，并填写是命令类型
                   2.本端通信层发送报文格式要求
                      a)CRccFrame.wMsg1 = MSG_RPC.wMsg1
                      b)CRccFrame.wMsg2 = MSG_RPC.wMsg2
                      c)CRccFrame.ctrl = MSG_RPC.ctrl
                   3.对端通信层组包后的格式
                      a)CRccFrame.wMsg1 = MSG_RPC.wMsg1
                      b)CRccFrame.wMsg2 = MSG_RPC.wMsg2
                      c)判断ctrl为命令，则根据CRccFrame.wMsg1查找处理该业务的模块id和消息入口id
                      d)发送消息(MSG_RPC_MM)结构体)至对应队列
                   4.业务提供者
                      a)收消息
                      b)得到MSG_RPC_MM结构体，知道远端MSG_RPC.wMsg2

                   B)响应消息到达订阅者
                   1.业务提供方调用本端通信层接口
                      提供MSG_RPC结构体,控制字填写为响应类型，其它MSG_RPC.wMsg1和MSG_RPC.wMsg2回填即可
                   2.本端通信层发送报文格式要求
                      a)CRccFrame.wMsg1 = MSG_RPC.wMsg1
                      b)CRccFrame.wMsg2 = MSG_RPC.wMsg2
                      c)CRccFrame.ctrl = MSG_RPC.ctrl
                   3.对端通信层组包后的格式
                      a)CRccFrame.wMsg1 = MSG_RPC.wMsg1
                      b)CRccFrame.wMsg2 = MSG_RPC.wMsg2
                      c)判断ctrl为响应，则使用CRccFrame.wMsg2为模块id，消息入口id为1，调用dcf_module_get_task_and_msgque_id查找对应任务id和队列id，发送MSG_RPC_MM消息
                    4.业务订阅者处理响应
                      a)收消息，得到MSG_RPC_MM结构体
                      b)处理数据                      
                   
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-17  16:36:2
1修改描述 : 为了降低移动端java的复杂性，系统间均采用raw方式通信
                   为了减少消息的字节序转换次数，采用发送方不转换字节序，收方根据flag标记来区分是否需要转换的模式
*修改作者 : zjb
*修改时间 : 2017-06-16  9:26:53
****************************************************************/
#include "dcf_def.h"
#include "dcf_msg0.h"

/* 信息帧类型 为了防止和别的应用混淆，信息帧从0x8000开始编排*/
/* 当前版本为了简化，添加类型之后，还需要在 dcf_sv_info_provider_register() 中注册，而且还需要调用该函数注册，否则不生效 */
const WORD INFO_TYPE_BSC_CHANNEL = 0x8001;     /* BS和BC之间的管理通道报文 */

const WORD INFO_TYPE_BS_BEGIN = 0x8002;
const WORD INFO_TYPE_BS_GENCODE = 0x8002;       /* 二维码创建服务 */
const WORD INFO_TYPE_BS_CODE = 0x8003;               /* 二维码服务,含二维码查询、提交、入库、出库、存储等 */
const WORD INFO_TYPE_BS_BSDB = 0x8004;               /* 企业基本信息查询 */
const WORD INFO_TYPE_BS_INSTORE = 0x8005;        /* 企业商品入库服务 */
const WORD INFO_TYPE_BS_STOCK = 0x8007;            /* 企业商品出库服务 */
const WORD INFO_TYPE_BS_INTERFACE = 0x8010;          /* 与BS界面通信 */

const WORD INFO_TYPE_BS_END = 0x8020;                 /* 超出范围之后一定要调整该值 */



const WORD INFO_TYPE_SPS_CHANNEL = 0x8101;     /* SPC和SPS之间的管理通道报文 */

const WORD INFO_TYPE_BSS_CHANNEL = 0x8201;    /* BSS和BSC之间的管理通道报文*/

const WORD INFO_TYPE_BSS_INSTORE = 0x8205;        /* 生产企业商品入库服务 */
const WORD INFO_TYPE_BSS_BSDB = 0x8206;               /* 企业基本信息查询 */
const WORD INFO_TYPE_BSS_OUTSTORE = 0x8207;     /* 企业商品出库服务 */
const WORD INFO_TYPE_BSS_PINSTORE = 0x8208;      /* 企业采购入库服务 */
const WORD INFO_TYPE_BSS_RELATED = 0x8209;        /* 重关联服务 */  
const WORD INFO_TYPE_BSS_QRGET = 0x820a;            /* 命令查询QR信息 */
const WORD INFO_TYPE_BSS_PCRING = 0x820b;          /* 供货商圈和客户圈维护 */
const WORD INFO_TYPE_BSS_PSDB = 0x820c;          /* 待收发货表查询 */
const WORD INFO_TYPE_BSS_DAMAGED = 0x820d;        /* 报损业务 */
const WORD INFO_TYPE_BSS_QRCREATE = 0x820e;        /* 企业二维码生成业务 */

const WORD INFO_TYPE_BSS_FPLANT = 0x8301;          /* 农产品种植模块业务 */
const WORD INFO_TYPE_BSS_FCIRCULATION = 0x8302;          /* 农产品流通模块业务 */

/* 用户发送报文时使用的结构体 */
#pragma pack(4)
struct MSG_RPC
{
    BYTE     *pMsg;           /* 不含有通信层的，只含有应用层的消息(和wMsgType对应) */    
    DWORD  MsgLen;       /* 应用层的消息长度(也是消息buffer长度，对于bsg内的消息，必须是16的倍数) */
    BYTE      ctrl;                 /* 0:无需确认 1:需要确认 2:第一帧 0x80:确认帧*/
    BYTE      bhost;   /* 收到消息方使用,对方是什么字节序:1同本机器 0:不同，由frame层感知和填写 */
    WORD    Msg_Secret_Ver;   /* 应用层的加密版本, 0未采用加密 */
    WORD    wMsg1;/* 各种消息帧含义不一样,raw_bsc对应为业务信息类型 */
    WORD    wMsg2;/* 各种消息帧含义不一样,raw_bsc对应为响应时收消息模块id，接收方自定义 */
    WORD    wMsgType;  /* 消息类型 */
    WORD    MsgPort;    
    DWORD  bs_id;           /* 企业id :在spc的业务服务层需要 */
    DWORD  dev_id;         /* 设备id:在spc的业务服务层需要*/
    DWORD  dev_no;       /* 设备编号 */
    DWORD  bs_sub_id;  /* 分厂id */
    DWORD  dev_optid;  /* 关联操作员 */
    DWORD  MsgIP;    
    DWORD  cirtifyid;        /* 认证码: 在bs圈中根据认证码传递信息 */    
};
#pragma pack()
typedef DWORD(*SENDMSGFUNC)(void *pMsg, WORD MsgLen, DWORD dstIP, WORD dstPort, WORD MsgType, BYTE bHost, void *pThis);
typedef DWORD(*RECVMSGFUNC)(MSG_RPC &msg, void *pThis);
typedef DWORD(*RESPONSEFUNC)(MSG_RPC& msg, void* pThis);
typedef void(*PROGRESSFUNC)(WORD modId, WORD curBegin, WORD totalFrame);
// 一个消息通道的信息
struct COMM_CHANNEL
{
    DWORD cirtifyid;
    DWORD dstIP;
    WORD   dstPort;
    WORD   Secret_Ver;
    SENDMSGFUNC Sender;   /* 发送消息的接口 */
    RECVMSGFUNC Recver;    /* 收数据的接口 */
    RESPONSEFUNC Response; /* 发送完成回复的接口 */ //add by hhw 2023.6.29 用于pda数据发送完成后自己应答自己
    PROGRESSFUNC Progress; /* 发送文件进度条的接口 */ //add by hhw 2023.12.4 用于pda数据发送数据时上报进度
    void*     pThis;
    const char *print_ownapp;  /* 打印app */
};

const DWORD RCC_MSG_B_MAXLEN = (20*1024*1024);     /* 设置一个最大的报文长度，用于bigdata */


/* 
通信头部的版本,起步大版本为0x02,判定兼容原则:
1.大版本必须相同，否则认为通信不兼容
2.客户端版本不能大于服务器版本，服务器兼容客户端
*/
const WORD RCC_VER = 0x0201;
const WORD RCC_CRY_VER_FRAME = 0x0101;

/* 
基本通信头 20 字节 
特别注意:如果调整了本信息头部，则需要修改 RCC_SIZE_HEADER 宏的值以及下面的值
*/

/* 
特别注意:
1.支持的帧类型定义 特别注意:添加类型之后，一定要将RCC_MSG_TYPE_MAX调整
2.MB类型必须要做几件事情:
   a)在dcf_mailbox_channel.cpp中添加对应的类型
   b)在对应的rpc中调用mailbox的registerchannel函数
*/
const WORD RCC_MSG_TYPE_CHANNEL = 0;                   /* 通用的通道维持消息 */
const WORD RCC_MSG_TYPE_MB_SPS    = 1;                   /* SPS网络内部的MB帧 */
const WORD RCC_MSG_TYPE_RAW_SP = 2;                      /* SP网络中的RAW帧 */
const WORD RCC_MSG_TYPE_RAW_BC_CRM = 3;           /* BC网络中的RAW帧 */
const WORD RCC_MSG_TYPE_RAW_BC_BIGDATA = 4;   /* BC网络中的RAW大数据帧 */
const WORD RCC_MSG_TYPE_RAW_BSS_CRM =5;               /* 中海湾服务器和企业服务器(PDA)间的RAW帧 */
const WORD RCC_MSG_TYPE_RAW_BSS_BIGDATA = 6;      /* 中海湾服务器和企业服务器(PDA)间的RAW大数据帧 */
const WORD RCC_MSG_TYPE_RAW_BSG_CRM =7;               /* 企业设备和网关间的报文 */
const WORD RCC_MSG_TYPE_MAX = 7;                                /*  增加类型之后要调整*/
static const char *struct_fmt_rcc_header = "d-w-w-w-w-w-w-w-w";

#pragma pack(4)
class CRccHeader
{
public:
    union
    {
        CHAR      chMagic[4];  /* 魔术字 "DCF\0"*/
        DWORD  dwMagic;
    };
    
    WORD wVer;                /* 通信头部的版本 */
    WORD HeaderCRC16; /* 本头部的CRC16 */ 

    WORD Msgtype;  /* 消息帧类型 1:mailbox帧  2:raw帧 3:通道管理帧 */  
    WORD DataCrc16;  /* 非CRccHeader部分数据区的校验和 (含有CRccFrame)*/    

    WORD Frame_Secret_ver;  /* Frame 的加密版本号*/
    WORD Secret_Ver ;    /* 内容层的加密版本号 */
    
    WORD wMsgLen;        /* 用户数据区的总长度,除了CRccHeader之外的长度 (加密前的)*/    
    WORD PackageLen;    /* 内存(初始化时)或打包(加密)之后的长度，除了CRccHeader之外的长度*/
};
#pragma pack()

const WORD RCC_SIZE_HEADER = sizeof(CRccHeader);       /* 通信总头长度 */
/* 
封帧层 16字节
特别注意:
1.修改了本数据结构，需要修改 RCC_SIZE_FRAME 的数字
2.CRccFrame的加密由通道层完成，采用统一的密钥，加密范围ctrl~CurFrames
*/
/* ctrl定义 高位~地位
bit7   1 可靠性帧 0非可靠帧
bit6   1 发送通道 0收通道
bit5   1 接收确认帧 CurFrames表示期望的下一帧号 wMsg1为收方的窗口大小  0未使用
bit4   1 命令帧 0响应帧
bit3   1 帧序号确认帧 0未使用
bit2   保留
bit1   1未结束     0结束
bit0   1 非第一帧 0第一帧
*/
const BYTE RCC_CTRL_VALUE_YES = 1;
const BYTE RCC_CTRL_VALUE_NO = 0;

const BYTE RCC_CTRL_BIT_SURE = 7;                // 0 是否为可靠性帧
const BYTE RCC_CTRL_BIT_CHANNEL = 6;        /* 通道类型 1:到对方的发送(命令)通道 0:到对方的接收(响应)通道，给对方应答时，要填写本端的通道类型 */
const BYTE RCC_CTRL_BIT_SURE_RECV = 5;    // 0 未使用 若bit4也为1，则表示是请求帧，而非确认帧
const BYTE RCC_CTRL_BIT_CMD = 4;                 // 0 响应(多义)
const BYTE RCC_CTRL_BIT_CON = 1;                 // 0 结束(最后1帧)
const BYTE RCC_CTRL_BIT_NEXT = 0;                // 0 第一帧
const BYTE RCC_CTRL_BIT_WANT = 4;               /* 正常来说，需要第0帧时出现 */
const BYTE RCC_CTRL_BIT_ACK_SEQ = 3;         /* 确认已经收到的帧序号，仅仅对单包有效 */

// 常用的几个常量
const BYTE RCC_CTRL_FLAG_RECV_SURE = (1<<RCC_CTRL_BIT_SURE)|(1<<RCC_CTRL_BIT_SURE_RECV);
const BYTE RCC_CTRL_FLAG_CMD_SURE = (1<<RCC_CTRL_BIT_SURE)|(1<<RCC_CTRL_BIT_CMD);
const BYTE RCC_CTRL_FLAG_CMD_NOSURE = (1<<RCC_CTRL_BIT_CMD);
const BYTE RCC_CTRL_FLAG_RSP_SURE = (1<<RCC_CTRL_BIT_SURE);
const BYTE RCC_CTRL_FLAG_RSP_NOSURE = 0;
const BYTE RCC_CTRL_FLAG_RSP_SURE_CON = (1<<RCC_CTRL_BIT_SURE)|(1<<RCC_CTRL_BIT_CON);
const BYTE RCC_CTRL_FLAG_CMD_SURE_CON = (1<<RCC_CTRL_BIT_SURE)|(1<<RCC_CTRL_BIT_CMD)|(1<<RCC_CTRL_BIT_CON);
const BYTE RCC_CTRL_FLAG_ACK = (1<<RCC_CTRL_BIT_SURE_RECV);
const BYTE RCC_CTRL_FLAG_ACTSEQ = ((1<<RCC_CTRL_BIT_SURE)|(1<<RCC_CTRL_BIT_SURE_RECV)|(1<<RCC_CTRL_BIT_ACK_SEQ));
const BYTE RCC_CTRL_FLAG_SENDER_SEQ_RANGE = 0xff;  // 且SequenceNumber = 0 wMsg1为最老序号

const WORD RCC_FRAME_REJECT = 0xffff;   /* 收方接收到错误包号的帧，则返回该帧，发送方应该删除该节点 */
static const char *struct_fmt_rcc_frame = "w-b-b-d-w-w-w-w";

#pragma pack(4)
class CRccFrame
{
public:
    WORD   Flag;                /* 解密和字节序转换之后的 */
    BYTE      ctrl;                 /* 控制字 */
    BYTE      send_times;   /* 重发次数 */
    
    DWORD cirtifyid;    /*  认证号 ，是用户登录之后授予的号(和IP地址关联的一个随机值)，客户的需要该句柄和服务端交互*/  
    
    WORD   SequenceNumber;/* 报文序号(发送方的) 一个应用层帧对应一个序号(如果不需要确认的帧，则帧序号为0) */
    /* 报文分片信息 这个版本不支持分片 8字节 后面分片需要放在通道层，方便其采用统一的滑窗机制，简化应用层*/
    WORD    CurFrames;       /* 2017-06-16  10:43:12 第一帧时这个表示总帧数，和ctrl联动*/
    
    WORD    wMsg1;/* 各种消息帧含义不一样,raw_bsc对应为业务信息类型(或者是命令字)，如果是确认帧，则表示收方窗口大小 */
    WORD    wMsg2;/* 各种消息帧含义不一样,第一帧为raw_bsc对应为响应时收消息模块id，接收方自定义，非第一帧为窗口大小 */
public:
    void ctrl_set(BYTE bit,BYTE bvalue)
    {
        if (bvalue)
        {
            ctrl |= (1<<bit); 
        }
        else
        {
            ctrl &= ~(1<<bit);
        }
    }
    BYTE ctrl_get(BYTE bit)
    {
        if (ctrl &(1<<bit))
        {
            return RCC_CTRL_VALUE_YES;
        }
        return 0;
    }
    /* 是否为确认帧 */
    bool IsSureFrame()
    {
        return ctrl_get(RCC_CTRL_BIT_SURE_RECV)?true:false;
    }
    
    /* 是否需要可靠的 */
    bool IsNeedSure()
    {
        // 确认帧不能再确认，防止死循环
        if (IsSureFrame()) return false;
        return ctrl_get(RCC_CTRL_BIT_SURE)?true:false;
    }
    
    /* 是否为命令响应帧 */
    bool IsCmdRspFrame()
    {
        return ctrl_get(RCC_CTRL_BIT_CMD)?false:true;
    }

    /* 是否为命令帧 */
    bool IsCmdFrame()
    {
        return ctrl_get(RCC_CTRL_BIT_CMD)?true:false;
    }
    
    /* 是否为第一帧 */
    bool IsFirstFrame()
    {
        return ctrl_get(RCC_CTRL_BIT_NEXT)?false:true;
    }

    bool IsSureSeq()
    {
        return (IsSureFrame() & (ctrl_get(RCC_CTRL_BIT_ACK_SEQ) > 0))?true:false;
    }
};
#pragma pack()

const WORD RCC_SIZE_FRAME = sizeof(CRccFrame);         /* 封帧头部的长度 */

// 同节点的进程内/间通信的报文格式
static const char *struct_fmt_rcc_mailbox = "d-d-w-w-b-b-b-b-d-d-d-w-w";

/* 32字节 */
#pragma pack(4)
class CMailHeader
{ 
public:
    /* 消息寻址信息 16字节 */
    DWORD dstSysID;                      // 消息目的系统id
    DWORD srcSysID;                      // 消息源的系统id    
    
    WORD   dstModid;                      // 消息目的模块ID
    WORD   srcModid;                       // 消息源模块ID
    BYTE      ctrl;                                 // 预留一个控制
    BYTE      bHost;                            // 应用报文是否为主机序(发送方在crm打包时填写，收方frame层填写,crm参考)
    BYTE      dstEntryID;                   // 消息目的入口ID
    BYTE      srcEntryID;                   // 消息源入口ID
    
    /* 消息控制部分 (优先级控制)*/
    
    /* 用户自定义信息 12字节 */
    DWORD dwMsg0;                           // 用户数据Msg[0]
    DWORD dwMsg1;                           // 用户数据Msg[1]
    DWORD dwMsg2;                           // 用户数据Msg[2]  
    /* 报文内容的寻址 4字节 */
    WORD   Flag;                                   /* 2017-06-16  9:29:12 修改为字节序标记*/
    WORD   header_offset;                  /* 后面是用户区的地址，这里指示到报文总头部的偏移(指向CMailCommCtrlHeader.Magic) */
public:
    void InitHeader(MAIL_BOX_ADDR &srcAddr,MAIL_BOX_ADDR &dstAddr,BYTE bhost);
    void GetMsgAddr(MAIL_BOX_ADDR *psrcAddr,MAIL_BOX_ADDR *pdstAddr);
    void GetUserParam(DCFQUEMSG &msg);
    bool IsHostOrder();
    void CheckFlag();
    BYTE GetFlagOrder();
};
#pragma pack()

const WORD RCC_SIZE_MB_HEAD = sizeof(CMailHeader);   /* MB的总长度 */

/*  
控制字定义:
bit7:    1:响应  0:命令
bit6:   多义 (响应)1:结束  0:未结束 (命令)1:需要应答 0:无需应答
*/
const WORD CRM_TYPE_CMD = (1<<7);
const WORD CMD_RSPFLAG_END = (1<<6);
const WORD CMD_CMDFLAG_NEEDQ = (1<<6);
const WORD CRM_CTRL_TF_MASK = CRM_TYPE_CMD|CMD_RSPFLAG_END;
/* 下面一组常用组合 */
const WORD CRM_CTRL_CMD_NOREQ = 0x00;
const WORD CRM_CTRL_CMD_NEEDREQ = CMD_CMDFLAG_NEEDQ;
const WORD CRM_CTRL_RSP_END = CRM_TYPE_CMD|CMD_RSPFLAG_END;
const WORD CRM_CTRL_RSP_CON = CRM_TYPE_CMD;

static const char *struct_fmt_crm_cmd = "w-w-w-w";
const WORD RCC_SIZE_CRM_TLV = 4*sizeof(WORD);         /* CRM TLV结构体指针 */
/* 通用的打包命令格式 */
#pragma pack(4)
struct CRM_CMD
{
    WORD     wCmdIdx;     // 命令字索引
    WORD     wCtrlCmd;    // 控制字
    union
    {
        WORD wCmd;        // 命令字
        WORD wEvent;      // 事件
    };
    WORD     wParaLen;    // 参数长度
    BYTE     *pbyPara;     // 参数指针    
    bool IsCmd(){return IsRsp()?false:true;};
    bool IsRsp()
    {
        return (wCtrlCmd&CRM_TYPE_CMD)?true:false;
    }
    bool IsRspEnd(){return ((wCtrlCmd&CRM_CTRL_TF_MASK)==CRM_CTRL_RSP_END)?true:false;};
    bool IsCmdNoReq(){return ((wCtrlCmd&CRM_CTRL_TF_MASK)==CRM_CTRL_CMD_NOREQ)?true:false;};
    bool IsCmdReq(){return ((wCtrlCmd&CRM_CTRL_TF_MASK)==CRM_CTRL_CMD_NEEDREQ)?true:false;};
};
#pragma pack()

static const char *struct_fmt_crm_header = "w-w";
/* 用于控制连续取包 */
#pragma pack(4)
struct CRM_HEAD
{
    WORD CmdNums;
    WORD Len;               /* 总长度 */
};
#pragma pack()

const WORD RCC_SIZE_CRM_HEAD = sizeof(CRM_HEAD);   /* CRM的头部长度 */

/* 常用的计算长度 */
/* RAW格式:rccheader + frameheader+自定义(tlv)*/
const WORD RCC_SIZE_RAW_HEAD = RCC_SIZE_HEADER + RCC_SIZE_FRAME;
const WORD RCC_SIZE_RAW_USER = RCC_SIZE_TOTAL - RCC_SIZE_RAW_HEAD;
const WORD RCC_SIZE_RAW_ONETLV_MIN = RCC_SIZE_RAW_HEAD+RCC_SIZE_CRM_HEAD+RCC_SIZE_CRM_TLV ;
/* mailbox通信格式下，用户报文的最大长度 */
const WORD RCC_SIZE_MB_HEAD_ALL = RCC_SIZE_HEADER + RCC_SIZE_FRAME + RCC_SIZE_MB_HEAD;
const WORD RCC_SIZE_MB_USER = RCC_SIZE_TOTAL - RCC_SIZE_MB_HEAD_ALL;
const WORD RCC_SIZE_MB_MSGLEN_HEAD = RCC_SIZE_FRAME + RCC_SIZE_MB_HEAD;
const WORD CRM_SIZE_ONETLV_MIN = RCC_SIZE_CRM_HEAD+RCC_SIZE_CRM_TLV ;

/* 通信底层读报文的错误码 */
const DWORD RCC_SUCCESS = 0;
const DWORD RCC_ERR_FAILED = (DWORD)-1;
const DWORD RCC_ERR_SOCKET  = 1;    /* SOCKET错误 */
const DWORD RCC_ERR_TIMEOUT = 2;
const DWORD RCC_ERR_EMPTY      = 3;
const DWORD RCC_ERR_BUFFER_LEN = 4;

/* 大数据传输 只有在第一帧才有这个头部*/
static const char *struct_fmt_bdt_header = "w-w-w-w-d";
#pragma pack(4)
struct BDT_TLV
{
    WORD wCmdIdx;
    WORD wCtrl;
    WORD wCmd;
    WORD wParaExt;   /* 记录条数等，具体依赖命令 */
    DWORD DataLen;
    BYTE *pPara;
};
#pragma pack()

const WORD RCC_SIZE_BIG_HEAD = sizeof(BDT_TLV) - sizeof(BYTE*);

static const char *struct_fmt_gzip_header = "d-d";
#pragma pack(4)
struct GZIP_INFO
{
    DWORD org_len;   /* 压缩前长度，0表示未知，多用于文件流方式 */
    DWORD zlib_len;   /* 压缩之后长度 */
};
#pragma pack()

const WORD RCC_SIZE_GZIP_HEAD = sizeof(GZIP_INFO);

#endif
