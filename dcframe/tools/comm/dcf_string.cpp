
#include "dcf_def.h"
#include "dcf_string.h"
#include "extern_api.h"
#include <stdarg.h>

char *dcf_strtools::replace_char(char *pstr,char srcchar,char dstchar)
{
    char *pcur = pstr;
    while(*pcur)
    {
        if (*pcur == srcchar) *pcur = dstchar;
        pcur++;
    }
    return pstr;
}

/*
函 数 名：从原字符串流中拷贝指定长度的字符串
参    数：
返回参数: 
作    者：zjb
时    间：2017-4-25
*/
const char* dcf_strtools::get_string(const char*src_str, char*dst_str, int ifromPt_src, int iLen_src, int ibufferLen_dst)
{
    int icopy_len = (iLen_src > ibufferLen_dst) ? ibufferLen_dst : iLen_src;
    if (icopy_len)
    {
        strncpy(dst_str, src_str + ifromPt_src, icopy_len);
    }
    if (icopy_len < ibufferLen_dst)
    {
        // 小于缓冲区大小，可以填写0;大于缓冲区，则优先保证内容
        dst_str[icopy_len] = 0;
    }

    return dst_str;
}
/*
函 数 名：字符串中搜索引号(可以跳过转义引号)
参    数：pstr字符串源
返回参数: 引号位置
作    者：zjb
时    间：2017-4-25
*/
const char* dcf_strtools::search_quote(const char *pstr)
{
    while ((*pstr) && (*pstr != '\"') && (*pstr != '\''))
    {
        if ((*pstr == '\\') && (((*pstr + 1) == '\"') || (*(pstr + 1) == '\'')))
        {
            pstr += 2;
        }
        else
        {
            pstr++;
        }
    }
    return pstr;
}

/*
函 数 名：以分解符来切割字符串,返回停止的位置
参        数：pstr字符串源
                          chsplit切割字符
                          ptr_pos_array位置数组
                          strlen_array长度数组
                          size_array数组大小
输出参数：array_num实际搜索到的字符串数量
返回参数: 搜索结束的位置，一般来说，停止非0字符处，一般是数组满了，还可以++之后再查找
作    者：zjb
时    间：2017-4-25
*/
const char* dcf_strtools::dcf_split_string(const char *pstr, char chsplit, int *ptr_pos_array, int *strlen_array, int size_array, int *array_num)
{
    int inums = 0;

    int iPos = 0;

    if (!pstr || (!*pstr))
    {
        return NULL;
    }

    if (size_array)
    {
        memset(ptr_pos_array, 0, sizeof(int)*size_array);
        memset(strlen_array, 0, sizeof(int)*size_array);
    }

    // 先跳过空格
    const char *pCur = skip_space(pstr);
    const char *pCurBegin = pCur;
    const char *pCurEnd = pCurBegin;

    while (*pCur)
    {
        if (*pCur == chsplit)
        {
            // 找到了分界符号
            // 1.计算当前的结束
            if ((inums >= size_array) || (!ptr_pos_array) || (!strlen_array))
            {
                break;
            }

            // 记录数据
            ptr_pos_array[inums] = (int)(pCurBegin - pstr);
            strlen_array[inums] = (int)(pCurEnd - pCurBegin);
            inums++;
            if (inums >= size_array)
            {
                // 下一个来了也记录不下，返回
                break;
            }

            // 预处理下一个的空格等
            pCur++;
            pCur = skip_space(pCur);
            pCurBegin = pCur;
            pCurEnd = pCur;
            continue;
        }

        if (*pCur == ' ')
        {
            // 过滤一下空格
            // 先记录一下空格前的最后一个字符位置
            pCurEnd = pCur;
            pCur = skip_space(pCur);
            continue;
        }
        else if ((*pCur == '\\') && ((*(pCur + 1) == '\"') || (*(pCur + 1) == '\'')))
        {
            // 是转义引号，跳过去
            pCur += 2;
        }
        else if ((*pCur == '\"') || (*pCur == '\''))
        {
            pCur++;
            pCur = search_quote(pCur);
            if (*pCur)
            {
                // 数到尾巴了
                pCur++;
            }
        }
        else
        {
            pCur++;
        }

        pCurEnd = pCur;
    }

    // 填写最后一个
    if ((inums < size_array) && (ptr_pos_array) && (strlen_array))
    {
        ptr_pos_array[inums] = (int)(pCurBegin - pstr);
        strlen_array[inums] = (int)(pCurEnd - pCurBegin);
    }

    // 填写个数
    if (array_num)
    {
        *array_num = inums + 1;
        if (*array_num > size_array)
        {
            *array_num = size_array;
        }
    }

    return pCur;
}
/*
函 数 名：字符串转换为数字类型(可自动识别16进制)
参        数：pstr字符串源
                          defvalue:缺省值
输出参数：pEnd读取结束地方
返回参数: 转换之后的值
作    者：zjb
时    间：2017-4-27 15:28
*/

long dcf_strtools::strtol(const char *pstr,long defvalue,char **pEnd)
{
    long lvalue = 0;
    bool bHex = false;
    pstr = skip_space(pstr);

    if ((*pstr == '0') && ((*(pstr+1) == 'x') ||(*(pstr+1) == 'X')))
    {
        pstr += 2;
        bHex = true;
    }

    if (!(*pstr))
    {
        // 没有有效内容，填写缺省值
        lvalue = defvalue;
        goto clearn;
    }

    while(*pstr)
    {
        if ((*pstr >='0') && (*pstr <= '9'))
        {
            if (bHex)
            {
                lvalue = (lvalue << 4) + ((BYTE)*pstr - (BYTE)'0');
            }
            else
            {
                lvalue = lvalue * 10 + ((BYTE)*pstr - (BYTE)'0');
            }
        }
        else if ((bHex) && (*pstr >='a') && (*pstr <= 'f'))
        {
            lvalue = (lvalue <<4)  + ((BYTE)*pstr - (BYTE)'a') + 10;
        }
        else if ((bHex) && (*pstr >='A') && (*pstr <= 'F'))
        {
            lvalue = (lvalue <<4) + ((BYTE)*pstr - (BYTE)'A') + 10;
        }
        else
        {
            break;
        }
        pstr++;
    }
    clearn:
    if (pEnd)
    {
        *pEnd = (*pstr)?(char*)pstr:NULL;
    }
    return lvalue;
}

const char *dcf_strtools::filter_sqlnull_str(const char *pstr)
{
    if (!_stricmp(pstr,"NULL"))
    {
        return "";
    }
    return pstr;
}

char *dcf_strtools::strmalloc(const char *strorg,int limit_len,bool bIgnoreSqlNull)
{
    if (!strorg)
    {
        return NULL;
    }
    const char *pstr = strorg;
    if (bIgnoreSqlNull && (!_stricmp(pstr,"NULL")))
    {
        pstr = "";
    }
    const char *pCur = pstr;
    int iMallocLen = 0;
    while(*pCur) pCur++;
    iMallocLen = (int)(pCur - pstr + 1);
    if (limit_len > 0)
    {
        iMallocLen = (limit_len>iMallocLen)?iMallocLen:(limit_len +1);
    }

    char *pnew = (char*)dcf_mem_malloc(iMallocLen);
    memset(pnew,0,iMallocLen);
    memcpy(pnew,pstr,iMallocLen);
    return pnew;
}
/*
windows下的点一下回车，效果是：回车换行，就是\r\n
unix系统下的回车一下就是一个\n
*/
const char *dcf_strtools::skip_to_line_end(const char *pstr)
{
    while ((*pstr) && (*pstr != '\n')) pstr++;
    if (*pstr == '\n')  pstr++;
    return pstr;
}

const char *dcf_strtools::skip_note_line(const char *pstr)
{
    const char *pcur = pstr;
    const char *pcurline_begin = pcur;
    while(*pcur)
    {
        // 跳过空格
        pcur = skip_space(pcur);

        // 判断本行是否为空格
        if ((*pcur == ';') || ((*pcur == '/') && (*(pcur+1) == '/')))
        {
            // 本行是注释行调到行尾继续
            pcur = skip_to_line_end(pcur);
            // 当前行的开始位置
            pcurline_begin = pcur;
            continue;
        }
        else
        {
            // 本行不是注释行,跳出来
            break;
        }
    }
    return pcurline_begin;
}

/*
函 数 名：剔除文本中的注释行
参        数：src_str字符串源
输出参数: 无
返回参数: 无
作    者：zjb
时    间：2017-5-04 14:40
*/
void dcf_strtools::reject_note_line(char *src_str,DWORD *len)
{
    bool b_note_line = false;
    bool b_need_cpy = false;
    char *dst_str = src_str;
    DWORD    iline_len = 0;
    DWORD    itotal_len = 0;

    while(*src_str)
    {
        const char *p_del_begin = src_str;
        // 如果不是注释行,则p_line_start = src_str
        // 如果是注释行,则p_line_start 是非注释行的首地址
        const char *p_line_start =  skip_note_line(src_str);
        // 本有效行的下一行开始
        const char *p_line_end  =  skip_to_line_end(p_line_start);

        if (p_line_start != src_str)
        {
            b_need_cpy = true;
            // 需要拷贝了，第一个一定是0
            dst_str[0] = 0;
        }

        // 先把本行拷贝过去
        iline_len = (DWORD)(p_line_end - p_line_start);
        if ((b_need_cpy) && (iline_len))
        {
            memmove(dst_str,p_line_start,iline_len);
            dst_str += iline_len;
            dst_str[0] = 0;
        }
        else
        {
            dst_str += iline_len;
        }

        itotal_len += iline_len;
        // 再处理指针
        src_str = (char*)p_line_end;
    }

    if (len)
    {
        *len = itotal_len;
    }
}

char *dcf_strtools::strcat(char *pbuffer,char *psrc,int &append_pt,int buffer_len)
{
    while(*psrc)
    {
        if (buffer_len && ((append_pt + 1) >=buffer_len))
        {
            break;
        }
        pbuffer[append_pt++] = *psrc;
        psrc++;
    }

    pbuffer[append_pt] = 0;
    if (*psrc)
    {
        return NULL;
    }

    return pbuffer;
}


int dcf_strtools::strcpy_s(char *pbuffer,const char *psrc,int buffer_len)
{
    if (buffer_len <= 0)
    {
        return 0;
    }
    const char *pEnd  = pbuffer + (buffer_len - 1);
    char *pdst = pbuffer;
    bool bfillzero = false;
    while(pdst != pEnd)
    {
        if (bfillzero)
        {
            *pdst = 0;
        }
        else
        {
            *pdst = *psrc;
            if (!(*psrc))
            {
                bfillzero = true;
            }
        }

        pdst++;
        psrc++;
    }
    *pdst = 0;
    return (int)(pdst- pbuffer) + 1;
}

const char *dcf_strtools::get_short_filename(const char *pfilename)
{
    const char *pret = pfilename;
    const char *pcur = pfilename;
    while(*pcur)
    {
        if ((*pcur == '/') ||(*pcur == '\\') )
        {
            pret = pcur + 1;
        }
        pcur ++;
    }
    return pret;
}

int dcf_strtools::datetime_compare(const char* time1, const char* time2)
{
    if (NULL == time1 || NULL == time2) return -1;

    int year1, month1, day1, hour1, min1, sec1;
    int year2, month2, day2, hour2, min2, sec2;
    sscanf(time1, "%d-%d-%d %d:%d:%d", &year1, &month1, &day1, &hour1, &min1, &sec1);
    sscanf(time2, "%d-%d-%d %d:%d:%d", &year2, &month2, &day2, &hour2, &min2, &sec2);

    // 判断日期
    int tm1 = year1 * 10000 + month1 * 100 + day1;
    int tm2 = year2 * 10000 + month2 * 100 + day2;
    if (tm1 != tm2) return (tm1 > tm2) ? 1 : -1;

    // 判断时间
    tm1 = hour1 * 3600 + min1 * 60 + sec1;
    tm2 = hour2 * 3600 + min2 * 60 + sec2;
    if (tm1 != tm2) return (tm1 > tm2) ? 1 : -1;

    return 0;
}

DWORD dcf_strtools::date_strtol(char chbuffer[32])
{
    char *p = chbuffer;
    DWORD v = 0;
    int inums = 0,ifirst = 0;
    for(;*p;p++)
    {
        BYTE ch = (BYTE)*p;
        if (ch == ' ')
        {
            if ((inums >= 2) && (ifirst))
            {
                break;
            }
            continue;
        }

        if (ch == '-')
        {
            ifirst = 0;
            inums++;

            if (inums > 2)
            {
                /* 应该是不会走到这里来的 */
                break;
            }
            continue;
        }

        if (ch == ':')
        {
            break;
        }

        ifirst++;
        if ((ch >= '0') && (ch <= '9'))
        {
            v = v * 10 + (ch - '0');
        }
        else
        {
            break;
        }
    }

    return v;
}
void dcf_strtools::date_ltostr(DWORD v,char chbuffer[32])
{
    WORD vt_d = v % 100;
    v = v /100;
    WORD vt_m = v % 100;
    v = v /100;
    WORD vt_y = v % 10000;
    sprintf_s(chbuffer,32,"%4d-%02d-%02d 00:00:00",vt_y,vt_m,vt_d);
}

uint64 dcf_strtools::uint64_strtov(DWORD no_t,char chbuffer[32])
{
    return uint64_combine(date_strtol(chbuffer),no_t);
}
void dcf_strtools::uint64_vtostr(uint64 v,DWORD &no_t,char chbuffer[32])
{
    DWORD date_v =0;
    uint64_split(v,date_v,no_t);
    date_ltostr(date_v,chbuffer);
}

uint64 dcf_strtools::uint64_combine(DWORD dt_v,DWORD idx_v)
{
    /* 2017-08-21  16:23:1 奶奶的，int64类型还不能用位计算，先用简单的乘法吧 */
    uint64 v = 0;
    const uint64 beishu = 10000000000;
    v = dt_v;
    v = (v * beishu) + idx_v;
    return v;
}

void dcf_strtools::uint64_split(uint64 v,DWORD &dt_v,DWORD &idx_v)
{
    const uint64 beishu = 10000000000;
    dt_v =  (DWORD)(v / beishu);
    idx_v = (DWORD)(v % beishu);
}

DWORD dcf_strtools::uint_strtov(DWORD idx_t,char chbuffer[32])
{
    const DWORD beishu = 10000;
    DWORD dt_v = date_strtol(chbuffer) - (DCF_YEAR_BEGIN * beishu);
    return uint_combine(dt_v,idx_t);
}

void dcf_strtools::uint_vtostr(DWORD v,DWORD &idx_t,char chbuffer[32])
{
    DWORD date_v =0;
    uint_split(v,date_v,idx_t);
    const DWORD beishu = 10000;
    date_v = date_v + (DCF_YEAR_BEGIN * beishu);
    date_ltostr(date_v,chbuffer);
}

DWORD dcf_strtools::uint_combine(DWORD dt_v,DWORD idx_v)
{
    DWORD v = 0;
    const DWORD beishu = 10000;
    v = dt_v - (DCF_YEAR_BEGIN * beishu);
    v = (v * beishu) + idx_v%beishu;
    return v;
}

void dcf_strtools::uint_split(DWORD v,DWORD &dt_v,DWORD &idx_v)
{
    const DWORD beishu = 10000;
    /* 系统上线的最早时间 2016-8-24 */
    if (v < 108240000)
    {
        v = 108240000;
    }

    dt_v =  (DWORD)(v / beishu) + (DCF_YEAR_BEGIN * beishu);
    idx_v = (DWORD)(v % beishu);
}



dcf_strsearch::dcf_strsearch(const char*pstr, char chsplit)
{
    m_pstr_org = pstr;
    m_pstr_cur = pstr;
    m_chsplit = chsplit;
}

const char *dcf_strsearch::get_first(int *pstrlen)
{
    m_pstr_cur = m_pstr_org;
    if ((!m_pstr_cur) || (!(*m_pstr_cur)))
    {
        return NULL;
    }
    return find(pstrlen);
}

const char *dcf_strsearch::get_next(int *pstrlen)
{
    if ((!m_pstr_cur) || (!(*m_pstr_cur)))
    {
        return NULL;
    }

    m_pstr_cur++;
    return find(pstrlen);
}
/*
函    数   名:     查找下一个分解符
参             数:    
输出参数:      pstrlen字符串长度
返回参数:      当前字符串
作    者：zjb
时    间：2017-4-26
*/

const char *dcf_strsearch::find(int *pstrlen)
{
    int iFromPtr = 0, iStrLen = 0,iNums = 0;
    const char *pnext = dcf_strtools::dcf_split_string(m_pstr_cur, m_chsplit, &iFromPtr, &iStrLen, 1, &iNums);
    if (iNums <= 0)
    {
        return NULL;
    }

    if (pstrlen)
    {
        *pstrlen = iStrLen;
    }

    m_chbuffer[0] = 0;
    if ((iStrLen <= 0) || (iFromPtr < 0))
    {
        m_pstr_cur = pnext;
        return (const char*)m_chbuffer;
    }

    (void)dcf_strtools::get_string(m_pstr_cur, m_chbuffer,iFromPtr,iStrLen, MAX_ITEM_LEN + 1);
    m_pstr_cur = pnext;
    return (const char*)m_chbuffer;
}

dcfstring::dcfstring()
{
    m_pbuffer = m_fixbuffer;
    m_fixbuffer[0] = 0;
    m_strlen = 0;
}

dcfstring::dcfstring(char *p,...)
{
    m_pbuffer = m_fixbuffer;
    m_fixbuffer[0] = 0;
    m_strlen = 0;
    va_list marker;
    char *pstr = p;
    va_start( marker,p );
    int iTotallen = 0;
    while(pstr != NULL)
    {
        iTotallen += (DWORD)strlen(pstr);
        pstr = va_arg(marker,char*);
    }

    iTotallen++;
    if (iTotallen > sizeof(m_fixbuffer))
    {
        // 超过缺省缓存长度
        m_pbuffer = (char*)dcf_mem_malloc(iTotallen);
    }

    m_pbuffer[0] = 0;

    // 拷贝字符串
    pstr = p;
    va_start( marker ,p);
    int iPt = 0;
    while(pstr != NULL)
    {
        if (dcf_strtools::strcat(m_pbuffer,pstr,iPt,iTotallen) == NULL)
        {
            // 出错了
            dcf_output("写入字符串错误...");
            break;
        }

        pstr = va_arg(marker,char*);
    }

    va_end( marker );

    m_strlen = iPt;
}

dcfstring::~dcfstring()
{
#ifdef TEST_DEBUG
    dcf_output("~dcfstring:0x%x->%s\r\n", m_pbuffer, m_pbuffer);
#endif
    if (m_pbuffer != m_fixbuffer)
    {
        // 是新申请的内存
        dcf_mem_free((void*&)m_pbuffer);
    }
}

char *dcfstring::detach()
{
    char *pret = m_pbuffer;
    if (m_pbuffer == m_fixbuffer)
    {
        // 需要新申请内存
        pret = (char*)dcf_mem_malloc(m_strlen + 1);
        pret[0] = 0;
        memmove(pret,m_pbuffer,m_strlen + 1);
    }

    m_pbuffer = m_fixbuffer;
    m_pbuffer[0] = 0;
    m_strlen = 0;
    return pret;
}

void dcfstring::release()
{
    if (m_pbuffer != m_fixbuffer)
    {
        // 是新申请的内存
        dcf_mem_free((void*&)m_pbuffer);
    }
    m_pbuffer = m_fixbuffer;
    m_pbuffer[0] = 0;
    m_strlen = 0;
}

dcfstring::operator char*()
{
    return m_pbuffer;
}

dcfstring &dcfstring::operator +(char *pstr)
{
    if (!pstr)
    {
        return *this;
    }

    int inewlen = (DWORD)strlen(pstr);

    if ((m_pbuffer != m_fixbuffer ) || ((m_strlen + inewlen + 1) > sizeof(m_fixbuffer)))
    {
        // 需要新申请内存了
        char *pret = m_pbuffer;
        m_pbuffer = (char*)dcf_mem_malloc(m_strlen + inewlen + 1);
        if (m_strlen)
        {
            memmove(m_pbuffer,pret,m_strlen);
        }

        if (pret != m_fixbuffer )
        {
            // 需要释放以前申请的内存
            dcf_mem_free((void*&)pret);
        }
    }

    // 把新的拷贝进去
    memmove(m_pbuffer + m_strlen,pstr,inewlen);
    m_strlen += inewlen;
    m_pbuffer[m_strlen] = 0;
    return *this;
}

/****************************************************************
*功能描述 : IP地址的2个转换函数
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-18  17:51:37
****************************************************************/
char * dcf_strtools::IPtoStr(DWORD dwIP,char chbuffer[16])
{
    sprintf_s(chbuffer,16,"%u.%u.%u.%u",(dwIP&0xff000000)>>24,(dwIP&0xff0000)>>16,(dwIP&0xff00)>>8,dwIP&0xff);
    return chbuffer;
}

DWORD dcf_strtools::StrtoIP(char *p,char **pEnd)
{
    DWORD dwIP = 0;
    BYTE bValue = 0;
    while(*p)
    {
        BYTE ch = *p;
        if ((ch >= '0') && (ch <= '9'))
        {
            bValue = bValue *10 + (ch - '0');
        }
        else if (ch == '.')
        {
            dwIP = (dwIP << 8) + bValue;
            bValue = 0;
        }
        else if (ch != ' ')
        {
            break;
        }
        p++;
    }

    // 还有最后一个
    dwIP = (dwIP << 8) + bValue;

    if (pEnd)
    {
        *pEnd = p;
    }
    return dwIP;
}

char *dcf_tools_get_shortname(const char *pfilename)
{
    return (char*)dcf_strtools::get_short_filename(pfilename);
}



