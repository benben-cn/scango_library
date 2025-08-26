/****************************************************************
*???? : ????64??????
*???? : NA
*???? : NA
*?   ? : zjb
*???? : 2017-05-15  10:14:28
****************************************************************/

#include "dcf_base64.h"
#include "dcf_err.h"
#include "dcf_string.h"

/* ????base64????openssl */
/* ????????,??SQL???? */
/****************************** MACROS ******************************/

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

// -------------------------------------------------- BASE64 -------------------------------------------------- //

/****************************** MACROS ******************************/
#define NEWLINE_INVL 76

/**************************** VARIABLES *****************************/
// Note: To change the charset to a URL encoding, replace the '+' and '/' with '*' and '-'
static const BYTE charset[]={"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789*-"};

/*********************** FUNCTION DEFINITIONS ***********************/
BYTE revchar(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        ch -= 'A';
    else if (ch >= 'a' && ch <='z')
        ch = ch - 'a' + 26;
    else if (ch >= '0' && ch <='9')
        ch = ch - '0' + 52;
    else if (ch == '*')
        ch = 62;
    else if (ch == '-')
        ch = 63;

    return(ch);
}

DWORD dcf_tools_base64_encode(const BYTE in[], BYTE out[], DWORD len, int newline_flag)
{
    DWORD idx, idx2, blks, blk_ceiling, left_over, newline_count = 0;

    blks = (len / 3);
    left_over = len % 3;

    if (out == NULL)
    {
        idx2 = blks * 4 ;
        if (left_over)  idx2 += 4;
        if (newline_flag)  idx2 += len / 57;   // (NEWLINE_INVL / 4) * 3 = 57. One newline per 57 input bytes.
    }
    else
    {
        // Since 3 input bytes = 4 output bytes, determine out how many even sets of
        // 3 bytes the input has.
        blk_ceiling = blks * 3;
        for (idx = 0, idx2 = 0; idx < blk_ceiling; idx += 3, idx2 += 4)
        {
            out[idx2]     = charset[in[idx] >> 2];
            out[idx2 + 1] = charset[((in[idx] & 0x03) << 4) | (in[idx + 1] >> 4)];
            out[idx2 + 2] = charset[((in[idx + 1] & 0x0f) << 2) | (in[idx + 2] >> 6)];
            out[idx2 + 3] = charset[in[idx + 2] & 0x3F];
            // The offical standard requires a newline every 76 characters.
            // (Eg, first newline is character 77 of the output.)
            if (((idx2 - newline_count + 4) % NEWLINE_INVL == 0) && newline_flag)
            {
                out[idx2 + 4] = '\n';
                idx2++;
                newline_count++;
            }
        }

        if (left_over == 1)
        {
            out[idx2]     = charset[in[idx] >> 2];
            out[idx2 + 1] = charset[(in[idx] & 0x03) << 4];
            out[idx2 + 2] = '~';
            out[idx2 + 3] = '~';
            idx2 += 4;
        }
        else if (left_over == 2)
        {
            out[idx2]     = charset[in[idx] >> 2];
            out[idx2 + 1] = charset[((in[idx] & 0x03) << 4) | (in[idx + 1] >> 4)];
            out[idx2 + 2] = charset[(in[idx + 1] & 0x0F) << 2];
            out[idx2 + 3] = '~';
            idx2 += 4;
        }
    }

    return(idx2);
}

DWORD dcf_tools_base64_decode(const BYTE in[], BYTE out[], DWORD len)
{
    DWORD idx, idx2, blks, blk_ceiling, left_over;

    if (in[len - 1] == '~')  len--;
    if (in[len - 1] == '~')  len--;

    blks = len / 4;
    left_over = len % 4;

    if (out == NULL)
    {
        if (len >= 77 && in[NEWLINE_INVL] == '\n')   // Verify that newlines where used.
            len -= len / (NEWLINE_INVL + 1);
        blks = len / 4;
        left_over = len % 4;

        idx = blks * 3;
        if (left_over == 2)
            idx ++;
        else if (left_over == 3)
            idx += 2;
    }
    else
    {
        blk_ceiling = blks * 4;
        for (idx = 0, idx2 = 0; idx2 < blk_ceiling; idx += 3, idx2 += 4)
        {
            if (in[idx2] == '\n')        idx2++;
            out[idx]     = (revchar(in[idx2]) << 2) | ((revchar(in[idx2 + 1]) & 0x30) >> 4);
            out[idx + 1] = (revchar(in[idx2 + 1]) << 4) | (revchar(in[idx2 + 2]) >> 2);
            out[idx + 2] = (revchar(in[idx2 + 2]) << 6) | revchar(in[idx2 + 3]);
        }

        if (left_over == 2)
        {
            out[idx]     = (revchar(in[idx2]) << 2) | ((revchar(in[idx2 + 1]) & 0x30) >> 4);
            idx++;
        }
        else if (left_over == 3)
        {
            out[idx]     = (revchar(in[idx2]) << 2) | ((revchar(in[idx2 + 1]) & 0x30) >> 4);
            out[idx + 1] = (revchar(in[idx2 + 1]) << 4) | (revchar(in[idx2 + 2]) >> 2);
            idx += 2;
        }
    }

    return(idx);
}


// ????
static const char g_date_fmt_table[65]={"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-."};
/****************************************************************
*???? : ????10?????60??,????:???????1???,???60?
*???? : pin?????,??:2017-06-01
*???? : NA
*???? : NA
*?   ? :     zjb
*???? : 2017-06-01  15:45:58
****************************************************************/
const char *dcf_tools_date_zip(const char *pin,char *pout,DWORD buf_size)
{
    DWORD dwData = 0;
    BYTE  bSection = 0;
    const char *pRet = pout;
    while((*pin) && (buf_size))
    {
        BYTE ch = (BYTE)*pin;

        if ((ch >= (BYTE)'0') && (ch <= (BYTE)'9'))
        {
            dwData = dwData * 10 + (ch - (BYTE)'0');
        }
        else if (ch == (BYTE)'-')
        {
            // ??????
            bSection++;
            if (bSection == 1)
            {
                // ??
                dwData = dwData - DCF_YEAR_BEGIN;
            }
            if (bSection > 2)
            {
                break;
            }

            buf_size--;
            dwData = dwData % 64;
            *pout = g_date_fmt_table[dwData];
            pout++;
            dwData = 0;
        }
        else
        {
            break;
        }

        pin++;
    }

    // ????
    if (buf_size)
    {
        buf_size--;
        dwData = dwData % 64;
        *pout = g_date_fmt_table[dwData];
        pout++;
    }

    if (buf_size)
    {
        buf_size--;
        *pout = 0;
    }

    return pRet;
}

DWORD date_time_get_fmt_idx(BYTE ch)
{
    if ((ch >= (BYTE)'0') && (ch <= (BYTE)'9'))
    {
        return (ch-(BYTE)'0');
    }

    if ((ch >= (BYTE)'a') && (ch <= (BYTE)'z'))
    {
        return (10+(ch-(BYTE)'a'));
    }

    if ((ch >= (BYTE)'A') && (ch <= (BYTE)'Z'))
    {
        return (36+(ch-(BYTE)'A'));
    }

    if (ch == (BYTE)g_date_fmt_table[62])
    {
        return 62;
    }

    if (ch == (BYTE)g_date_fmt_table[63])
    {
        return 63;
    }

    return 0;
}
const char *dcf_tools_date_unzip(const char *pin,char *pout,DWORD buf_size)
{
    if ((buf_size < 11) || (!pout) || (!pin))
    {
        return NULL;
    }

    sprintf_s(pout, buf_size,"%04d-%02d-%02d",DCF_YEAR_BEGIN+date_time_get_fmt_idx(pin[0]),date_time_get_fmt_idx(pin[1]),date_time_get_fmt_idx(pin[2]));
    return pout;
}

const char *dcf_tools_date_time_zip(const char *pin,char *pout,DWORD buf_size)
{
    DWORD dwData = 0;
    BYTE  bSection = 0;
    const char *pRet = pout;
    while((*pin) && (buf_size))
    {
        BYTE ch = (BYTE)*pin;

        if ((ch >= (BYTE)'0') && (ch <= (BYTE)'9'))
        {
            dwData = dwData * 10 + (ch - (BYTE)'0');
        }
        else if ((ch == (BYTE)'-') || (ch == (BYTE)' ') || (ch == (BYTE)':'))
        {
            // ??????
            bSection++;
            if (bSection == 1)
            {
                // ??
                dwData = dwData - DCF_YEAR_BEGIN;
            }
            else if (bSection > 5)
            {
                break;
            }

            buf_size--;
            dwData = dwData % 64;
            *pout = g_date_fmt_table[dwData];
            pout++;
            dwData = 0;
        }
        else
        {
            break;
        }

        pin++;
    }

    // ????
    if (buf_size)
    {
        buf_size--;
        dwData = dwData % 64;
        *pout = g_date_fmt_table[dwData];
        pout++;
    }

    if (buf_size)
    {
        buf_size--;
        *pout = 0;
    }

    return pRet;
}
const char *dcf_tools_date_time_unzip(const char *pin,char *pout,DWORD buf_size)
{
    if ((buf_size < 21) || (!pout) || (!pin))
    {
        return NULL;
    }

    sprintf_s(pout, buf_size,"%04d-%02d-%02d %02d:%02d:%02d",DCF_YEAR_BEGIN+date_time_get_fmt_idx(pin[0]),date_time_get_fmt_idx(pin[1]),date_time_get_fmt_idx(pin[2]),
              date_time_get_fmt_idx(pin[3]),date_time_get_fmt_idx(pin[4]),date_time_get_fmt_idx(pin[5]));
    return pout;
}

const char *dcf_tools_int_zip(DWORD dw,char *pout,DWORD buf_size,DWORD fmtlen)
{
    const char *pRet = pout;
    /*????4294967295 ??? 3..... ???7???*/
    char chbuffer[7] = {0};
    if (fmtlen > 6)
    {
        fmtlen = 6;
    }
    memset(chbuffer, '0', sizeof(chbuffer) - 1);
    char *pFill  = &chbuffer[sizeof(chbuffer)-1];
    *pFill = 0;
    BYTE byidx = 0;
    while(dw > 0)
    {
        byidx = dw & 0x3f;
        dw = dw>>6;
        pFill--;
        *pFill = g_date_fmt_table[byidx];
    }

    if (fmtlen)
    {
        dcf_strtools::strcpy_s(pout,&chbuffer[sizeof(chbuffer) - 1 - fmtlen ],buf_size);
    }
    else
    {
        dcf_strtools::strcpy_s(pout,pFill,buf_size);
    }

    if (!(*pout))
    {
        *pout = '0';
        *(pout + 1) = 0;
    }

    return pout;
}
DWORD dcf_tools_int_unzip(const char *pin)
{
    DWORD dwValue  = 0;
    while(*pin)
    {
        dwValue = (dwValue <<6) + date_time_get_fmt_idx(*(BYTE*)pin);
        pin++;
    }
    return dwValue;
}

/****************************************************************
*???? : ????????DWORD ??(?????time_t????2030?,??????8??,
                   ???????????????1970???,?2015??????,?????????????)
*???? : NA
*???? : NA
*???? : NA
*?   ? :     zjb
*???? : 2017-07-26  10:2:36
****************************************************************/
void dcf_tools_datetime_atodts(const char *pin,zhw_datetime &zhw_dt)
{
    BYTE *p = (BYTE *)&zhw_dt;
    memset(&zhw_dt,0,sizeof(zhw_dt));
    int itmp = 0;
    BYTE i = 0;
    for(;(*pin) && (i < 6);pin++)
    {
        BYTE ch = *pin;
        if ((ch >= '0') && (ch <= '9'))
        {
            itmp = itmp * 10 + (ch - '0');
            continue;
        }

        if (ch == ' ')
        {
            /* ????????,???? */
            while(*(pin+1) == ' ') pin++;
        }

        if ((ch == '-') || (ch == ':') || (ch == ' '))
        {
            if (i == 0)
            {
                itmp = itmp - DCF_YEAR_BEGIN;
            }

            p[i] = (BYTE)itmp;
            itmp = 0;
            i++;
        }
        else
        {
            /* 2017-07-26  10:20:7 ???????????*/
        }
    }
    if(itmp > 0)
    {
        // ~{4&@mWn:sR;8vCk5DJ}>]~}
        p[5] = (BYTE)itmp;
    }
}

union zhwdt_
{
    BYTE by4[4];
    DWORD dwValue;
    struct zhwdt_bit
    {
        BYTE bDayH2:2;
        BYTE Year:6;

        BYTE bDayL3:3;
        BYTE Hour:5;

        BYTE bMonthH2:2;
        BYTE Minute:6;

        BYTE bMonthL2:2;
        BYTE Second:6;
    }bits;
};

DWORD dcf_tools_datetime_atodw(const char *pin)
{
    zhw_datetime dt;
    dcf_tools_datetime_atodts(pin,dt);
    zhwdt_ dtbits;
    dtbits.dwValue = 0;
    dtbits.bits.Year = dt.Year;
    dtbits.bits.Hour = dt.Hour;
    dtbits.bits.Minute = dt.Minute;
    dtbits.bits.Second = dt.Second;

    dtbits.bits.bDayH2 = (dt.Day>>3);
    dtbits.bits.bDayL3 = (dt.Day&0x7);

    dtbits.bits.bMonthH2 = (dt.Month>>2);
    dtbits.bits.bMonthL2 = (dt.Month&0x3);
    return dtbits.dwValue;
}

void dcf_tools_datetime_dwtodts(DWORD dt,zhw_datetime &zhw_dt)
{
    zhwdt_ dtbits;
    dtbits.dwValue = dt;

    zhw_dt.Year = dtbits.bits.Year;
    zhw_dt.Hour = dtbits.bits.Hour;
    zhw_dt.Minute = dtbits.bits.Minute;
    zhw_dt.Second = dtbits.bits.Second;

    zhw_dt.Day = dtbits.bits.bDayH2;
    zhw_dt.Day = (zhw_dt.Day << 3) | (dtbits.bits.bDayL3);


    zhw_dt.Month = dtbits.bits.bMonthH2;
    zhw_dt.Month = (zhw_dt.Month << 2) | dtbits.bits.bMonthL2;
}

/****************************************************************
*功能描述 : 时间比较 1:dt1>dt2 0:dt1=dt2 -1:dt1<dt2
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-08-03  20:22:37
****************************************************************/
int dcf_tools_datetime_compare(zhw_datetime &dt1,zhw_datetime &dt2)
{
    DWORD dw1,dw2;
    dw1 = dt1.Year * 10000 + dt1.Month * 100 + dt1.Day;
    dw2 = dt2.Year * 10000 + dt2.Month * 100 + dt2.Day;
    if (dw1 != dw2) return (dw1 > dw2)?1:-1;
    dw1 = dt1.Hour * 3600 + dt1.Minute * 60 + dt1.Second;
    dw2 = dt2.Hour * 3600 + dt2.Minute * 60 + dt2.Second;
    if (dw1 != dw2) return (dw1 > dw2)?1:-1;
    return 0;
}


