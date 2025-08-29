/****************************************************************
*文件范围 : 滑窗收发报文
*设计说明 : 参考说明:https://my.oschina.net/xinxingegeya/blog/485650
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  13:35:4
****************************************************************/
#include "dcf_dtframe.h"
#include "extern_api.h"
#include "dcf_err.h"
#include "dcf_rcc_dthelper.h"
#include "dcf_time.h"

/* 增加详细的收发报文打印信息 */
#define DBP_DTFRAME 0
#define DBP_DTFRAME_NO 0    /* 完全没有打印 */
#define DBT_SEQ_BEGIN 0  /* 开始帧号 */
enum FIND_RESON
{
    FR_OK = 0,
    FR_NEAR = 1,
    FR_OLD = 2,
    FR_NEW = 3
};
/* 概念
rwnd:recive window 接收窗口
cwnd:congestion window 拥塞窗口
swnd:send window 发送窗口 = Min [rwnd, cwnd]
mss:最大报文段 http://blog.chinaunix.net/uid-20788636-id-2626119.html?/11207.html 协商双方的tcp mss都是1460
RTT:Round Trip Time，网络往返时间
ssthresh:慢启动门限.当cwnd>ssthresh，需要进入避免拥塞的过程,进入慢启动控制过程
              当cwnd<ssthresh,使用拥塞控制算法，停用慢启动算法
              该阀值变化:
              1.1 当网络只要出现没有应答，则ssthresh=cwnd/2同时设置cwnd为原来的1/3,进入慢启动过程
RTO:重传定时器,当RTO超时，而且没有ACK时，则可能是网络丢包等，出现该情况之后，将进行如上1.1调整
*/
/* wnd_size调整表 */
/*
慢启动:
1.开始启动时窗口大小为g_cwnd_size_table_rtt[0]
2.经过一次RTT，如果收方请求的报文次数低于重传门限，则可以升一次窗口

拥塞避免:
1.当cwnd>ssthresh后，进入拥塞避免
2.进入拥塞避免之后，cwnd进入加法增大的阶段，则不是此前慢启动的指数增长阶段
3.当对应时间内所有报文均被确认之后，cwnd值+1
4.出现RTO内超时，则进入乘法减小的阶段
  乘法减小：无论在慢启动阶段还是在拥塞控制阶段，只要网络出现超时，就是将cwnd置为1，ssthresh置为cwnd的一半，然后开始执行慢启动算法

快速重传:
1.收方收到乱序包时，立即发送ACK.收方收到3个相同ACK时，则立即进入快速重传机制
2.快速重传做的事情:
   a)把ssthresh设置为cwnd的一半
   b)把cwnd再设置为ssthresh的值(具体实现有些为ssthresh+3)
   c)重新进入拥塞避免阶段

快速恢复:
1.当收到3个重复ACK时，把ssthresh设置为cwnd的一半，把cwnd设置为ssthresh的值加3，然后重传丢失的报文段，加3的原因是因为收到3个重复的ACK
2.再收到重复的ACK时，拥塞窗口增加1
3.收到一个新的ACK后，退出快速恢复阶段，提高吞吐率
*/
#define TCP_WND_ADJUST_TIMES 3   /* 请求重传的次数门限(即评估本窗口是否合适 次数小于该值，则可以调大，大于则调小 等于则可以不变) */
// 滑窗大小表,滑窗调整表
BYTE g_swnd_size_table_rtt[] = {2,4,16,32};
// 计时时间表,对应不同的滑窗计数器时间不一样(统计的目的是升滑窗值还是降低滑窗值)
BYTE g_swnd_rtt_table[] = {2,2,3,4};
const BYTE wnd_table_size = sizeof(g_swnd_size_table_rtt);

enum FRAME_STATE
{
    FS_SLAVE_START = 0,  /* 该状态时，wnd按照g_cwnd_size_table_rtt表调整 */
    FS_FLOW_CTRL = 1,     /*  该状态时，wnd每次只能+1*/
    FS_QUICK_RESTORE = 2   /* 该状态时，ssthresh变为cwnd一半 */
};

/****************************************************************
*功能描述 : 构造函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  14:17:33
****************************************************************/
#pragma warning(disable:4311 4302)
CMsgSender::CMsgSender(COMM_CHANNEL &Info)
{
    memset(m_sequence_list,0,sizeof(m_sequence_list));
    m_beginPt = 0;
    m_endPt = 0;

    m_SequenceNumber = DBT_SEQ_BEGIN;
    memcpy(&m_channel,&Info,sizeof(m_channel));
            ASSERT(m_channel.Sender != NULL);
    m_NodeNums = 0;
    m_total_times = 0;
}

CMsgSender::~CMsgSender()
{
    ClearAll();
}
/****************************************************************
*功能描述 : 清除所有待确认节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  14:16:52
****************************************************************/
void CMsgSender::ClearAll()
{
    m_endPt = m_endPt %MAX_MSG_SEQ_NUM;
    for(;;)
    {
        if (m_sequence_list[m_beginPt])
        {
            FreeNode(m_sequence_list[m_beginPt]);
            m_sequence_list[m_beginPt] = NULL;
        }

        if (m_beginPt == m_endPt)
        {
            break;
        }

        m_beginPt = (m_beginPt + 1)%MAX_MSG_SEQ_NUM;
    }

    m_NodeNums = 0;
    m_beginPt = 0;
    m_endPt = 0;
}

/****************************************************************
*功能描述 : 切换端口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-14  11:3:31
****************************************************************/
void CMsgSender::ChangePort(WORD wPort)
{
    m_channel.dstPort = wPort;
}


/****************************************************************
*功能描述 : 获取指定节点的报文序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  13:44:10
****************************************************************/
WORD CMsgSender::GetSequence(WORD Pt)
{
    if ((Pt < MAX_MSG_SEQ_NUM) && m_sequence_list[Pt])
    {
        return m_sequence_list[Pt]->SequenceNumber;
    }
    return 0;
}

/****************************************************************
*功能描述 : 获取指定节点的对象
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  13:45:46
****************************************************************/
FrameSend *CMsgSender::GetNodePtr(WORD Pt)
{
    if (Pt < MAX_MSG_SEQ_NUM)
    {
        return m_sequence_list[Pt];
    }
    return NULL;
}
/****************************************************************
*功能描述 : 快速查找对应的序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  13:48:37
1修改描述 : 修改为可以查找相近节点
*修改作者 : zjb
*修改时间 : 2017-06-19  17:14:16
1修改描述 : 这个算法还有点问题，先搞一个全查找，增加定位信息，后面再优化
*修改作者 : zjb
*修改时间 : 2017-09-05  11:21:47
****************************************************************/
WORD CMsgSender::FindSequence(WORD Seq,BYTE &bReson)
{
    WORD curPt = m_beginPt;
    WORD wBeginSeq = GetSequence(m_beginPt);
    WORD wEndSeq = GetSequence(m_endPt);
    if ((!wBeginSeq) || (!wEndSeq))
    {
        // 头或者位为空
        bReson = FR_NEW;
        return MAX_MSG_SEQ_NUM;
    }

    if (Seq == wBeginSeq)
    {
        bReson = FR_OK;
        return m_beginPt;
    }

    if (Seq == wEndSeq)
    {
        bReson = FR_OK;
        return m_endPt;
    }

    if (wEndSeq >= wBeginSeq)
    {
        // 还没有绕接的场景
        if (Seq < wBeginSeq)
        {
            // 已经无效
            bReson = FR_OLD;
            return MAX_MSG_SEQ_NUM;
        }

        if (Seq > wEndSeq)
        {
            // 新节点
            bReson = FR_NEW;
            return MAX_MSG_SEQ_NUM;
        }

        // 找中间的
        /* 下面这段算法可以进行优化 有空弄*/
        for(;curPt != m_endPt;curPt = (curPt+1)%MAX_MSG_SEQ_NUM)
        {
            if (!m_sequence_list[curPt])
            {
                continue;
            }

            if (m_sequence_list[curPt]->SequenceNumber == Seq)
            {
                bReson = FR_OK;
                return curPt;
            }

            if (m_sequence_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }
        }

        dcf_output("%s exception!",m_channel.print_ownapp);
        bReson = FR_OLD;
        return MAX_MSG_SEQ_NUM;
    }

    // 反转后的找法
    // 先把下面算法无法甄别的搞定，根据距离判断
    if ((Seq < wBeginSeq) && (Seq > wEndSeq))
    {
        if ((wBeginSeq - Seq) > (Seq - wEndSeq))
        {
            // 更近尾巴一点，判定为新节点
            bReson = FR_NEW;
        }
        else
        {
            // 近头部一点,判断为过时的节点
            bReson = FR_OLD;
        }
        return MAX_MSG_SEQ_NUM;
    }

    // 1.划分区域
    BYTE bLeft = 0;
    if (Seq >= wBeginSeq)
    {
        bLeft = 1;
    }

    for(;curPt != m_endPt;curPt = (curPt + 1)%MAX_MSG_SEQ_NUM)
    {
        if (!m_sequence_list[curPt])
        {
            continue;
        }

        if (m_sequence_list[curPt]->SequenceNumber == Seq)
        {
            bReson = FR_OK;
            return curPt;
        }

        // 在区域中查找
        if (bLeft)
        {
            if (m_sequence_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }

            if (m_sequence_list[curPt]->SequenceNumber < wBeginSeq)
            {
                // 到了开始反转的地方了，往后不可能再有
                bReson = FR_NEAR;
                return curPt;
            }
        }
        else
        {
            if (m_sequence_list[curPt]->SequenceNumber > wBeginSeq)
            {
                // 先要找到临界点
                continue;
            }

            // 在右边区域内找
            if (m_sequence_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }
        }
    }

    // 走到这里应该是end节点为空,判定为尾巴的
    bReson = FR_NEW;
    return MAX_MSG_SEQ_NUM;
}

/****************************************************************
*功能描述 : 全查找
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-09-05  11:11:2
****************************************************************/
WORD CMsgSender::Full_Search(WORD Seq,FrameSend *&pNode)
{
    WORD curPt = m_beginPt;
    WORD wBeginSeq = GetSequence(m_beginPt);
    WORD wEndSeq = GetSequence(m_endPt);
    pNode = NULL;
    if ((!wBeginSeq) || (!wEndSeq))
    {
        // 头或者位为空
        return MAX_MSG_SEQ_NUM;
    }

    if (Seq == wBeginSeq)
    {
        pNode = m_sequence_list[m_beginPt];
        return m_beginPt;
    }

    if (Seq == wEndSeq)
    {
        pNode = m_sequence_list[m_endPt];
        return m_endPt;
    }

    /* 全缓存找 */
    for(;;)
    {
        curPt = (curPt+1)%MAX_MSG_SEQ_NUM;
        if ((m_sequence_list[curPt]) && (m_sequence_list[curPt]->SequenceNumber == Seq))
        {
            pNode = m_sequence_list[curPt];
            return curPt;
        }

        if (curPt == m_endPt)
        {
            break;
        }
    }

    return MAX_MSG_SEQ_NUM;
}


/****************************************************************
*功能描述 : 调整首指针
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  14:3:7
****************************************************************/
void CMsgSender::AdjustHeadPtr()
{
    m_endPt = m_endPt %MAX_MSG_SEQ_NUM;
    for(;m_beginPt != m_endPt;)
    {
        if (m_sequence_list[m_beginPt])
        {
            break;
        }
        m_beginPt = (m_beginPt + 1)%MAX_MSG_SEQ_NUM;
    }
}

/****************************************************************
*功能描述 : 调整尾指针
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  14:3:26
****************************************************************/
void CMsgSender::AdjustTailPtr()
{
    m_beginPt = m_beginPt %MAX_MSG_SEQ_NUM;
    for(;m_endPt != m_beginPt;)
    {
        if (m_sequence_list[m_endPt])
        {
            break;
        }

        if (!m_endPt)
        {
            m_endPt = MAX_MSG_SEQ_NUM - 1;
        }
        else
        {
            m_endPt--;
        }
    }
}

/****************************************************************
*功能描述 : 发送消息
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  14:19:39
****************************************************************/
DWORD CMsgSender::SendMsg(MSG_RPC &msg)
{
    WORD newPt = (m_endPt + 1) %MAX_MSG_SEQ_NUM;
    if (newPt == m_beginPt)
    {
        // 满了，不让发送
        dcf_output("[%s]%s sender que full(%d)...\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,m_NodeNums);
        SendSeqRange();
        return DCF_ERR_FULL;
    }

    FrameSend *pNode = m_sequence_list[newPt];
    /* 此时该节点一定是空的 */
    /* 2017-09-05  11:36:36 如果前面查找失败，在这里可能就是非空的 */
    if (pNode != NULL)
    {
        m_sequence_list[newPt] = NULL;
        dcf_sys_checkerr_fmt(DCF_ERR_REPEAT,LEVEL_COMM_ERROR,pNode->SequenceNumber,"Pt:%u not empty!(bPt:%u,ePt:%u)",newPt,m_beginPt,m_endPt);
        FreeNode(pNode);
        if (m_NodeNums) m_NodeNums--;
    }

    m_NodeNums++;
    pNode = CopyNode(msg);
    m_sequence_list[newPt] = pNode;
    // 调整endpt位置
    m_endPt = newPt;
    // 调整头部位置(第1次0位置不会被使用)
    AdjustHeadPtr();

    // 分配帧序号，0是用于无需确认的帧序号
    m_SequenceNumber++;
    if (!m_SequenceNumber)
    {
        m_SequenceNumber++;
    }
    pNode->SequenceNumber = m_SequenceNumber;
#if (!DBP_DTFRAME_NO)
    if (pNode->totalFrame > 1)
    {
        dcf_output("[%s]%s sender start to send a multi packet(tf:%d,seq:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->totalFrame,pNode->SequenceNumber);
    }
#elif DBP_DTFRAME
    dcf_output("[%s]%s sender start to send a packet(tf:%d,seq:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->totalFrame,pNode->SequenceNumber);
#endif

    // 发送滑窗大小
    check_send_window(pNode);
    return DCF_SUCCESS;
}
/****************************************************************
*功能描述 : 分配消息节点、初始化等
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  21:23:18
****************************************************************/
FrameSend *CMsgSender::CopyNode(MSG_RPC &msg)
{
    // 分配内存拷贝原数据
    FrameSend *pNode = (FrameSend*)dcf_mem_malloc(sizeof(FrameSend));
    memset(pNode,0,sizeof(FrameSend));
    // 总帧数
    DWORD dwMallocLen = (DWORD)msg.MsgLen + ONE_FRAME_LEN -1;
    pNode->totalFrame = (WORD)(dwMallocLen /ONE_FRAME_LEN);
    pNode->pSendMsg = (BYTE*)dcf_mem_malloc(msg.MsgLen);
    memset(pNode->pSendMsg,0,msg.MsgLen);
    memcpy(pNode->pSendMsg,msg.pMsg,msg.MsgLen);
    pNode->wMsg1 = msg.wMsg1;
    pNode->wMsg2 = msg.wMsg2;
    pNode->msg_ctrl = msg.ctrl;
    pNode->wMsgType = msg.wMsgType;
    pNode->MsgLen = msg.MsgLen;
    pNode->Msg_Secret_Ver = msg.Msg_Secret_Ver;
    pNode->bHost =  msg.bhost;

    // 发送控制初始化
    pNode->ssthresh = SWND_SSTHRESH_INIT;
    pNode->awnd_idx = 0;
    pNode->swnd = g_swnd_size_table_rtt[pNode->awnd_idx];
    pNode->rwnd = pNode->swnd;
    pNode->cwnd= pNode->swnd;
    return pNode;
}

void CMsgSender::FreeNode(FrameSend *&pNode)
{
    if (!pNode)
    {
        return;
    }

    if (pNode->pSendMsg)
    {
        dcf_mem_free((void*&)pNode->pSendMsg);
        pNode->pSendMsg = NULL;
    }

    dcf_mem_free((void*&)pNode);
}

/****************************************************************
*功能描述 : 发送节点对应的指定帧报文,在本类中只负责填写frame相关的数据结构
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  15:36:59
****************************************************************/
DWORD CMsgSender::SendFrame(FrameSend *pNode,WORD wFrameIdx)
{
    if ((wFrameIdx + 1) > pNode->totalFrame)
    {
                ASSERT(0);
        return DCF_ERR_PARAM;
    }

    BYTE msgBuffer[RCC_SIZE_TOTAL]={0};
    // 每次先把头部格式化
    // memset(msgBuffer,0,RCC_SIZE_RAW_HEAD);
    CRawMsgHelper::InitMsgBuffer(msgBuffer,pNode->msg_ctrl,RCC_SIZE_TOTAL,pNode->wMsg1,m_channel.cirtifyid);
    // 下面是组包
    CRccHeader *pHeader = (CRccHeader *)msgBuffer;
    pHeader->Secret_Ver = m_channel.Secret_Ver;
    pHeader->Msgtype = pNode->wMsgType;

    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(msgBuffer);
    if (wFrameIdx == pNode->Frame_curBegin)
    {
        pNode->curBegin_stimes++;
    }

    pFrame->send_times = (BYTE)pNode->curBegin_stimes;
    pFrame->wMsg2 = pNode->wMsg2;
    pFrame->SequenceNumber = pNode->SequenceNumber;

    // 这里都是需要确认的报文
    pFrame->ctrl_set(RCC_CTRL_BIT_CHANNEL,1);
    pFrame->ctrl_set(RCC_CTRL_BIT_SURE,1);
    if (!wFrameIdx)
    {
        pFrame->CurFrames = pNode->totalFrame;
        pFrame->ctrl_set(RCC_CTRL_BIT_NEXT,0);
    }
    else
    {
        pFrame->CurFrames = wFrameIdx;
        /* 2017-06-27  23:46:4 非第一帧调整为发送窗口*/
        pFrame->wMsg2 = pNode->swnd;
        pFrame->ctrl_set(RCC_CTRL_BIT_NEXT,1);
    }

    WORD wSendLen = ONE_FRAME_LEN;
    if ((wFrameIdx + 1) == pNode->totalFrame)
    {
        // 最后一帧
                ASSERT(pNode->MsgLen > 0);
        wSendLen = (WORD)(pNode->MsgLen - (wFrameIdx*ONE_FRAME_LEN));
        pFrame->ctrl_set(RCC_CTRL_BIT_CON,0);
    }
    else
    {
        pFrame->ctrl_set(RCC_CTRL_BIT_CON,1);
    }

    pHeader->wMsgLen = wSendLen + RCC_SIZE_FRAME;
    pHeader->PackageLen = pHeader->wMsgLen; /* 不加密时二者数据长度一致 */
    // 拷贝消息包
    memcpy(msgBuffer+RCC_SIZE_RAW_HEAD,pNode->pSendMsg+(wFrameIdx*ONE_FRAME_LEN),wSendLen);
    wSendLen = pHeader->wMsgLen + RCC_SIZE_HEADER;

    DWORD dwRet = m_channel.Sender(msgBuffer, wSendLen,m_channel.dstIP,m_channel.dstPort,pNode->wMsgType,1,m_channel.pThis);
    if (!dwRet)
    {
        dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"sender");
        if (pNode->frame_send_wait <= wFrameIdx)
        {
            pNode->frame_send_wait = wFrameIdx + 1;
        }
    }

#if DBP_DTFRAME
    dcf_output("[%s]%s sender send a frame(seq:%d,cf:%d,tf:%d,sf:%d,dlen:%d,IP:0x%x,Port:%u,verify:0x%x)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                                 pNode->SequenceNumber,wFrameIdx,pNode->totalFrame,pNode->frame_send_wait,wSendLen,m_channel.dstIP,m_channel.dstPort,m_channel.cirtifyid);
#endif

    // 里面进行了字节序转换发送完成之后清除，避免每次删除整个报文
    memset(msgBuffer,0, wSendLen);
    return dwRet;
}
/****************************************************************
*功能描述 : 发送自己的seq范围
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-13  21:22:55
****************************************************************/
void CMsgSender::SendSeqRange()
{
    BYTE msgBuffer[RCC_SIZE_TOTAL]={0};
    CRawMsgHelper::InitMsgBuffer(msgBuffer,RCC_CTRL_FLAG_SENDER_SEQ_RANGE,RCC_SIZE_TOTAL,0,m_channel.cirtifyid);
    // 下面是组包
    CRccHeader *pHeader = (CRccHeader *)msgBuffer;
    pHeader->Secret_Ver = m_channel.Secret_Ver;
    pHeader->Msgtype = RCC_MSG_TYPE_CHANNEL;

    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(msgBuffer);
    pFrame->ctrl_set(RCC_CTRL_BIT_SURE,1);    // 统一带上确认标记
    pFrame->send_times = 0;
    pFrame->wMsg1 = GetSequence(m_beginPt);
    pFrame->wMsg2 = m_SequenceNumber;
    pFrame->SequenceNumber = 0;
    WORD wSendLen = 0;
    pHeader->wMsgLen = wSendLen + RCC_SIZE_FRAME;
    pHeader->PackageLen = pHeader->wMsgLen;
    wSendLen = pHeader->wMsgLen + RCC_SIZE_HEADER;

    DWORD dwRet = m_channel.Sender(msgBuffer, wSendLen,m_channel.dstIP,m_channel.dstPort,RCC_MSG_TYPE_CHANNEL,1,m_channel.pThis);
    dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"sender");
#if DBP_DTFRAME
    dcf_output("[%s]%s sender send a seq range(begin:%d,end:%d,IP:0x%x,Port:%u)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->wMsg1,m_SequenceNumber,m_channel.dstIP,m_channel.dstPort);
#endif
    // 里面进行了字节序转换发送完成之后清除，避免每次删除整个报文
    memset(msgBuffer,0, wSendLen);
    return;
}

/****************************************************************
*功能描述 : 确认是否为合法的帧号。合法的条件:
                   1.在当前window中
                   2.没有被确认过
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  23:56:57
****************************************************************/
bool CMsgSender::IsValidFrame(FrameSend *pNode,WORD wFrameIdx)
{
    // 看一下当前帧序号的有效性
    if (wFrameIdx < pNode->Frame_curBegin)
    {
        // 过时了的确认帧
        return false;
    }

    if (wFrameIdx >= pNode->frame_send_wait)
    {
        // 乱搞的帧,都还没有发送
        return false;
    }

    return true;
}
/****************************************************************
*功能描述 : 删除节点，调整数组
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  0:13:3
****************************************************************/
void CMsgSender::FinishNode(WORD Pt)
{
            ASSERT(Pt < MAX_MSG_SEQ_NUM);
            ASSERT(m_sequence_list[Pt] != NULL);
    /* 先把本节点资源释放掉 */
    FrameSend *pDelNode = m_sequence_list[Pt];
    m_sequence_list[Pt] = NULL;
    FreeNode(pDelNode);

    // 将后面的往前面移动
    while(Pt != m_endPt)
    {
        WORD nextPt = (Pt + 1)%MAX_MSG_SEQ_NUM;
        m_sequence_list[Pt] = m_sequence_list[nextPt];
        Pt = nextPt;
    }

    m_sequence_list[Pt] = NULL;
    if (m_endPt != m_beginPt)
    {
        // 当前至少不只一个节点
        if (m_endPt)
            m_endPt--;
        else
            m_endPt = MAX_MSG_SEQ_NUM - 1;
    }

    m_NodeNums = m_NodeNums?(m_NodeNums -1):0;
}

/****************************************************************
*功能描述 : 确认报文
                   收方确认当前帧，意味着需要下一帧。但对于需要第0帧的情况，则需要bit5协助
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  16:52:28
****************************************************************/
void CMsgSender::ProcRecvMsg(BYTE *pMsg,WORD msgLen,BYTE bHost)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    if (pFrame->cirtifyid != m_channel.cirtifyid)
    {
        // 随机码不一样
        dcf_output("%s sender unespect cirtifyid!(0x%x,0x%x)\r\n",m_channel.print_ownapp,pFrame->cirtifyid,m_channel.cirtifyid);
        return;
    }

    // 先找到节点
    FrameSend *pNode = NULL;
    if (pFrame->IsSureSeq())
    {
        // 确认序号签收的帧
        /* 2017-07-16  7:5:20 处理收方不能收下包的情况*/
        if (pFrame->CurFrames == RCC_FRAME_REJECT)
        {
            WORD Pt = Full_Search(pFrame->SequenceNumber, pNode);
            // 对方无法收下
#if DBP_DTFRAME
            dcf_output("[%s]%s sender recv can not recv the seq,reject the seq!(seq:%d,cf:%d)\r\n",
                                 dcf_get_time_string_unsafe(),m_channel.print_ownapp, pFrame->SequenceNumber,pFrame->CurFrames);
#else
            dcf_output("{x}");
#endif
            // 释放本节点
            if (Pt < MAX_MSG_SEQ_NUM)
            {
                FinishNode(Pt);
            }
            return;
        }

        Proc_RecvAckedSeq(pFrame->SequenceNumber);
        return;
    }

    WORD Pt = Full_Search(pFrame->SequenceNumber,pNode);
    /* 节点 */
    if (pNode == NULL)
    {
        // 没有找到对应节点
        dcf_output("%s sender find node fail!(pt:%d,seq:%d,cf:%d,bPt:%d,ePt:%d,nums:%d)\r\n",m_channel.print_ownapp,Pt,pFrame->SequenceNumber,
                   pFrame->CurFrames,m_beginPt,m_endPt,m_NodeNums);
        return;
    }

#if DBP_DTFRAME
    dcf_output("[%s]%s sender recv msg(seq:%d,cf:%d,tf:%d,bg:%d,sw:%d,rw:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                         pFrame->SequenceNumber,pFrame->CurFrames,pNode->totalFrame,pNode->Frame_curBegin,pNode->frame_send_wait,
                         pFrame->wMsg2);
#endif

    /* 2017-06-27  23:43:43 调整为发送方的滑动窗口*/
    pNode->rwnd = pFrame->wMsg2;
    /* 2017-06-27  18:22:50 CurFrames 为了避免0号帧，调整为期望的下一帧帧号，而不是当前签收的帧号*/
    if (pFrame->CurFrames > pNode->frame_send_wait)
    {
        // 乱搞的帧,都还没有发送
#if DBP_DTFRAME
        dcf_output("[%s]%s sender recv except frame(seq:%d,cf:%d,sf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames,pNode->frame_send_wait);
#endif
        return;
    }

    if ((pFrame->CurFrames) && (pFrame->CurFrames <= pNode->Frame_curBegin))
    {
        // 只能请求其前面一帧
        dcf_output("%s sender recv unespect frame!(cir:0x%x,seq:%d,cf:%d,tf:%d,bf:%d)\r\n",m_channel.print_ownapp,
                   pFrame->cirtifyid,pNode->SequenceNumber,pFrame->CurFrames,pNode->totalFrame,pNode->Frame_curBegin);
        return;
    }

    bool bsureframe = true;
    if ((!pFrame->CurFrames) && (pFrame->ctrl_get(RCC_CTRL_BIT_WANT)))
    {
        // 第一帧是主动请求的。没有收到第0帧，但收方需要第0帧的情况
        bsureframe = false;
    }
    else if (pFrame->CurFrames < pNode->Frame_curBegin)
    {
        // 是请求帧
        bsureframe = false;
    }

    if (!bsureframe)
    {
        // 只是请求发送帧
        AskRetryHead(pNode);
        return;
    }

    // 下面是确认要移动的帧
    // 将窗口移动
    MakeSureToFrame(pNode,pFrame->CurFrames);
    AckedFrame(pNode);

    if (pNode->Frame_curBegin >= pNode->totalFrame)
    {
        // 所有的帧发送完了
        // 发送成功了
#if (!DBP_DTFRAME_NO)
        if (pNode->totalFrame > 1)
        {
            dcf_output("[%s]%s sender finish(tf:%d,seq:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->totalFrame,pNode->SequenceNumber);
        }
#elif DBP_DTFRAME
        dcf_output("[%s]%s sender send finish(%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->SequenceNumber);
#endif

        NotifyData(pNode, bHost);

        // 释放本节点
        FinishNode(Pt);
        return;
    }

    m_channel.Progress(pNode->wMsg2,pNode->Frame_curBegin,pNode->totalFrame); //add by hhw 2023.12.4 用于pda数据发送数据时上报进度

    // 窗口调整了，可以重新发送一轮
    if (!check_send_window(pNode))
    {
        AskRetryHead(pNode);
    }
}

/****************************************************************
*功能描述 : 确认序号帧的应答，对应序号是表示未确认的(之前的seq都已经确认)，但只对单包数据帧有效
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  12:1:17
****************************************************************/
void CMsgSender::Proc_RecvAckedSeq(WORD seq)
{
    WORD begin_seq = GetSequence(m_beginPt);
    WORD end_seq  = GetSequence(m_endPt);
#if DBP_DTFRAME
    dcf_output("[%s]%s sender ackseq:cur:(%d:%d),pt:(%d,%d),endseq:%d)\r\n", dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                begin_seq, end_seq, m_beginPt, m_endPt,seq);
#endif
    if ((!begin_seq) || (!end_seq))
    {
        return;
    }

    BYTE bReson = 0;
    WORD seqPt = FindSequence(seq,bReson);
    if (bReson == FR_OLD)
    {
        // 过时节点,已经确认过了,丢弃
        return;
    }

    if(bReson == FR_NEW)
    {
        seqPt = m_endPt;
    }

            ASSERT(seqPt < MAX_MSG_SEQ_NUM);

    if (bReson == FR_NEAR)
    {
        // 需要往前面减一个
        if (seqPt) seqPt--;
        else seqPt = MAX_MSG_SEQ_NUM - 1;
    }

    WORD curPt = m_beginPt;
    // 在此序号之前的节点都需要干掉
    for(;curPt != seqPt;curPt = (curPt + 1) % MAX_MSG_SEQ_NUM)
    {
        if (!m_sequence_list[curPt])
        {
            continue;
        }

        if (m_sequence_list[curPt]->totalFrame <= 1)
        {
            // 只清理单包节点
            m_NodeNums = m_NodeNums?(m_NodeNums - 1):0;
#if DBP_DTFRAME
            dcf_output("[%s]%s sender clear seq:(seq:%d)\r\n", dcf_get_time_string_unsafe(),m_channel.print_ownapp, m_sequence_list[curPt]->SequenceNumber);
#endif
            FreeNode(m_sequence_list[curPt]);
            m_sequence_list[curPt] = NULL;
        }

        if (!m_sequence_list[m_beginPt])
        {
            m_beginPt = (m_beginPt+1)%MAX_MSG_SEQ_NUM;
        }
    }


    // 清理空节点
    for(curPt = m_beginPt;curPt != m_endPt;)
    {
        if (m_sequence_list[curPt])
        {
            // 不用搬迁,往后移动
            curPt = (curPt + 1)%MAX_MSG_SEQ_NUM;
            continue;
        }

        // 本节点是空节点
        if (m_beginPt == curPt)
        {
            // 头节点跟着移动
            m_beginPt = (m_beginPt + 1)%MAX_MSG_SEQ_NUM;
        }

        for(WORD delPt = curPt;delPt != m_endPt;)
        {
            WORD wNextPt = (delPt+ 1)%MAX_MSG_SEQ_NUM;
            m_sequence_list[delPt] = m_sequence_list[wNextPt];
            delPt = wNextPt;
        }

        /* 移动了，尾巴置空 */
        m_sequence_list[m_endPt] = NULL;
        // 移动尾巴
        if (m_endPt) m_endPt--;
        else m_endPt = MAX_MSG_SEQ_NUM - 1;
    }

    // 最后一个节点
    if ((!m_sequence_list[m_endPt]) && (m_beginPt != m_endPt))
    {
        m_sequence_list[m_endPt] = NULL;
        if (m_endPt) m_endPt--;
        else m_endPt = MAX_MSG_SEQ_NUM - 1;
    }
}


/****************************************************************
*功能描述 : 超时定时器处理
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  14:27:33
****************************************************************/
void CMsgSender::ProcTimer()
{
    WORD Pt = m_beginPt;
    WORD old_num = m_NodeNums;
    m_NodeNums = 0;
    for(;;)
    {
        FrameSend *pNode = m_sequence_list[Pt];
        if (pNode)
        {
            m_NodeNums ++;
            //dcf_output("[%s]NodeTimer_Main start...\r\n", dcf_get_time_string_unsafe());
            NodeTimer_Main(pNode,Pt);
            //dcf_output("[%s]NodeTimer_Main end...\r\n", dcf_get_time_string_unsafe());
        }

        if (Pt == m_endPt)
        {
            break;
        }

        Pt = (Pt + 1)%MAX_MSG_SEQ_NUM;
    }

    bool bSend = false;
    if (m_NodeNums != old_num)
    {
        dcf_output("[%s]%s sender seq nums not same(%d,%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,old_num,m_NodeNums);
        bSend = true;
    }

    if(m_NodeNums)
    {
        m_total_times++;
        if (m_total_times > 10)
        {
            // 间隔10秒发送一次总帧数
            bSend = true;
            m_total_times = 0;
        }
    }

    if (bSend)
    {
        SendSeqRange();
    }
}
/****************************************************************
*功能描述 : 处理一个节点的定时器
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-17  14:32:37
****************************************************************/
void CMsgSender::NodeTimer_Main(FrameSend *pNode,WORD Pt)
{
    if (pNode->curBegin_stimes >= RETRY_TOTAL_TIMES)
    {
        dcf_output("[%s]%s sender excute sendtimes force to send next!(seq:%d,cf:%d,tf:%d,times:%d)\r\n",
                   dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->SequenceNumber,pNode->Frame_curBegin,pNode->totalFrame,pNode->curBegin_stimes);

        MakeSureToFrame(pNode,pNode->Frame_curBegin+1);

        if (pNode->Frame_curBegin >= pNode->totalFrame)
        {
            FinishNode(Pt);
        }
        else
        {
            /* 移动之后发送下一帧 */
            SendFrame(pNode,pNode->Frame_curBegin);
        }

        return;
    }

    if (pNode->send_con_faild)
    {
        // 先前发送失败了，现在定时器再尝试一下,别忘记了
        check_send_window(pNode);
    }

    if (pNode->send_times_left)
    {
        pNode->send_times_left--;
        if (!(pNode->send_times_left % SEND_DELAY_TIMES))
        {
            // 可以主动重新发送一次了
            SendFrame(pNode,pNode->Frame_curBegin);
        }
    }
    else
    {
        // 只能继续再来一遍了,总超时?
#if DBP_DTFRAME
        dcf_output("[%s]%s sender timer repeat(seq:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->SequenceNumber);
#endif
        pNode->send_times_left = RETRY_TOTAL_TIMES;
    }

    // 先把当前帧的计时增加
    pNode->head_act_rtts++;
    WORD wMax = g_swnd_rtt_table[pNode->awnd_idx];
    //dcf_output("[%s]NodeTimer_Main(head_act_rtts:%d,wMax:%d)\r\n", dcf_get_time_string_unsafe(), pNode->head_act_rtts, wMax);
    if(pNode->head_act_rtts < wMax)
    {
        // 还没有到阈值
        return;
    }

    // 下一轮RTT评估清0
    pNode->head_act_rtts = 0;

    // 调整级别等
    //dcf_output("[%s]NodeTimer_Jibie start...\r\n", dcf_get_time_string_unsafe());
    NodeTimer_Jibie(pNode);
    //dcf_output("[%s]NodeTimer_Jibie end...\r\n", dcf_get_time_string_unsafe());

    // 重新发送数据
    if (!check_send_window(pNode))
    {
        // 一帧都没有发送，将首帧发送一下,避免挂死
#if DBP_DTFRAME
        dcf_output("[%s]%s sender timer force send begin(curseq:%d,cf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->SequenceNumber,pNode->Frame_curBegin);
#endif
        SendFrame(pNode,pNode->Frame_curBegin);
    }

    // 下一轮质量评估控制清0
    pNode->send_frames_ok = 0;
    pNode->send_frames_retry = 0;
    pNode->ack_times = 0;    /* 要调整? */
}
/****************************************************************
*功能描述 : 调整窗口等级与状态
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-17  15:18:32
****************************************************************/
void CMsgSender::NodeTimer_Jibie(FrameSend *pNode)
{
    // 到达了阈值，需要检查各种信息来调整信息
    // 不存在确认中间帧的情况
    // WORD wSureNums = dcf_bittools::count_from_head(pNode->msgSurebit,pNode->swnd,1);
    // pNode->send_frames_ok += wSureNums;
    //dcf_output("[%s]NodeTimer_Jibie ack_time:%d\r\n", dcf_get_time_string_unsafe(), pNode->ack_times);
    if (pNode->ack_times < WND_ACKTIMES_MAX)
    {
        //dcf_output("[%s]NodeTimer_Jibie_Good start...\r\n", dcf_get_time_string_unsafe());
        NodeTimer_Jibie_Good(pNode);
        //dcf_output("[%s]NodeTimer_Jibie_Good end...\r\n", dcf_get_time_string_unsafe());
    }
    else
    {
        //dcf_output("[%s]NodeTimer_Jibie_Bad start...\r\n", dcf_get_time_string_unsafe());
        NodeTimer_Jibie_Bad(pNode);
        //dcf_output("[%s]NodeTimer_Jibie_Bad end...\r\n", dcf_get_time_string_unsafe());
    }
}
/****************************************************************
*功能描述 : 非连续请求的场景，初步判断质量好的情况
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  15:20:41
****************************************************************/
void CMsgSender::NodeTimer_Jibie_Good(FrameSend *pNode)
{
    // 收方请求的次数少，质量好
    // 通信质量评估
    //dcf_output("[%s] sender NodeTimer_Jibie_Good:sfo:(%d),sfr:(%d),gsstr:%d)\r\n", dcf_get_time_string_unsafe(), pNode->send_frames_ok, pNode->send_frames_retry, g_swnd_size_table_rtt[pNode->awnd_idx]);
    if ((pNode->send_frames_ok > pNode->send_frames_retry) && (pNode->send_frames_ok >= g_swnd_size_table_rtt[pNode->awnd_idx]))
    {
        // 在1个RRT中将窗口中的帧传输完成，且大于重传帧，则判断为网络质量好，可以调整窗口
        if (pNode->frame_state == FS_SLAVE_START)
        {
            if ((pNode->awnd_idx + 1) == wnd_table_size)
            {
                /* 此前已经在最高质量等级,则要进入流控状态 */
                pNode->frame_state = FS_FLOW_CTRL;
            }
            else
            {
                pNode->awnd_idx = (pNode->awnd_idx + 1)%wnd_table_size;
            }

            pNode->cwnd = g_swnd_size_table_rtt[pNode->awnd_idx];
            pNode->ssthresh = SWND_SSTHRESH_INIT;
            //dcf_output("[%s] sender NodeTimer_Jibie_Good:cwnd:(%d),awnd_idx:(%d),ssthresh:%d)\r\n", dcf_get_time_string_unsafe(), pNode->cwnd, pNode->awnd_idx, pNode->ssthresh);
        }
        else if (pNode->frame_state == FS_FLOW_CTRL)
        {
            // 每次只能将控制窗口+1
            pNode->cwnd ++;
            if (pNode->cwnd > MAX_WINDOW_SIZE)
            {
                pNode->cwnd = MAX_WINDOW_SIZE;
            }
        }
        else
        {
            // 快速恢复模式的质量也会好，在收到新帧的ack时去调整，在这里不评估和改变

        }
    }
    else
    {
        NodeTimer_Jibie_Bad(pNode);
    }
}
/****************************************************************
*功能描述 : 初判定通信质量差的处理
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-17  15:22:9
****************************************************************/
void CMsgSender::NodeTimer_Jibie_Bad(FrameSend *pNode)
{
    // 通信质量差
    if (pNode->frame_state == FS_SLAVE_START)
    {
        // 在慢模式下，需要往下调级
        if (pNode->awnd_idx) pNode->awnd_idx--;
        pNode->cwnd = g_swnd_size_table_rtt[pNode->awnd_idx];
    }
    else if (pNode->frame_state == FS_FLOW_CTRL)
    {
        // 在流控模式下，则进入慢模式的通道
        pNode->frame_state = FS_SLAVE_START;
        pNode->awnd_idx = wnd_table_size/2;
        pNode->cwnd = g_swnd_size_table_rtt[pNode->awnd_idx];
        pNode->ssthresh = SWND_SSTHRESH_INIT;
    }
    else
    {
        // 在快速恢复模式下质量差,还是由其确认帧去调整
    }
}

/****************************************************************
*功能描述 : 确认至指定帧号，不用管中间还有哪些没有确认
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  12:4:39
****************************************************************/
void CMsgSender::MakeSureToFrame(FrameSend *pNode,WORD wFrameIdx)
{
            ASSERT(wFrameIdx >= pNode->Frame_curBegin);

    /* 2017-07-13  18:8:55 收方是请求要下一帧 */
    if (wFrameIdx == (pNode->Frame_curBegin + 1))
    {
        pNode->curBegin_stimes = 0;
    }

    WORD wNums = wFrameIdx - pNode->Frame_curBegin;
    // 发送成功，统计
    pNode->send_frames_ok += wNums;

    // 确认窗口移动
    if (wNums)
    {
        dcf_bittools::left_move(pNode->msgSurebit,SWD_BYTES,wNums);
        pNode->Frame_curBegin += wNums;
        pNode->curBegin_stimes = 0;
    }
}

/****************************************************************
*功能描述 : 处理收方请求头部帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  13:43:12
****************************************************************/
void CMsgSender::AskRetryHead(FrameSend *pNode)
{
    // 需要的次数++
    pNode->ack_times++;
    //dcf_output("[%s] AskRetryHead:ack_times:(%d)\r\n", dcf_get_time_string_unsafe(), pNode->ack_times);
    if (pNode->ack_times % ACK_RETRY_TIMES)
    {
        // 防止多次请求，造成更多的拥塞
        return;
    }

    if (pNode->frame_state != FS_QUICK_RESTORE)
    {
        /*
        当前不在快速重传阶段，那么需要进行如下调整:
        1.ssthresh为wnd一半
        2.
        */
        /* 调整进入慢 */
        pNode->ssthresh = pNode->swnd/2;
        if (!pNode->ssthresh) pNode->ssthresh = 1;

        pNode->frame_state = FS_QUICK_RESTORE;
    }
    else
    {
        // 已经是快速恢复模式了，没有啥处理
    }

    // 发送这一帧
    (void)SendFrame(pNode,pNode->Frame_curBegin);
}

/****************************************************************
*功能描述 : 因接收方确认帧之后调整控制变量
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-17  16:30:1
****************************************************************/
void CMsgSender::AckedFrame(FrameSend *pNode)
{
    pNode->send_con_faild = 0;
    pNode->send_times_left = RETRY_TOTAL_TIMES;
}

/****************************************************************
*功能描述 : 检查调整发送窗口，如果有满足条件的帧，则会发送
*输入参数 : NA
*输出参数 : NA
*返回参数 : 发送的帧数量
*作   者 : zjb
*创建日期 : 2017-06-17  15:28:1
****************************************************************/
WORD CMsgSender::check_send_window(FrameSend *pNode)
{
    // 1.检查接收方的rwnd
    if (!pNode->rwnd) pNode->rwnd = 1;  /* 至少要能发送一个 */
    // 2.调整自己的发送窗口
    pNode->swnd = pNode->rwnd;
    if (pNode->cwnd < pNode->rwnd)
    {
        pNode->swnd = pNode->cwnd;
    }
    //dcf_output("[%s] check_send_window:pNode->rwnd:(%d),pNode->swnd:(%d),pNode->cwnd:(%d)\r\n", dcf_get_time_string_unsafe(), pNode->rwnd, pNode->swnd, pNode->cwnd);
    // 防止没有初始化
            ASSERT(pNode->frame_send_wait >= pNode->Frame_curBegin);
            ASSERT(pNode->frame_send_wait <= pNode->totalFrame);
    // if (pNode->frame_send_wait < pNode->Frame_curBegin) pNode->frame_send_wait = pNode->Frame_curBegin;
    // if (pNode->frame_send_wait > pNode->totalFrame) pNode->frame_send_wait = pNode->totalFrame;
    //dcf_output("[%s] check_send_window:pNode->frame_send_wait:(%d),pNode->Frame_curBegin:(%d)\r\n", dcf_get_time_string_unsafe(), pNode->frame_send_wait, pNode->Frame_curBegin);
    if ((pNode->frame_send_wait - pNode->Frame_curBegin) >= pNode->swnd)
    {
        // 不能再发送了
        pNode->send_con_faild = 0;
        //dcf_output("[%s] check_send_window:can't send\r\n", dcf_get_time_string_unsafe());
        return 0;
    }

    //WORD wEndPt = pNode->frame_send_wait + pNode->swnd;
    WORD wEndPt = pNode->Frame_curBegin + pNode->swnd;

    if (wEndPt > pNode->totalFrame)
    {
        wEndPt = pNode->totalFrame;
    }

    WORD iNums = 0;
    for(;pNode->frame_send_wait < wEndPt;)
    {
        if (SendFrame(pNode, pNode->frame_send_wait))
        {
            // 发送失败了就不要再发送了
            pNode->send_con_faild = 1;
            return iNums;
        }

        iNums++;
    }

    pNode->send_con_faild = 0;
    return iNums;
}

/****************************************************************
*功能描述 : 发完所有数据，通知应用层
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     hhw
*创建日期 : 2023-06-29  17:37:45
****************************************************************/
void CMsgSender::NotifyData(FrameSend* pNode, BYTE bhost)
{
    MSG_RPC msg = { 0 };
    /* 填写其它信息 */
    msg.ctrl = RCC_CTRL_FLAG_RSP_SURE;
    msg.bhost = bhost;
    msg.MsgIP = m_channel.dstIP;
    msg.MsgPort = m_channel.dstPort;
    msg.Msg_Secret_Ver = pNode->Msg_Secret_Ver;
    msg.wMsg1 = pNode->wMsg1;
    msg.wMsg2 = pNode->wMsg2;
    //msg.cirtifyid = m_channel.cirtifyid;   /* 2017-09-15  15:2:15 将bs挂在公网之后,端口会变化，因此不能用ip和端口两个去匹配 */
    msg.wMsgType = RCC_MSG_TYPE_RAW_BSS_CRM;

    msg.pMsg = (BYTE*)dcf_mem_malloc(pNode->MsgLen);
    memset(msg.pMsg, 0, pNode->MsgLen);
    memcpy(msg.pMsg, pNode->pSendMsg, pNode->MsgLen);
    msg.MsgLen = pNode->MsgLen;
    DWORD dwRet = m_channel.Response(msg, m_channel.pThis);
    dcf_sys_checkerr(dwRet, LEVEL_LOW_ERROR, 1, "Response");
    if ((dwRet) && (msg.pMsg))
    {
        // 对方未释放内存
        dcf_output("[%s]%s recver notify app multi msg failed!(msg1:%u,msg2:%u)\r\n", dcf_get_time_string_unsafe(), m_channel.print_ownapp, pNode->wMsg1, pNode->wMsg2);
        dcf_time_sleep(10);
        dcf_mem_free((void*&)msg.pMsg);
        msg.pMsg = NULL;
    }
}

#if 0
// 分隔一下
#endif
/******************************************************************************************************************
******************************************************************************************************************
******************************************************************************************************************
******************************************************************************************************************
******************************************************************************************************************/
CMsgRecver::CMsgRecver(COMM_CHANNEL &Info)
{
    memcpy(&m_channel,&Info,sizeof(m_channel));

    memset(m_sequence_multi_list,0,sizeof(m_sequence_multi_list));
    m_mult_beginPt = 0;
    m_mult_endPt = 0;

    /* 确认总表 */
    m_seq_sure_total = CFSWndCtrl::GetWndCtrl(SEQ_WND_SIZE);
    m_bNeedNotifySeq = 0;
    m_bNeedMoveWndSeq = 0;
    m_seq_ageing_tools = CFsAgeingTable::GetTable(WND_AGEING_TIMEOUT);
    /* 注意帧号是从1开始 */
    // m_seq_sure_total->wnd_move(1);
}

CMsgRecver::~CMsgRecver()
{
    ClearAll();
}

/****************************************************************
*功能描述 : 清除所有待确认节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  14:16:52
****************************************************************/
void CMsgRecver::ClearAll()
{
    m_mult_endPt = m_mult_endPt %MAX_MSG_SEQ_NUM;
    for(;;)
    {
        if (m_sequence_multi_list[m_mult_beginPt])
        {
            FreeNode(m_sequence_multi_list[m_mult_beginPt]);
            m_sequence_multi_list[m_mult_beginPt] = NULL;
        }

        if (m_mult_beginPt == m_mult_endPt)
        {
            break;
        }

        m_mult_beginPt = (m_mult_beginPt + 1)%MAX_MSG_SEQ_NUM;
    }

    dcf_mem_free((void*&)m_seq_sure_total);
    m_seq_sure_total = NULL;
    if (m_seq_ageing_tools)
    {
        CFsAgeingTable::FreeTable(m_seq_ageing_tools);
    }
}

/****************************************************************
*功能描述 : 切换端口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-14  11:3:31
****************************************************************/
void CMsgRecver::ChangePort(WORD wPort)
{
    m_channel.dstPort = wPort;
}


/****************************************************************
*功能描述 : 获取指定节点的报文序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  13:44:10
****************************************************************/
WORD CMsgRecver::GetSequence(WORD Pt)
{
    if ((Pt < MAX_MSG_SEQ_NUM) && m_sequence_multi_list[Pt])
    {
        return m_sequence_multi_list[Pt]->SequenceNumber;
    }
    return 0;
}

/****************************************************************
*功能描述 : 获取指定节点的对象
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  13:45:46
****************************************************************/
FrameRecv *CMsgRecver::GetNodePtr(WORD Pt)
{
    if (Pt < MAX_MSG_SEQ_NUM)
    {
        return m_sequence_multi_list[Pt];
    }
    return NULL;
}
/****************************************************************
*功能描述 : 查找对应的序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  13:48:37
1修改描述 : 修改为可以查找相近节点
*修改作者 : zjb
*修改时间 : 2017-06-19  17:14:16
****************************************************************/
WORD CMsgRecver::FindMultSequence(WORD Seq,BYTE &bReson)
{
    WORD curPt = m_mult_beginPt;
    WORD wBeginSeq = GetSequence(m_mult_beginPt);
    WORD wEndSeq = GetSequence(m_mult_endPt);

    if ((!wBeginSeq) || (!wEndSeq))
    {
        // 头或者位为空
        bReson = FR_NEW;
        return MAX_MSG_SEQ_NUM;
    }

    if (Seq == wBeginSeq)
    {
        bReson = FR_OK;
        return m_mult_beginPt;
    }

    if (Seq == wEndSeq)
    {
        bReson = FR_OK;
        return m_mult_endPt;
    }

    if (wEndSeq >= wBeginSeq)
    {
        // 还没有绕接的场景
        if (Seq < wBeginSeq)
        {
            // 已经无效
            bReson = FR_OLD;
            return MAX_MSG_SEQ_NUM;
        }

        if (Seq > wEndSeq)
        {
            // 新节点
            bReson = FR_NEW;
            return MAX_MSG_SEQ_NUM;
        }

        // 找中间的
        /* 下面这段算法可以进行优化 有空弄*/
        for(;curPt != m_mult_endPt;curPt = (curPt + 1)%MAX_MSG_SEQ_NUM)
        {
            if (!m_sequence_multi_list[curPt])
            {
                continue;
            }

            if (m_sequence_multi_list[curPt]->SequenceNumber == Seq)
            {
                bReson = FR_OK;
                return curPt;
            }

            if (m_sequence_multi_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }
        }

        dcf_output("exception!");
        bReson = FR_OLD;
        return MAX_MSG_SEQ_NUM;
    }

    // 反转后的找法
    // 先把下面算法无法甄别的搞定，根据距离判断
    if ((Seq < wBeginSeq) && (Seq > wEndSeq))
    {
        if ((wBeginSeq - Seq) > (Seq - wEndSeq))
        {
            // 更近尾巴一点，判定为新节点
            bReson = FR_NEW;
        }
        else
        {
            // 近头部一点,判断为过时的节点
            bReson = FR_OLD;
        }
        return MAX_MSG_SEQ_NUM;
    }

    // 1.划分区域
    BYTE bLeft = 0;
    if (Seq >= wBeginSeq)
    {
        bLeft = 1;
    }

    for(;curPt != m_mult_endPt;curPt = (curPt+1)%MAX_MSG_SEQ_NUM)
    {
        if (!m_sequence_multi_list[curPt])
        {
            continue;
        }

        if (m_sequence_multi_list[curPt]->SequenceNumber == Seq)
        {
            bReson = FR_OK;
            return curPt;
        }

        // 在区域中查找
        if (bLeft)
        {
            if (m_sequence_multi_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }

            if (m_sequence_multi_list[curPt]->SequenceNumber < wBeginSeq)
            {
                // 到了开始反转的地方了，往后不可能再有
                bReson = FR_NEAR;
                return curPt;
            }
        }
        else
        {
            if (m_sequence_multi_list[curPt]->SequenceNumber > wBeginSeq)
            {
                // 先要找到临界点
                continue;
            }

            // 在右边区域内找
            if (m_sequence_multi_list[curPt]->SequenceNumber > Seq)
            {
                bReson = FR_NEAR;
                return curPt;
            }
        }
    }

    // 走到这里应该是end节点为空,判定为尾巴的
    bReson = FR_NEW;
    return MAX_MSG_SEQ_NUM;
}

/****************************************************************
*功能描述 : 调整首指针
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  14:3:7
****************************************************************/
void CMsgRecver::AdjustHeadPtr()
{
    m_mult_endPt = m_mult_endPt %MAX_MSG_SEQ_NUM;
    for(;m_mult_beginPt != m_mult_endPt;)
    {
        if (m_sequence_multi_list[m_mult_beginPt])
        {
            break;
        }
        m_mult_beginPt = (m_mult_beginPt + 1)%MAX_MSG_SEQ_NUM;
    }
}

/****************************************************************
*功能描述 : 调整尾指针
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  14:3:26
****************************************************************/
void CMsgRecver::AdjustTailPtr()
{
    m_mult_beginPt = m_mult_beginPt %MAX_MSG_SEQ_NUM;
    for(;m_mult_endPt != m_mult_beginPt;)
    {
        if (m_sequence_multi_list[m_mult_endPt])
        {
            break;
        }

        if (!m_mult_endPt)
        {
            m_mult_endPt = MAX_MSG_SEQ_NUM - 1;
        }
        else
        {
            m_mult_endPt--;
        }
    }
}

/****************************************************************
*功能描述 : 根据对应报文生成节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  18:5:40
****************************************************************/
FrameRecv *CMsgRecver::CopyNode(BYTE *pMsg,WORD MsgLen)
{
    // 分配内存拷贝原数据
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);

    FrameRecv *pNode = (FrameRecv*)dcf_mem_malloc(sizeof(FrameRecv));
    memset(pNode,0,sizeof(FrameRecv));

    pNode->wMsg1 = pFrame->wMsg1;
    pNode->wMsg2 = pFrame->wMsg2;
    pNode->Secret_Ver = pHeader->Secret_Ver;
    pNode->msg_ctrl = pFrame->ctrl;
    pNode->SequenceNumber = pFrame->SequenceNumber;
    pNode->totalFrame = pFrame->CurFrames;
    // 每包用户的数据长度
    pNode->PacketLen = pHeader->PackageLen - RCC_SIZE_FRAME;
    // 总数据长度(最后一包是按照完整大小来分配的)
    DWORD dwContendLen = pNode->PacketLen * pNode->totalFrame;
    pNode->pRecvMsg = (BYTE*)dcf_mem_malloc(dwContendLen);
    memset(pNode->pRecvMsg,0,dwContendLen);
    return pNode;
}

void CMsgRecver::FreeNode(FrameRecv *&pNode)
{
    if (!pNode)
    {
        return;
    }

    if (pNode->pRecvMsg)
    {
        dcf_mem_free((void*&)pNode->pRecvMsg);
        pNode->pRecvMsg = NULL;
    }

    dcf_mem_free((void*&)pNode);
    pNode = NULL;
}
/****************************************************************
*功能描述 : 定时器
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  10:23:13
****************************************************************/
void CMsgRecver::ProcTimer()
{
    //dcf_output("[%s]dcframe ProcTimer start...\r\n", dcf_get_time_string_unsafe());
    // 1.发送确认帧
    if (m_bNeedNotifySeq)
    {
        Notify_AckSeq(0);
        m_bNeedNotifySeq = 0;
    }

    // 2.总表的维护
    Timer_Total_Maintain();

    // 3.多帧表的维护
    WORD i = m_mult_beginPt;
    for(;i != m_mult_endPt;i++)
    {
        i = i % MAX_MSG_SEQ_NUM;
        if (!m_sequence_multi_list[i])
        {
            continue;
        }
        //dcf_output("[%s]Timer_Mult_ProcNode start...[i:%d]\r\n", dcf_get_time_string_unsafe(), i);
        Timer_Mult_ProcNode(m_sequence_multi_list[i]);
        //dcf_output("[%s]Timer_Mult_ProcNode end...\r\n", dcf_get_time_string_unsafe());
    }

    if (m_sequence_multi_list[m_mult_endPt])
    {
        Timer_Mult_ProcNode(m_sequence_multi_list[m_mult_endPt]);
    }
    //dcf_output("[%s]dcframe ProcTimer end...\r\n", dcf_get_time_string_unsafe());
}

/****************************************************************
*功能描述 : 多帧报文的定时处理
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-27  15:6:28
****************************************************************/
void CMsgRecver::Timer_Mult_ProcNode(FrameRecv *pNode)
{
    const WORD WND_MAX_SIZE = g_swnd_size_table_rtt[wnd_table_size - 1];
    //dcf_output("[%s]frame_errors[%d], frame_repeat[%d], frame_recvs[%d]\r\n", dcf_get_time_string_unsafe(), pNode->frame_errors, pNode->frame_repeat, pNode->frame_recvs);
    if ((pNode->frame_errors) || (pNode->frame_repeat) || (pNode->frame_recvs > (WND_MAX_SIZE / 2)) || (!pNode->frame_recvs))
    {
        /* 有错误帧 */
#if DBP_DTFRAME
        dcf_output("[%s]%s recv timer ask frame(seq:%d,cf:%d,tf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->SequenceNumber,pNode->totalFrame,pNode->Frame_curBegin);
#endif
        Multi_Frame_Ack(pNode);
        pNode->frame_errors = 0;
        pNode->frame_repeat = 0;
        pNode->frame_recvs = 0;
    }
}

/****************************************************************
*功能描述 : 总表的确认表维护
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  10:49:37
****************************************************************/
void CMsgRecver::Timer_Total_Maintain()
{
    // 1.看是否有数据帧
    if (!m_seq_sure_total->range_size())
    {
        return;
    }

    /* 处理总表移动问题 */
    const WORD can_move_size = (SEQ_WND_SIZE*3)/4;
    if (m_bNeedMoveWndSeq && (m_seq_sure_total->range_size() >= can_move_size))
    {
#if DBP_DTFRAME
        WORD cur_begin = m_seq_sure_total->wnd_head();
        WORD curend = m_seq_sure_total->wnd_tail();
#endif
        WORD wSize = m_seq_sure_total->range_size();
        m_seq_sure_total->wnd_move(WND_TOTAL_MOVE_SIZE_BIT);
        m_bNeedNotifySeq = 1;
#if DBP_DTFRAME
        dcf_output("[%s]%s recv force move cur:(b:%d,e:%d)->(b:%d,e:%d);len:(%d->%d):%d\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                                     cur_begin,curend,m_seq_sure_total->wnd_head(),m_seq_sure_total->wnd_tail(),
                                     wSize,m_seq_sure_total->range_size());
#endif
    }

    /* 处理表的老化问题 */
    Timer_Total_Ageing();

    if (m_bNeedNotifySeq)
    {
        // 移动之后需要通知一下
        Notify_AckSeq();
    }
}
/****************************************************************
*功能描述 : 处理总表的老化问题
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-01  14:21:46
****************************************************************/
void CMsgRecver::Timer_Total_Ageing()
{
    WORD curend = m_seq_sure_total->wnd_tail();
    WORD ageing_end;
    m_seq_ageing_tools->increase(curend,ageing_end);
    WND_CMP cmr_end = m_seq_sure_total->wnd_compare(ageing_end);

    if ((WCR_OLD == cmr_end) || (WCR_OUT_WND == cmr_end) || (!ageing_end))
    {
        // 老化表比总表旧，无需处理
        return;
    }

    WORD cur_begin = m_seq_sure_total->wnd_head();
    WORD max_len = m_seq_sure_total->wnd_can_move_nums();
    WORD age_len = m_seq_sure_total->idx_size(ageing_end);
    if (age_len > max_len)
    {
        age_len = max_len;
    }

    if (!age_len)
    {
        return;
    }

    m_seq_sure_total->wnd_move(age_len);
#if DBP_DTFRAME
    dcf_output("[%s]%s recv ageing cur:(b:%d,e:%d)->(b:%d,e:%d);(ag:%d,max:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                                 cur_begin,curend,m_seq_sure_total->wnd_head(),m_seq_sure_total->wnd_tail(),
                                 age_len,max_len);
#endif
    m_bNeedNotifySeq = 1;
}

/****************************************************************
*功能描述 : 处理socket收到的信息，进入此处已经完成frame层的转换
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  18:7:43
****************************************************************/
void CMsgRecver::ProcRecvMsg(BYTE *pMsg,WORD msgLen,BYTE bHost)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    if (pFrame->cirtifyid != m_channel.cirtifyid)
    {
        dcf_output("[%s]%s recv unespect cirtifyid!(0x%x,0x%x)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->cirtifyid,m_channel.cirtifyid);
        return;
    }

    if ((!pFrame->SequenceNumber) && (pFrame->ctrl == RCC_CTRL_FLAG_SENDER_SEQ_RANGE))
    {
        SureSenderSeqWnd(pMsg);
        return;
    }

    if (!pFrame->SequenceNumber)
    {
        // 是无需确认的帧
        dcf_output("[%s]%s recv zero one msg(msg1:%u,msg2:%u)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->wMsg1,pFrame->wMsg2);
        NotifyOnceData(pMsg,msgLen, bHost);
        return;
    }

    if (pFrame->IsFirstFrame())
    {
        // 是第一帧数据,那么CurFrames表示总帧数
        if (pFrame->CurFrames <= 1)
        {
            ProcOnceFrame(pMsg,msgLen, bHost);
            return;
        }

        // 多帧的情况
        ProcNewMultiFrame(pMsg,msgLen, bHost);
    }
    else
    {
        // 处理多帧的后续帧
        ProcNextFrame(pMsg,msgLen,bHost);
    }
}

/****************************************************************
*功能描述 : 在数据接收完成之后，通知应用模块数据
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  11:32:28
****************************************************************/
void CMsgRecver::NotifyOnceData(BYTE *pMsg,WORD msgLen,BYTE bhost)
{
    MSG_RPC msg = {0};
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    if ((msgLen <= RCC_SIZE_RAW_HEAD) || (msgLen > RCC_SIZE_TOTAL))
    {
        dcf_output("[%s]%s ****************recver try to send error app one msg(len:%u,msg1:%u,msg2:%u)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,msgLen,pFrame->wMsg1,pFrame->wMsg2);
        return;
    }

#if DBP_DTFRAME
    dcf_output("[%s]%s recv once frame(seq:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber);
#endif

    msg.bhost = bhost;
    msg.ctrl = pFrame->ctrl;
    msg.MsgIP = m_channel.dstIP;
    msg.MsgPort = m_channel.dstPort;
    msg.Msg_Secret_Ver = pHeader->Secret_Ver;
    msg.wMsg1 = pFrame->wMsg1;
    msg.wMsg2 = pFrame->wMsg2;
    msg.wMsgType = pHeader->Msgtype;
    //msg.cirtifyid = m_channel.cirtifyid;   /* 2017-09-15  15:2:15 将bs挂在公网之后,端口会变化，因此不能用ip和端口两个去匹配 */
    msg.MsgLen = msgLen - RCC_SIZE_RAW_HEAD;
    msg.pMsg = (BYTE*)dcf_mem_malloc(msgLen);
    memset(msg.pMsg,0,msgLen);
    memcpy(msg.pMsg,pMsg + RCC_SIZE_RAW_HEAD,msg.MsgLen);
    DWORD dwRet = m_channel.Recver(msg,m_channel.pThis);
    dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"Recver");
    if ((dwRet) && (msg.pMsg))
    {
        // 对方未释放内存
        dcf_output("[%s]%s recver notify app one msg failed!(msg1:%u,msg2:%u)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->wMsg1,pFrame->wMsg2);
        dcf_time_sleep(10);
        dcf_mem_free((void*&)msg.pMsg);
        msg.pMsg = NULL;
    }
}

/****************************************************************
*功能描述 : 收完所有数据，通知应用层
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-27  15:34:38
****************************************************************/
void CMsgRecver::NotifyMultiData(FrameRecv *pSeqNode,WORD msgType,BYTE bhost)
{
    DWORD MsgLen = (pSeqNode->totalFrame - 1)*pSeqNode->PacketLen+pSeqNode->LastFrameLen;
    MSG_RPC msg = {0};
#if (!DBP_DTFRAME_NO)
    dcf_output("[%s]%s recv mult packet and notify app(seq:%d,len:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pSeqNode->SequenceNumber,MsgLen);
#endif
    /* 填写其它信息 */
    msg.ctrl = pSeqNode->msg_ctrl;
    msg.bhost = bhost;
    msg.MsgIP = m_channel.dstIP;
    msg.MsgPort = m_channel.dstPort;
    msg.Msg_Secret_Ver = pSeqNode->Secret_Ver;
    msg.wMsg1 = pSeqNode->wMsg1;
    msg.wMsg2 = pSeqNode->wMsg2;
    //msg.cirtifyid = m_channel.cirtifyid;   /* 2017-09-15  15:2:15 将bs挂在公网之后,端口会变化，因此不能用ip和端口两个去匹配 */
    msg.wMsgType = msgType;
    msg.MsgLen = MsgLen;
    msg.pMsg = pSeqNode->pRecvMsg;
    pSeqNode->pRecvMsg = NULL;
    DWORD dwRet = m_channel.Recver(msg,m_channel.pThis);
    dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"Recver");
    if ((dwRet)&&(msg.pMsg))
    {
        // 对方未释放内存
        dcf_output("[%s]%s recver notify app multi msg failed!(msg1:%u,msg2:%u)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp, pSeqNode->wMsg1, pSeqNode->wMsg2);
        dcf_time_sleep(10);
        dcf_mem_free((void*&)msg.pMsg);
        msg.pMsg = NULL;
    }
}
/****************************************************************
*功能描述 : 预先检查该序号报文是否能处理
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-30  18:13:51
****************************************************************/
bool CMsgRecver::precheck_by_total(WORD seq,WND_CMP &cwr)
{
    cwr = m_seq_sure_total->wnd_compare(seq);
    if (m_seq_sure_total->IsAcked(seq, cwr))
    {
        // 已经确认了
        m_bNeedNotifySeq = 1;
        return false;
    }

    if (cwr == WCR_OUT_WND)
    {
        // 窗口没有那么大
        dcf_output("%s seq excite window(begin:%d,end:%d,cur:%d)\r\n",m_channel.print_ownapp,m_seq_sure_total->wnd_head(),m_seq_sure_total->wnd_tail(),seq);
        // 不在管辖的窗口内，要么已经确认，要么窗口放不下
        m_bNeedMoveWndSeq = 1;
        m_bNeedNotifySeq = 1;
        return false;
    }

    return true;
}


/****************************************************************
*功能描述 : 处理一包的确认帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-17  22:24:7
****************************************************************/
void CMsgRecver::ProcOnceFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost)
{
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    WND_CMP cwr;
    if (!precheck_by_total(pFrame->SequenceNumber,cwr))
    {
        if (pFrame->send_times >= ACK_RETRY_TIMES)
        {
            /* 对方一直发送该报文要给应答 */
            if (m_seq_sure_total->IsAcked(pFrame->CurFrames, cwr))
            {
                /* 是已经签收过的帧,通知对方OK */
                dcf_output("[%s]%s recv old frame,notify end!(seq:%d,cf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames);
                SendFrameAck(pMsg,true);
            }

            /* 通知一下当前的结束帧号 */
            Notify_AckSeq();
        }

        return;
    }

    // 窗口内或者是能存放的下的
#if DBP_DTFRAME
    WORD begin = m_seq_sure_total->wnd_head();
    WORD end = m_seq_sure_total->wnd_tail();
#endif

    m_seq_sure_total->wnd_save(pFrame->SequenceNumber,cwr);
    TryMoveTotalWnd(pFrame->SequenceNumber);
    m_seq_ageing_tools->push_idx(pFrame->SequenceNumber);
#if DBP_DTFRAME
    dcf_output("[%s]%s recv new one,save cur:%d,(begin:%d,end:%d)->(begin:%d,end:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                            pFrame->SequenceNumber,begin,end,
                            m_seq_sure_total->wnd_head(),m_seq_sure_total->wnd_tail());
#endif
    // 只有1帧数据，就只需要给一个确认即可,无需走到组帧环节
    // 需要解决重复收到的问题
    SendFrameAck(pMsg);
    // 通知应用层
    NotifyOnceData(pMsg,MsgLen,bHost);
}
/****************************************************************
*功能描述 : 处理多包的第一帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-18  13:4:11
****************************************************************/
void CMsgRecver::ProcNewMultiFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost)
{
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    WND_CMP cwr;
    if (!precheck_by_total(pFrame->SequenceNumber,cwr))
    {
        if (pFrame->send_times >= ACK_RETRY_TIMES)
        {
            /* 对方一直发送该报文要给应答 */
            if (m_seq_sure_total->IsAcked(pFrame->CurFrames, cwr))
            {
                /* 是已经签收过的帧,通知对方OK */
                dcf_output("[%s]%s recv old frame,notify end!(seq:%d,cf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames);
                SendFrameAck(pMsg,true);
            }

            /* 通知一下当前的结束帧号 */
            Notify_AckSeq();
        }
        return;
    }

    if (AddIntoMultiQue(pMsg,MsgLen))
    {
        // 应该是里面节点满了，无法再处理，只能先让发送方重传一段时间
        return;
    }

#if (!DBP_DTFRAME_NO)
    WORD begin = m_seq_sure_total->wnd_head();
    WORD end = m_seq_sure_total->wnd_tail();
#endif
    // 在这里队列里写ok标记
    m_seq_sure_total->wnd_save(pFrame->SequenceNumber,cwr);
    m_seq_ageing_tools->push_idx(pFrame->SequenceNumber);
    TryMoveTotalWnd(pFrame->SequenceNumber);
#if (!DBP_DTFRAME_NO)
    dcf_output("[%s]%s recv new muti curseq:%d,total:%u,(begin:%d,end:%d)->(begin:%d,end:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
               pFrame->SequenceNumber,pFrame->CurFrames,begin,end,
               m_seq_sure_total->wnd_head(),m_seq_sure_total->wnd_tail());
#endif
}
/****************************************************************
*功能描述 : 处理后续帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-18  13:16:45
****************************************************************/
void CMsgRecver::ProcNextFrame(BYTE *pMsg,WORD MsgLen,BYTE bHost)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    BYTE bReson = 0;
    WORD Pt = FindMultSequence(pFrame->SequenceNumber,bReson);
    if ((Pt >= MAX_MSG_SEQ_NUM) || (bReson))
    {
#if DBP_DTFRAME
        dcf_output("[%s]%s recv muti next frame (curseq:%d,cf:%d),find failed!\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames);
#endif
        WND_CMP cwr;
        precheck_by_total(pFrame->SequenceNumber,cwr);
        if (pFrame->send_times >= ACK_RETRY_TIMES)
        {
            /* 对方一直发送该报文要给应答 */
            if (m_seq_sure_total->IsAcked(pFrame->CurFrames, cwr))
            {
                /* 是已经签收过的帧,通知对方OK */
                dcf_output("[%s]%s recv old frame,notify end!(seq:%d,cf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames);
                SendFrameAck(pMsg,true);
            }

            /* 通知一下当前的结束帧号 */
            Notify_AckSeq();
        }
        return;
    }

    // 找到了对应的节点
    Multi_Add_Frame(Pt,pMsg,MsgLen,bHost);
}
/****************************************************************
*功能描述 : 在多包的节点中增加一个数据帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  17:44:15
****************************************************************/
void CMsgRecver::Multi_Add_Frame(WORD Pt,BYTE *pMsg,WORD MsgLen,BYTE bHost)
{
    FrameRecv *pSeqNode = GetNodePtr(Pt);
    if (!pSeqNode)
    {
                ASSERT(0);
        return;
    }
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    if (pFrame->CurFrames < pSeqNode->Frame_curBegin)
    {
#if DBP_DTFRAME
        dcf_output("[%s]%s recv muti next frame (curseq:%d,cf:%d),repeat!(cb:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames,pSeqNode->Frame_curBegin);
#endif
        // 收到一帧过时的报文
        pSeqNode->frame_repeat++;
        if (pFrame->send_times >= ACK_RETRY_TIMES)
        {
            /* 对方一直发送该报文要给应答 */
            SendFrameAck(pMsg);
        }
        return;
    }

    if (pFrame->CurFrames >= pSeqNode->totalFrame)
    {
        // 错误帧
        dcf_output("[%s]%s recv too large frameidx(seq:%d,cf:%d,tf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                   pSeqNode->SequenceNumber,pFrame->CurFrames,pSeqNode->totalFrame);
        pSeqNode->frame_errors++;
        return;
    }

    if (pFrame->CurFrames > (pSeqNode->Frame_curBegin + MAX_WINDOW_SIZE))
    {
        // 超过了窗口，当前版本丢弃
        pSeqNode->frame_errors++;
        dcf_output("[%s]%s recv excute wnd frame(seq:%d,cf:%d,tf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                   pSeqNode->SequenceNumber,pFrame->CurFrames,pSeqNode->totalFrame);
        return;
    }

    WORD bit_idx = pFrame->CurFrames - pSeqNode->Frame_curBegin;
    if (dcf_bittools::get_bit(pSeqNode->msgSurebit,SWD_BYTES,bit_idx))
    {
        // 已经确认过了的
        pSeqNode->frame_repeat++;
        dcf_output("[%s]%s recv old frame act again(seq:%d,cf:%d,tf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                   pSeqNode->SequenceNumber,pFrame->CurFrames,pSeqNode->totalFrame);
        if (pSeqNode->frame_repeat > ACK_RETRY_TIMES)
        {
            Multi_Frame_Ack(pSeqNode);
        }

        return;
    }

    // 窗口内还没有确认的帧
    if (Multi_Save_Frame(pSeqNode,pMsg))
    {
        // 保存错误
        pSeqNode->frame_errors++;
        return;
    }

    // 设置本帧已收
    dcf_bittools::set_bit(pSeqNode->msgSurebit,SWD_BYTES,bit_idx,1);
    if (bit_idx)
    {
        // 不是第一帧
        pSeqNode->frame_errors++;
        pSeqNode->frame_recvs++;
        return;
    }

    // 是当前窗口头部,先数一数可以移动多少
    WORD bits = dcf_bittools::con_nums_from_head(pSeqNode->msgSurebit,SWD_BYTES,1);
            ASSERT(bits > 0);
    dcf_bittools::left_move(pSeqNode->msgSurebit,SWD_BYTES,bits);
    pSeqNode->Frame_curBegin += bits;
    pSeqNode->frame_recvs++;
#if DBP_DTFRAME
    dcf_output("[%s]%s recv new mult frame(seq:%d,cf:%d,tf:%d,swnd:%d,cvs:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                                                                pSeqNode->SequenceNumber,pFrame->CurFrames,pSeqNode->totalFrame,
                                                                pFrame->wMsg2,pSeqNode->frame_recvs);
#endif

    if (pSeqNode->Frame_curBegin < pSeqNode->totalFrame)
    {
        // 连续收到的帧数超过了发送方的窗口数
        if (pSeqNode->frame_recvs >= (pFrame->wMsg2 / 2))
        {
            // 连续每次收到了窗口一半的时候,通知发送下一波
#if DBP_DTFRAME
            dcf_output("[%s]%s recv helfwnd,and ask next(seq:%d,cf:%d,tf:%d,swnd:%d,cvs:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,
                                                                        pSeqNode->SequenceNumber,pFrame->CurFrames,pSeqNode->totalFrame,
                                                                        pFrame->wMsg2,pSeqNode->frame_recvs);
#endif
            Multi_Frame_Ack(pSeqNode);
            pSeqNode->frame_recvs = 0;
            pSeqNode->frame_errors = 0;
        }

        // 还没有收完
        return;
    }

    // 先给发送方回应答
    Multi_Frame_Ack(pSeqNode);
    // 收完所有数据
    NotifyMultiData(pSeqNode,pHeader->Msgtype,bHost);
    FinishNode(Pt);
}

/****************************************************************
*功能描述 : 增加一个新的多包节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  18:0:35
****************************************************************/
DWORD CMsgRecver::AddIntoMultiQue(BYTE *pMsg,WORD msgLen)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    WORD newPt = (m_mult_endPt + 1) %MAX_MSG_SEQ_NUM;
    if (newPt == m_mult_beginPt)
    {
        dcf_output("[%s]%s recv multi is full(seq:%d)...\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber);
        return DCF_ERR_FULL;
    }

    BYTE bReson = 0;
    newPt = FindMultSequence(pFrame->SequenceNumber,bReson);
    if (bReson == FR_NEW)
    {
        // 追加的方式
        newPt = (m_mult_endPt + 1) %MAX_MSG_SEQ_NUM;
        m_mult_endPt = newPt;
    }
    else if ((bReson != FR_NEAR) || (newPt >= MAX_MSG_SEQ_NUM))
    {
        dcf_output("[%s]%s recv multi is error(seq:%d,reson:%d,idx:%d)...\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,bReson,newPt);
        return DCF_ERR_FULL;
    }
    else
    {
        // 中间插入一个节点,采用从后往前搬移的方法
        WORD wCur = (m_mult_endPt + 1)%MAX_MSG_SEQ_NUM;
        WORD wPre = m_mult_endPt;
        m_mult_endPt = wCur;
        for(;wCur != newPt;)
        {
            m_sequence_multi_list[wCur] = m_sequence_multi_list[wPre];
            wCur = wPre;
            wPre = wPre?(wPre - 1):(MAX_MSG_SEQ_NUM-1);
        }
        m_sequence_multi_list[newPt] = NULL;
    }

    /* 此时该节点一定是空的 */
            ASSERT(m_sequence_multi_list[newPt] == NULL);
    FrameRecv *pNode = CopyNode(pMsg,msgLen);
    m_sequence_multi_list[newPt] = pNode;
    // 将数据保存到节点中去
    Multi_Save_Frame(pNode,pMsg);
    pNode->Frame_curBegin = 1;   /* 都是第一帧到之后才调用该函数 */
    // 调整头部位置(第1次0位置不会被使用)
    AdjustHeadPtr();
    Multi_Frame_Ack(pNode);
    return 0;
}
/****************************************************************
*功能描述 : 将数据保存到节点中去
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  18:21:20
****************************************************************/
DWORD CMsgRecver::Multi_Save_Frame(FrameRecv *pNode,BYTE *pMsg)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    WORD wCurFrameIdx = 0;
    if (!pFrame->IsFirstFrame())
    {
        wCurFrameIdx = pFrame->CurFrames;
        if ((wCurFrameIdx + 1) == pNode->totalFrame)
        {
            /* 收到最后一帧 */
            pNode->LastFrameLen = pHeader->PackageLen - RCC_SIZE_FRAME;
#if DBP_DTFRAME
            dcf_output("[%s]%s recv lastframe len:%d\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNode->LastFrameLen);
#endif
        }
    }
    if ((wCurFrameIdx >= pNode->totalFrame)
        || ((pHeader->PackageLen - RCC_SIZE_FRAME) > pNode->PacketLen))
    {
        dcf_output("%s error packet!\r\n",m_channel.print_ownapp);
        return DCF_ERR_INVALID_DATA;
    }

#if DBP_DTFRAME
    if (wCurFrameIdx)
    {
        dcf_output("[%s]%s recv muti next frame (curseq:%d,cf:%d),save ok!(cb:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pFrame->CurFrames,pNode->Frame_curBegin);
    }
#endif
    DWORD dwWritePt = wCurFrameIdx * pNode->PacketLen;
            ASSERT(pNode->pRecvMsg != NULL);
    memcpy(pNode->pRecvMsg + dwWritePt,pMsg+RCC_SIZE_RAW_HEAD,pHeader->PackageLen - RCC_SIZE_FRAME);
    return 0;
}

/****************************************************************
*功能描述 : 对总表的维护
*输入参数 : curSeq:当前收到的序号，为0表示在定时器中进行周期尝试,否则表示收到报文的第一包之后调用
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-04  9:10:20
****************************************************************/
void CMsgRecver::TryMoveTotalWnd(WORD curSeq)
{
    if (curSeq)
    {
        if (m_seq_sure_total->wnd_head() == curSeq)
        {
            /* 不是头部，不用尝试了 */
            return;
        }

        /* 是当前的头部 */
    }

    WORD move_nums = m_seq_sure_total->wnd_can_move_nums();
    if (!move_nums)
    {
        return;
    }

    m_seq_sure_total->wnd_move(move_nums);
}

/****************************************************************
*功能描述 : 完成一个节点的数据接收
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  23:21:47
****************************************************************/
void CMsgRecver::FinishNode(WORD Pt)
{
            ASSERT(Pt < MAX_MSG_SEQ_NUM);
            ASSERT(m_sequence_multi_list[Pt] != NULL);
    /* 先把本节点资源释放掉 */
    FrameRecv *pDelNode = m_sequence_multi_list[Pt];
    m_sequence_multi_list[Pt] = NULL;
    FreeNode(pDelNode);
    dcf_mem_free((void*&)pDelNode);

    // 将后面的往前面移动
    while(Pt != m_mult_endPt)
    {
        WORD nextPt = (Pt + 1)%MAX_MSG_SEQ_NUM;
        m_sequence_multi_list[Pt] = m_sequence_multi_list[nextPt];
        Pt = nextPt;
    }

    m_sequence_multi_list[Pt] = NULL;
    if (m_mult_endPt != m_mult_beginPt)
    {
        // 当前至少不只一个节点
        if (m_mult_endPt)
            m_mult_endPt--;
        else
            m_mult_endPt = MAX_MSG_SEQ_NUM - 1;
    }
}

/****************************************************************
*功能描述 : 发送应答帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  18:21:1
****************************************************************/
void CMsgRecver::SendFrameAck(BYTE *pMsg,bool bReject)
{
    CRccHeader *pHeader = (CRccHeader *)pMsg;
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);

    BYTE msgBuffer[RCC_SIZE_RAW_HEAD];
    memset(msgBuffer,0,sizeof(msgBuffer));
    CRawMsgHelper::InitMsgBuffer(msgBuffer,RCC_CTRL_FLAG_RECV_SURE,RCC_SIZE_RAW_HEAD);
    CRccFrame *pNewFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(msgBuffer);
    pNewFrame->SequenceNumber = pFrame->SequenceNumber;
    if (pFrame->IsFirstFrame())
    {
        pNewFrame->CurFrames =  1;
    }
    else
    {
        pNewFrame->CurFrames = pFrame->CurFrames + 1;
    }

    if (bReject)
    {
        pNewFrame->ctrl_set(RCC_CTRL_BIT_ACK_SEQ,1);
        pNewFrame->CurFrames = RCC_FRAME_REJECT;
    }

    pNewFrame->cirtifyid = m_channel.cirtifyid;
    pNewFrame->wMsg1 = pFrame->wMsg1;
    pNewFrame->wMsg2 = g_swnd_size_table_rtt[wnd_table_size - 1];
    pNewFrame->ctrl_set(RCC_CTRL_BIT_SURE,1);
    pNewFrame->ctrl_set(RCC_CTRL_BIT_CHANNEL,0);
#if DBP_DTFRAME
    dcf_output("[%s]%s recv send ack frame(seq:%d,cf:%d)!!!! SendFrameAck\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pFrame->SequenceNumber,pNewFrame->CurFrames);
#endif
    DWORD dwRet = m_channel.Sender(msgBuffer,RCC_SIZE_RAW_HEAD,m_channel.dstIP,m_channel.dstPort, RCC_MSG_TYPE_CHANNEL,1,m_channel.pThis);
    dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"sender");
}

/****************************************************************
*功能描述 : 通知已经收到的帧序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-19  11:32:7
****************************************************************/
void CMsgRecver::Notify_AckSeq(WORD seq)
{
    if (!seq)
    {
        WORD bitnums = m_seq_sure_total->wnd_can_move_nums();
        if (!bitnums)
        {
            return;
        }
        seq = m_seq_sure_total->wnd_head() + bitnums;
    }

#if DBP_DTFRAME
    dcf_output("[%s]%s recv timer notify begin seq(%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,seq);
#endif

    BYTE msgBuffer[RCC_SIZE_RAW_HEAD];
    memset(msgBuffer,0,sizeof(msgBuffer));
    CRawMsgHelper::InitMsgBuffer(msgBuffer,RCC_CTRL_FLAG_ACTSEQ,RCC_SIZE_RAW_HEAD);
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(msgBuffer);
    pFrame->SequenceNumber = seq;
    pFrame->wMsg2 = g_swnd_size_table_rtt[wnd_table_size - 1];
    pFrame->cirtifyid = m_channel.cirtifyid;

    pFrame->ctrl_set(RCC_CTRL_BIT_SURE,1);
    pFrame->ctrl_set(RCC_CTRL_BIT_CHANNEL,0);
    m_channel.Sender(msgBuffer,RCC_SIZE_RAW_HEAD,m_channel.dstIP,m_channel.dstPort, RCC_MSG_TYPE_CHANNEL,1,m_channel.pThis);
}

/****************************************************************
*功能描述 : 多包应答
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  22:37:20
****************************************************************/
void CMsgRecver::Multi_Frame_Ack(FrameRecv *pNode)
{
    // if (pNode->Frame_curBegin
    AskSenderFrame(pNode->SequenceNumber,pNode->Frame_curBegin);
}


/****************************************************************
*功能描述 : 主动请求发送对应包的第n帧
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-19  22:34:8
****************************************************************/
void CMsgRecver::AskSenderFrame(WORD seq,WORD ask_frame)
{
    BYTE msgBuffer[RCC_SIZE_RAW_HEAD];
    memset(msgBuffer,0,sizeof(msgBuffer));
    CRawMsgHelper::InitMsgBuffer(msgBuffer,RCC_CTRL_FLAG_ACK,RCC_SIZE_RAW_HEAD);
    CRccFrame *pNewFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(msgBuffer);
    pNewFrame->SequenceNumber = seq;
    pNewFrame->CurFrames = ask_frame;
    pNewFrame->cirtifyid = m_channel.cirtifyid;
    pNewFrame->wMsg2 = g_swnd_size_table_rtt[wnd_table_size - 1];
    if (!ask_frame)
    {
        // pNewFrame->ctrl_set(RCC_CTRL_BIT_WANT,1);
    }

    pNewFrame->ctrl_set(RCC_CTRL_BIT_SURE,1);
    pNewFrame->ctrl_set(RCC_CTRL_BIT_CHANNEL,0);
#if DBP_DTFRAME
    dcf_output("[%s]%s recv send ack frame(seq:%d,cf:%d)\r\n",dcf_get_time_string_unsafe(),m_channel.print_ownapp,pNewFrame->SequenceNumber,pNewFrame->CurFrames);
#endif
    DWORD dwRet = m_channel.Sender(msgBuffer,RCC_SIZE_RAW_HEAD,m_channel.dstIP,m_channel.dstPort, RCC_MSG_TYPE_CHANNEL,1,m_channel.pThis);
    dcf_sys_checkerr(dwRet,LEVEL_LOW_ERROR,1,"sender");
}

/****************************************************************
*功能描述 : 发送方定时发送seq窗口给收方
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-13  22:14:10
****************************************************************/
void CMsgRecver::SureSenderSeqWnd(BYTE *pMsg)
{
    CRccFrame *pFrame = (CRccFrame *)CRccFrameHelper::GetBufHeader(pMsg);
    WORD wBeginSeq = pFrame->wMsg1;
    WORD wEndSeq = pFrame->wMsg2;

    WORD wSelfBeginSeq_t = m_seq_sure_total->wnd_head();
    WORD wSelfendSeq_t = m_seq_sure_total->wnd_tail();

#if DBP_DTFRAME
    dcf_output("[%s]%s recv sender seq range(bs:%d,be:%d),(bs:%d,be:%d)\r\n",
                            dcf_get_time_string_unsafe(),m_channel.print_ownapp,wBeginSeq,wEndSeq,wSelfBeginSeq_t,wSelfendSeq_t);
#endif
    WND_CMP cwr;
    cwr = m_seq_sure_total->wnd_compare(wBeginSeq);
    if ((cwr == WCR_OLD) || (wBeginSeq == wSelfBeginSeq_t))
    {
        return;
    }

    WORD nums = m_seq_sure_total->idx_size(wBeginSeq);
    WORD canmovenum = m_seq_sure_total->wnd_can_move_nums();
    m_seq_sure_total->wnd_move( nums);

#if DBP_DTFRAME
    wSelfBeginSeq_t = m_seq_sure_total->wnd_head();
    wSelfendSeq_t = m_seq_sure_total->wnd_tail();
    dcf_output("[%s]%s recv sender seq range result:(bs:%d,be:%d)\r\n",
                            dcf_get_time_string_unsafe(),m_channel.print_ownapp,wSelfBeginSeq_t,wSelfendSeq_t);
#endif

}


