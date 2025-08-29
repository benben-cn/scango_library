/****************************************************************
*文件范围 : bit位操作类
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  18:18:58
****************************************************************/

#include "dcf_bits.h"
#include "extern_api.h"

/****************************************************************
*功能描述 : 获取指定位的bit
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  18:20:32
****************************************************************/
BYTE dcf_bittools::get_bit(BYTE *bs,WORD bytes,WORD bit)
{
    WORD wPt = bit/8;
    if (wPt > bytes)
    {
                ASSERT(0);
        return 0;
    }
    bit = bit - wPt*8;
    return (bs[wPt]&(1<<bit))?1:0;
}
/****************************************************************
*功能描述 : 设置对应bit位置的值
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-16  18:25:4
****************************************************************/
void dcf_bittools::set_bit(BYTE *bs,WORD bytes,WORD bit,BYTE value)
{
    WORD wPt = bit/8;
    if (wPt > bytes)
    {
                ASSERT(0);
        return;
    }

    bit = bit - wPt*8;
    if (value)
    {
        bs[wPt] |= (1<<bit);
    }
    else
    {
        bs[wPt] &= ~(1<<bit);
    }
}

/****************************************************************
*功能描述 : 向左移动指定数量的bit位
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  18:25:4
****************************************************************/
void dcf_bittools::left_move(BYTE *bs,WORD bytes,WORD bitnums)
{
    if (bitnums >= (bytes * 8))
    {
        // 全部清除
        memset(bs,0,bytes);
        return;
    }

    // 先把整个的移动拿掉
    WORD movebytes = bitnums/8;
    WORD i,j;
    if (movebytes)
    {
        j = 0;
        // 搬移
        for(i = movebytes;i < bytes;i++,j++)
        {
            bs[j] = bs[i];
        }

        // 清理掉后面的
        for(;j<bytes;j++)
        {
            bs[j] = 0;
        }
    }

    // 下面就是复杂的字节流处理了
    BYTE movebit = (BYTE)(bitnums - (movebytes*8));
    WORD leftbytes = bytes - movebytes;
    if (!movebit)
    {
        return;
    }

    for(i = 0;i < leftbytes - 1;i++)
    {
        BYTE b = bs[i];
        b = b >> movebit;
        // 取后面字符的8-movebit位
        BYTE bnext = bs[i+1];
        bnext = bnext <<(8-movebit);
        bs[i] = b|bnext;
    }
}
/****************************************************************
*功能描述 : 从头部开始统计连续为对应值个数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-16  23:12:53
****************************************************************/
WORD dcf_bittools::con_nums_from_head(BYTE *bs,WORD bytes,BYTE value)
{
    value = value?1:0;
    BYTE bCheck = value?0xff:0;
    WORD bitnums = 0;
    WORD i = 0;
    for(;i < bytes;i++,bs++)
    {
        if ((*bs) == bCheck)
        {
            bitnums += 8;
        }
        else
        {
            break;
        }
    }

    if (i >= bytes)
    {
        // 已经统计完了
        return bitnums;
    }

    // 统计当前字节的
    BYTE ch = *bs;
    for(i = 0;i < 8;i++)
    {
        if ((ch &0x1) == value)
        {
            bitnums++;
            ch >>= 1;
        }
        else
        {
            break;
        }
    }
    return bitnums;
}
/****************************************************************
*功能描述 : 统计指定范围内为指定值的bit数量
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-17  16:0:32
****************************************************************/
WORD dcf_bittools::count_from_head(BYTE *bs,WORD tobit,BYTE value)
{
    value = value?1:0;
    BYTE bCheck = value?0xff:0;
    WORD bitnums = 0;
    WORD i = 0;
    WORD search_bits = 0;
    while(search_bits < tobit)
    {
        BYTE b = *bs;
        bs++;
        if ((b == bCheck) && ((search_bits + 8) >= tobit))
        {
            bitnums += 8;
            search_bits += 8;
        }
        else
        {
            for(BYTE i = 0;((i < 8)&&(search_bits < tobit));i++,search_bits++)
            {
                if ((b &0x1) == value)
                {
                    bitnums++;
                }
                b >>= 1;
            }
        }
    }
    return bitnums;
}

const WORD MAX_SEQ = 0xffff;
const WORD MID_SEQ = 0xffff/2;

CFSWndCtrl *CFSWndCtrl::GetWndCtrl(WORD wndSize)
{
            ASSERT((((wndSize + 7)/8)*8) == wndSize);
    WORD bytes = sizeof(CFSWndCtrl) + wndSize/8 - sizeof(m_flag_bits);
    CFSWndCtrl *p = (CFSWndCtrl*)dcf_mem_malloc(bytes);
    memset(p,0,bytes);
    p->m_wnd_bitsize = wndSize;
    return p;
}

void CFSWndCtrl::FreeWndCtrl(CFSWndCtrl *&p)
{
    if (p)
    {
        dcf_mem_free((void*&)p);
        p = NULL;
    }
}

/****************************************************************
*功能描述 : 窗口比较
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  16:0:26
****************************************************************/
WND_CMP CFSWndCtrl::wnd_compare(WORD idx)
{
    if (!m_begin_idx)
    {
        /* 刚刚初始化，可以存放很多 */
        return WCR_IN_WND;
    }

    DWORD dwBeginIdx = m_begin_idx;
    DWORD dwWndMaxIdx = dwBeginIdx + m_wnd_bitsize;
    if ((idx == 0) || (idx == 1))
    {
        idx = idx;
    }
    if ((idx == m_begin_idx) || (idx == m_end_idx))
    {
        return WCR_IN_RANGE;
    }

    if (dwWndMaxIdx <= MAX_SEQ)
    {
        // 没有绕接
        if (idx < dwBeginIdx)
        {
            return WCR_OLD;
        }

        if (idx <= m_end_idx)
        {
            return WCR_IN_RANGE;
        }

        if (idx <= dwWndMaxIdx)
        {
            return WCR_IN_WND;
        }

        return WCR_OUT_WND;
    }

    // 窗口将会绕接
    dwWndMaxIdx = dwWndMaxIdx - MAX_SEQ + 1; // 0无需存放

            ASSERT(m_begin_idx > MID_SEQ);
    // 已经绕接的情形
    if (idx > MID_SEQ)
    {
        if (idx < m_begin_idx)
        {
            return WCR_OLD;
        }

        if (idx < m_end_idx)
        {
            return WCR_IN_RANGE;
        }

        return WCR_IN_WND;
    }

    if (idx > dwWndMaxIdx)
    {
        return WCR_OUT_WND;
    }

    if (idx > m_end_idx)
    {
        return WCR_IN_WND;
    }

    return WCR_IN_RANGE;
}
/****************************************************************
*功能描述 : 是否已经确认了
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  16:14:42
****************************************************************/
bool CFSWndCtrl::IsAcked(WORD idx,WND_CMP cwr)
{
    // 窗口过去的都是OK了
    if (cwr == WCR_OLD) return true;
    // 窗口当前范围外的都是非OK
    if (cwr != WCR_IN_RANGE) return false;
    // 下面都是窗口内的
    WORD pt = idx_to_pt(idx);
    if (pt > m_wnd_bitsize)
    {
        return false;
    }

    if (dcf_bittools::get_bit(m_flag_bits,m_wnd_bitsize/8,pt))
    {
        return true;
    }

    return false;
}
/****************************************************************
*功能描述 : 窗口内的索引对应位置
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-06-30  16:41:25
****************************************************************/
WORD CFSWndCtrl::idx_to_pt(WORD idx)
{
    if (!m_begin_idx)
    {
        /* 第一帧 */
        return 0;
    }

    if (m_begin_idx <= m_end_idx)
    {
        return idx - m_begin_idx;
    }

    if (idx >= m_begin_idx)
    {
        return idx - m_begin_idx;
    }

    return MAX_SEQ - m_begin_idx + idx;
}
/****************************************************************
*功能描述 : 获取范围内的节点数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  17:10:50
****************************************************************/
WORD CFSWndCtrl::get_range_size()
{
    if (m_begin_idx == m_end_idx)
    {
        if (m_flag_bits[0])
        {
            return 1;
        }
        return 0;
    }

    // 直接用最后节点来统计
    return idx_to_pt(m_end_idx) + 1;
}

/****************************************************************
*功能描述 : 保存到对应节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  16:41:25
****************************************************************/
void CFSWndCtrl::wnd_save(WORD idx,WND_CMP cwr)
{
            ASSERT((cwr == WCR_IN_RANGE) || (cwr == WCR_IN_WND));
    if (!m_begin_idx)
    {
        /* 第一帧用第一个到达的进行初始化，可能会至前期一些帧接收失败，但没有更好的办法 */
        m_begin_idx = idx;
        m_end_idx = idx;
    }
    WORD pt = idx_to_pt(idx);
            ASSERT(pt <= m_wnd_bitsize);
    dcf_bittools::set_bit(m_flag_bits,m_wnd_bitsize/8,pt,1);
    if (cwr == WCR_IN_WND)
    {
        m_end_idx = idx;
    }
}
/****************************************************************
*功能描述 : 统计可以移动的数量
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  16:56:59
****************************************************************/
WORD CFSWndCtrl::wnd_can_move_nums()
{
    return dcf_bittools::con_nums_from_head(m_flag_bits,m_wnd_bitsize/8,1);
}

/****************************************************************
*功能描述 : 统计可以移动的数量
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-06-30  16:56:59
****************************************************************/
void CFSWndCtrl::wnd_move(WORD nums)
{
    dcf_bittools::left_move(m_flag_bits,m_wnd_bitsize/8,nums);
    WORD size = get_range_size();
    m_begin_idx += nums;
    if (nums >= size)
    {
        m_end_idx = m_begin_idx;
    }
}

CFsAgeingTable*CFsAgeingTable::GetTable(WORD table_size)
{
    WORD wSize = sizeof(CFsAgeingTable)+sizeof(WORD)*table_size;
    CFsAgeingTable *p = (CFsAgeingTable*)dcf_mem_malloc(wSize);
    memset(p,0,wSize);
    p->m_ageing_size = table_size + 2;
    return p;
}
void CFsAgeingTable::FreeTable(CFsAgeingTable *&p)
{
    if (p)
    {
        dcf_mem_free((void*&)p);
        p = NULL;
    }
}

/****************************************************************
*功能描述 : 将当前收到的序号保存在老化表的当前位置
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-04  9:39:38
****************************************************************/
void CFsAgeingTable::push_idx(WORD idx)
{
    m_idxs[m_cursor] = idx;
}

/****************************************************************
*功能描述 : 滚动此前最老的收到的序号
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-04  9:40:44
****************************************************************/
void CFsAgeingTable::increase(WORD init_idx,WORD &old_idx)
{
    old_idx = m_idxs[(m_cursor+1)%m_ageing_size];
    m_cursor = (m_cursor + 1)%m_ageing_size;
    m_idxs[m_cursor] = init_idx;
}

