
#ifndef _DCF_DTFRAME_H
#define _DCF_DTFRAME_H
/****************************************************************
*文件范围 : 本类用于封装帧拆包和组包以及滑窗重传机制
*设计说明 : 滑窗参考设计
                   http://blog.csdn.net/chenchaofuck1/article/details/51995590
                   http://www.cnblogs.com/woaiyy/p/3554182.html
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  11:40:40
****************************************************************/

#include "dcf_i_rcc_dth.h"
#include "dcf_bits.h"

/* 发送帧结构 */
struct FrameSend
{
    // 数据信息
    BYTE     *pSendMsg;
    DWORD  MsgLen;
    /* 帧序号对应 */
    WORD   totalFrame;         /* 总帧数 */
    WORD   SequenceNumber;  /* 数据帧序号 */

    /* 确认信息对应 */
    BYTE      msgSurebit[8];   /* 滑窗机制,最大支持4*8bit=64帧 */
    /* 控制部分 */
    BYTE      awnd_idx;            /* 滑动窗口大小索引  */
    BYTE      ssthresh;              /* 门限 根据swnd值进行变化*/
    BYTE      frame_state;        /* 报文状态 0:初始化 1:慢起步 2:拥塞控制 3:快速重传 */
    BYTE      awnd_rtt;            /*  当前窗口等待确认完计时 定时器每次++，当awnd_rtt>g_cwnd_rtt_table[awnd_idx]时就判定为确认超时*/

    /* 首帧信息 */
    WORD   Frame_curBegin;  /* 当前未确认的首帧号 */
    BYTE      head_act_rtts;   /* 当前帧确认超时计时,第一次发送时初始为对应awnd表的时间 */
    BYTE      ack_times;           /* 收方连续发送ack_seq的次数，如果次数大于3，则滑窗要调小*/

    /* 窗口大小 */
    WORD   swnd;                   /* 自己的wnd_size =min{rwnd_peer,cwnd}*/
    WORD   rwnd;                   /*对方的wnd_size ，在确认帧中对应CRccFrame.wMsg2 */
    WORD   cwnd;                   /* 拥塞窗口 随流控进行调整大小*/
    WORD   curBegin_stimes;   /* 发送首帧的次数 ,发送首帧超过20次，则需要强制移动(可能是收方问题) */

    /* 当前发送控制 */
    WORD   frame_send_wait;  /* 当前待发帧号位置(curBeginFrame+send_winPt即为帧号) 此前均已发送*/
    BYTE      send_times_left;    /* 消息报文剩余的总次数，每次收到确认都置 RETRY_TOTAL_TIMES ，没隔SEND_DELAY_TIMES发送1次*/
    BYTE      send_con_faild;     /* 连续发送时失败了，下次定时器到达，则需要直接发送 */
    WORD   send_frames_ok;  /* 在一个RRT中一次成功的帧数量 */
    WORD   send_frames_retry;  /* 在一个RRT中重发的帧数量 */

    /* 消息参数 */
    WORD   wMsg1;                   /* 通常用来描述信息类型 RCC_DATA_TYPE_RAW_BSC*/
    WORD   wMsg2;
    WORD   Msg_Secret_Ver;  /* rcc_head.secret_ver */
    WORD   wMsgType;            /* rcc_head.MsgType */
    BYTE      msg_ctrl;
    BYTE      bHost;
};

// 最大窗口数量 msgSurebit 的bit数量
#define SWD_BYTES 8
#define MAX_WINDOW_SIZE (SWD_BYTES*8)
#define SWND_SSTHRESH_INIT 16   // 控制进入拥塞的阈值

#define MAX_MSG_SEQ_NUM 128   /* 最多缓存的包数量 */
#define FRAME_NUMS_PERONE 8   /* 滑窗每次发送的包数量 */
#define RETRY_TOTAL_TIMES 60   /* 报文发送总时间 */
#define SEND_DELAY_TIMES 3        /* 再次发送的延迟秒数 */
#define WND_ACKTIMES_MAX 3    /* 评估一个窗口质量好坏的标准:收方请求的次数大于该值，认为质量差，否则认为好 */
#define ACK_RETRY_TIMES 3         /* 请求重传的消息，重发的时间控制 */
const WORD ONE_FRAME_LEN = RCC_SIZE_RAW_USER;      // 1012-36 = 976

// 报文发送池
class CMsgSender
{
public:
    CMsgSender(COMM_CHANNEL &Info);
    ~CMsgSender();
    DWORD SendMsg(MSG_RPC &msg);
    void ProcRecvMsg(BYTE *pMsg,WORD msgLen,BYTE bHost);
    /* 1s定时器 */
    void ProcTimer();
    void ClearAll();
    void ChangePort(WORD wPort);
protected:
    WORD GetSequence(WORD Pt);
    FrameSend *GetNodePtr(WORD Pt);
    FrameSend *CopyNode(MSG_RPC &msg);
    void FreeNode(FrameSend *&pNode);
    WORD FindSequence(WORD Seq,BYTE &bReson);
    void AdjustHeadPtr();
    void AdjustTailPtr();
    DWORD SendFrame(FrameSend *pNode,WORD wFrameIdx);
    bool IsValidFrame(FrameSend *pNode,WORD wFrameIdx);
    void RemoveNode(WORD Pt);
    void MakeSureToFrame(FrameSend *pNode,WORD wFrameIdx);
    void AskRetryHead(FrameSend *pNode);
    void SendSeqRange();
protected:
    void NodeTimer_Main(FrameSend *pNode,WORD Pt);
    void NodeTimer_Jibie(FrameSend *pNode);
    void NodeTimer_Jibie_Good(FrameSend *pNode);
    void NodeTimer_Jibie_Bad(FrameSend *pNode);
protected:
    WORD check_send_window(FrameSend *pNode);
    void AckedFrame(FrameSend *pNode);
    void Proc_RecvAckedSeq(WORD seq);
    void FinishNode(WORD Pt);
    WORD Full_Search(WORD seq,FrameSend *&pNode);
    void NotifyData(FrameSend* pNode, BYTE bhost); //add by hhw 2023.6.29 用于pda数据发送完成后自己应答自己
protected:
    // 待确认指针列表
    FrameSend *m_sequence_list[MAX_MSG_SEQ_NUM];   /* 未确认完成列表，是一个循环数组，按照序号排序了的 */
    WORD m_beginPt;
    WORD m_endPt;
    WORD m_SequenceNumber;
    WORD m_NodeNums;
    WORD m_total_times;

    COMM_CHANNEL m_channel;
};

/**********************************************************************************************************
***********************************************************************************************************
*********************************接收组包部分***************************************************************
**********************因为收帧相对简单，数据也少，为了避免混杂，将收发的分开*********************************
***********************************************************************************************************
***********************************************************************************************************/

struct FrameRecv
{
    BYTE     *pRecvMsg;
    /* 帧序号对应 */
    WORD   totalFrame;         /* 总帧数 */
    WORD   PacketLen;          /* 每包数据长度 */
    WORD   LastFrameLen;   /* 最后一包的长度 */
    WORD   SequenceNumber;  /* 数据帧序号 */

    /* 确认信息对应 */
    WORD   Frame_curBegin;  /* 当前未确认的首帧号 */
    BYTE      frame_errors;        /* 当出现乱序帧时置该标记 */
    BYTE      frame_repeat;       /* 重复帧 */
    BYTE      frame_recvs;         /* 收到正确报文数 */
    /* 参数信息 */
    BYTE      msg_ctrl;

    /* 接收滑窗 */
    BYTE      msgSurebit[8];   /* 滑窗机制,最大支持4*8bit=64帧 */

    WORD   wMsg1;
    WORD   wMsg2;
    WORD   Secret_Ver;
};

#define SEQ_WND_SIZE 1024 /* 最多支持1024个未确认完成的帧 */
const WORD SEQ_WND_SIZE_BYTES = (SEQ_WND_SIZE + 7)/8;
const WORD SEQ_MID = ((WORD)-1)/2;
const WORD WND_TOTAL_MOVE_THRESHOLD = (SEQ_WND_SIZE/4);  // 256
const WORD WND_AGEING_TIMEOUT = 60*1;                               // 序号1分钟老化表，达到1分钟，则总窗口会被强制移动
const WORD WND_TOTAL_MOVE_SIZE_BIT = 64;                        /* 强制移动的bit数目(配置为8的倍数，移动效率高) */
/* 报文接收方 */
class CMsgRecver
{
public:
    CMsgRecver(COMM_CHANNEL &Info);
    ~CMsgRecver();
    void ProcRecvMsg(BYTE *pMsg,WORD msgLen,BYTE bHost);
    void ProcTimer();
    void ClearAll();
    void ChangePort(WORD wPort);
protected:
    WORD GetSequence(WORD Pt);
    FrameRecv *GetNodePtr(WORD Pt);
    FrameRecv *CopyNode(BYTE *pMsg,WORD MsgLen);
    void FreeNode(FrameRecv *&pNode);
    WORD FindMultSequence(WORD Seq,BYTE &bReson);
    void AdjustHeadPtr();
    void AdjustTailPtr();
protected:
    void ProcOnceFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost);
    void ProcNewMultiFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost);
    void ProcNextFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost);
    void SendFrameAck(BYTE *pMsg,bool bReject = false);
    int GetSeqDistance(WORD seq);
    DWORD AddIntoMultiQue(BYTE *pMsg,WORD msgLen);
    void AskSenderFrame(WORD seq,WORD ask_frame);
protected:
    // 总确认表的维护
    void Timer_Total_Maintain();
    void Timer_Total_Ageing();
    void Timer_Mult_ProcNode(FrameRecv *pNode);
    bool precheck_by_total(WORD seq,WND_CMP &cwr);
protected:
    void Notify_AckSeq(WORD seq = 0);
    void Multi_Add_Frame(WORD Pt,BYTE *pMsg,WORD MsgLen,BYTE bHost);
    DWORD Multi_Save_Frame(FrameRecv *pNode,BYTE *pMsg);
    void Multi_Frame_Ack(FrameRecv *pNode);
    void SureSenderSeqWnd(BYTE *pMsg);
protected:
    void FinishNode(WORD Pt);
    void NotifyMultiData(FrameRecv *pSeqNode,WORD msgType,BYTE bhost);
    void NotifyOnceData(BYTE *pMsg,WORD msgLen, BYTE bhost);
    void TryMoveTotalWnd(WORD curSeq = 0);
protected:
    // 待确认指针列表
    /*
    多包确认表和总表的关系
    1.总确认表的起始序列号大于等于多包确认表
    2.移动m_seq_sure_total有3个时机
       a)多表的开始序号大于m_seq_sure_total_begin
       b)m_bNeedMoveWndSeq为1，而该值为1，则表示窗口已经满，需要强制移除了
       c)m_seq_sure_timeout达到门限值，而且确认总表只剩余(WND_TOTAL_MOVE_THRESHOLD 256)了
    */
    FrameRecv *m_sequence_multi_list[MAX_MSG_SEQ_NUM];   /* 未确认完成列表，是一个循环数组，按照序号排序了的,进这里面的一定是多包的*/
    CFSWndCtrl  *m_seq_sure_total;               /* 总确认表 */
    CFsAgeingTable *m_seq_ageing_tools;      /* 序号老化表,要老化表的目的:如果增加一个变量，只能描述当前第一帧的开始时间，无法描述后面已经收到的时间 */
    BYTE      m_bNeedNotifySeq;                /* 需要通知需要确认的seq */
    BYTE      m_bNeedMoveWndSeq;         /* 需要清理seq_wnd */
    WORD   m_mult_beginPt;
    WORD   m_mult_endPt;

    // 通道参数
    COMM_CHANNEL m_channel;
};

#endif
