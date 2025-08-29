
#ifndef _DCF_PKGHELPER_H
#define _DCF_PKGHELPER_H
/****************************************************************
*文件范围 : 这个文件为命令和数据结构打包提供方便，推荐大家用这个框架来打包，但不强制
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-19  14:29:15
****************************************************************/

#include "dcf_def.h"
#include "dcf_i_rcc_dth.h"
#ifdef _SW_DCF_MACOS
#include <stdarg.h>
#endif

struct TLV_CMD_PKG_INFO
{
    const char *cmd_stru_script;  /* 描述结构体的参数对象 */
    const char *rsp_stru_script;  /* 响应的参数描述对象，特别注意:前面有一个WORD的错误码 */
    WORD        cmd;                /* 命令字，对应TLV的命令字 */
    WORD        modid;            /* 进行反向关联 */
};

struct TLV_MOD_CMD_REG
{
    WORD        belong_module;   /* 哪个模块注册的 */
    WORD        nums;
    TLV_CMD_PKG_INFO  *regs;
};
/*
可以支持以下类型:(当前只支持前面3种)
1.符号'B':BYTE
2.符号'W':WORD
3.符号'D':DWORD
4.符号'Cn':拷贝指定长度字节内容
5.符号'S+n':拷贝指定长度字节内容的字符串，源数据只移动到0字符，但目标留固定长度n
6.符号'S-n':拷贝指定长度字节内容的字符串，源数据只移动到0字符，目标也不固定长度，只包含到0，但最大不超过n
7.符号'R:':后面是所有类型的重复，而且自身的重复个数不填写，依赖参数长度来识别重复的个数
8.符号'Rm:n':后面部分类型的重复，后面参数个数取决于数字n，m表示自身变量是什么类型,只支持bwd这几个简单类型

参数间分解符号支持几种:
1.无 2.中划线'-' 3.左斜划线'/' 4.空格' '
*/
class dcf_pkg_tool
{
public:
    /* 
    将数据按照fmt格式从psrc转换到pdst 
    Offset :函数内做加法
    地址是函数可以操作的首地址，外面调用时完成地址偏移
    长度:是函数可以操作的长度，为0表示不校验
    */
    static DWORD cvt_fmt_ntoh(void *psrc,const char *fmt,void *pdst,WORD &Offset,int srclen = 0,int dstlen = 0);
    static DWORD cvt_fmt_hton(void *psrc,const char *fmt,void *pdst,WORD &Offset,int srclen = 0,int dstlen = 0);
    /* 指定了模块之后，查找效率会高一些，否则会多一次查找 */
    static DWORD cvt_mod_tlv_ntoh(void *psrc,CRM_CMD &Out,WORD wModid/* 注册这个命令格式的模块id */,WORD &Offset,int srclen = 0);
    /* 多一次根据命令区域匹配到模块的查找 */
    static DWORD cvt_tlv_ntoh(void *psrc,CRM_CMD &Out,WORD &Offset,int srclen = 0);
    
    static DWORD cvt_fmt_tlv_ntoh(void *psrc,CRM_CMD &Out,const char *fmt,WORD &Offset,int srclen = 0);
    static DWORD cvt_mod_tlv_hton(CRM_CMD &In,void *pdst,WORD wModid,WORD &Offset,int dstlen = 0);
    static DWORD cvt_fmt_tlv_hton(CRM_CMD &In,void *pdst,const char *fmt,WORD &Offset,int dstlen = 0);
    /* 多一次根据命令区域匹配到模块的查找 */
    static DWORD cvt_tlv_hton(CRM_CMD &In,void *pdst,WORD &Offset,int dstlen = 0);
};

/* 2017-06-20  17:10:42 为了避免错误调用到重载，这里将函数名修改掉*/
class dcf_para_fmt
{
public:
    /* 根据命令的参数格式拷贝到para缓存中 */
    static DWORD PacketParam_Mid(CRM_CMD &Cmd,WORD modid,...);
    static DWORD PacketParam_Cmd(CRM_CMD &Cmd,BYTE bCmd,...);    
    static DWORD PacketParam_Fm(CRM_CMD &Cmd,WORD &Offset,const char *parafmt,va_list marker);
    static DWORD PacketParam(CRM_CMD &Cmd,WORD &Offset,WORD modid,va_list marker);
    static DWORD PacketParam(CRM_CMD &Cmd,WORD &Offset,va_list marker);
};

class dcf_para_in
{
public:
    dcf_para_in(BYTE *pBuf,DWORD Len);
    ~dcf_para_in(){};
    dcf_para_in &operator <<(BYTE p);
    dcf_para_in &operator <<(WORD p);
    dcf_para_in &operator <<(DWORD p);
    dcf_para_in &operator <<(uint64 p);
    dcf_para_in &operator <<(char *p);
    /* 写固定长度的字符串 */
    bool write_string(char *buffer,WORD para_len);
    /* 写变长字符串 */
    bool write_string_vs(char *p);
    bool have_error(){return m_error?true:false;};
    DWORD GetDataLength(){return m_cursor;};
    /* 用于重新填写数据 */
    void Reset(){m_cursor = 0;m_error = 0;};
protected:
    BYTE   * m_buf;
    DWORD   m_Len;
    DWORD   m_cursor;
    BYTE      m_error;
};

class dcf_para_out
{
public:
    dcf_para_out(BYTE *pBuf,DWORD Len);
    ~dcf_para_out(){};
    dcf_para_out &operator >>(BYTE &p);
    dcf_para_out &operator >>(WORD &p);
    dcf_para_out &operator >>(DWORD &p);
	dcf_para_out &operator >>(uint64 &p);
    dcf_para_out &operator >>(char *p);
    bool read_string(char *buffer,WORD para_len);
    bool read_string_vs(char *buffer,WORD para_len);
    bool have_error(){return m_error?true:false;};
    /* 返回对应的参数指针，并跳过长度 */
    BYTE* get_para_ptr(DWORD para_len);
    /* 获取变长字符串，并跳过长度 */
    const char* get_str_vs_ptr(WORD max_len);
protected:
    BYTE   * m_buf;
    DWORD   m_Len;
    DWORD   m_cursor;
    BYTE      m_error;
};

#endif 
