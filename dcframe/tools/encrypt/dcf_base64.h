#ifndef Dcf_base64_h
#define Dcf_base64_h
/****************************************************************
*文件范围 : 本文是base64编码类:将二进制流转换为文本，编码之后内容会变长
*设计说明 : NA
*注意事项 : NA
*作   者 : zjb
*创建日期 : 2017-05-15  10:4:26
****************************************************************/
#include "dcf_def.h"
/* out 为0表示计算大小 */
extern DWORD dcf_tools_base64_encode(const BYTE in[], BYTE out[], DWORD len, int newline_flag);
extern DWORD dcf_tools_base64_decode(const BYTE in[], BYTE out[], DWORD len);
/* 一组时间的10进制和64进制的转换函数,输出长度是固定的 */
/*日期格式:2017-06-01 输出为261*/
extern const char *dcf_tools_date_zip(const char *pin,char *pout,DWORD buf_size);
extern const char *dcf_tools_date_unzip(const char *pin,char *pout,DWORD buf_size);
/*时间格式:2017-06-01 23:59:59 输出为261mXX*/
extern const char *dcf_tools_date_time_zip(const char *pin,char *pout,DWORD buf_size);
extern const char *dcf_tools_date_time_unzip(const char *pin,char *pout,DWORD buf_size);
/*最大数字4294967295 表示为 3..... 只需要7个字节,fmtlen非0表示需要固定位数的长度输出，前面用0格式化，为0则表示只输出有效字符*/
extern const char *dcf_tools_int_zip(DWORD dw,char *pout,DWORD buf_size,DWORD fmtlen);
extern DWORD dcf_tools_int_unzip(const char *pin);
extern void dcf_tools_datetime_atodts(const char *pin,zhw_datetime &zhw_dt);
extern DWORD dcf_tools_datetime_atodw(const char *pin);
extern void dcf_tools_datetime_dwtodts(DWORD dt,zhw_datetime &zhw_dt);
extern int dcf_tools_datetime_compare(zhw_datetime &dt1,zhw_datetime &dt2);
#endif

