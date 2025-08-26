
#ifndef _DCF_BITS_H
#define _DCF_BITS_H
/****************************************************************
*文件范围 : 本文件用于字节流的bit位操作,得到的结果类似打印出来的效果
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  18:11:17
****************************************************************/

#include "dcf_def.h"

class dcf_bittools
{
public:
    /* 获取指定位置的bit位值 */
    static BYTE get_bit(BYTE *bs,WORD bytes,WORD bit);
    /* 设置bit流的值 */
    static void set_bit(BYTE *bs,WORD bytes,WORD bit,BYTE value);
    /* 从头部统计连续为0/1的数量 */
    static WORD con_nums_from_head(BYTE *bs,WORD bytes,BYTE value);
    /* 从头部开始，统计指定长度为指定值的数量 */
    static WORD count_from_head(BYTE *bs,WORD tobit,BYTE value);
    /* 左移动 */
    static void left_move(BYTE *bs,WORD bytes,WORD bitnums);
    /* 右移动 (需要的时候再增加)*/
    // static void right_move(BYTE *bs,WORD nums,WORD bitnums);

};

enum WND_CMP
{
    WCR_OLD = -1,
    WCR_IN_RANGE = 0,
    WCR_IN_WND = 1,
    WCR_OUT_WND = 2
};
/* 在0~65535范围内的绕接窗口控制器 idx对应为序号*/
class CFSWndCtrl
{
public:
    /* 业务接口 */
    // 和窗口比较 -1:在窗口左边 0:在窗口内(end内) 1:在最大窗口内(但在end外) 2:在最大窗口外
    WND_CMP wnd_compare(WORD idx);
    // 下面业务接口均带有compare_wnd比较的结果，目的是为了少重复的比较操作
    bool IsAcked(WORD idx,WND_CMP cwr);
    void wnd_save(WORD idx,WND_CMP cwr);
    WORD wnd_can_move_nums();
    void wnd_move(WORD nums);
    WORD wnd_head(){return m_begin_idx;}
    WORD wnd_tail(){return m_end_idx;}
    WORD range_size(){return get_range_size();}
    WORD idx_size(WORD idx) { return idx_to_pt(idx); };
protected:
    WORD idx_to_pt(WORD idx);
    WORD get_range_size();
public:
    // wndSize为窗口大小
    static CFSWndCtrl *GetWndCtrl(WORD wndSize);
    static void FreeWndCtrl(CFSWndCtrl *&p);
protected:
    CFSWndCtrl(){};
    ~CFSWndCtrl(){};
protected:
    WORD m_begin_idx;
    WORD m_end_idx;
    WORD m_wnd_bitsize;
    BYTE    m_flag_bits[2];
};

/* 序号老化表 */
class CFsAgeingTable
{
public:
    /* 由外部保证idx是顺序增长的 */
    void push_idx(WORD idx);
    void increase(WORD init_id,WORD &old_idx);
public:
    /* 老化的秒数 */
    static CFsAgeingTable*GetTable(WORD table_size);
    static void FreeTable(CFsAgeingTable *&p);
protected:
    CFsAgeingTable(){};
    ~CFsAgeingTable(){};
protected:
    WORD m_ageing_size;
    WORD m_cursor;  /* 从左到右老化 cur+1为老化序号*/
    WORD m_idxs[2]; /* 实际的大小为m_ageing_size+2 */
};
#endif

