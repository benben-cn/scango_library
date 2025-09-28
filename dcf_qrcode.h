#ifndef dcf_qrcode_h
#define dcf_qrcode_h
/****************************************************************
*文件范围 : 提供对二维码处理的一系列静态函数
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-19  14:35:11
****************************************************************/
#include "dcf_def.h"

/* 二维码类型和字符串对应表 */
enum QR_TYPE
{
    QR_TYPE_S = 1,           /* 单品码 */
    QR_TYPE_BOX,            /* 箱码 */
    QR_TYPE_BATTER,     /* 垛码 */
    QR_TYPE_A = 21,         /* 场地码 */
    QR_TYPE_G = 22,         /* 生长码 */
    QR_TYPE_F = 23,         /* 农销码 */
    QR_TYPE_M = 24,         /* 农标码 */
    QR_TYPE_D = 25,         /* 农售码 */
    QR_TYPE_C = 26,         /* 框码 */
    QR_TYPE_Z = 40,         /* 资产码 */
    QR_TYPE_MAX
};

#define QRC_HEAD 3
/* 验证码类型 3bit 支持8种类型 */
#define QRC_UNKOWN 0  /* 旧版本不确定 */
#define QRC_NULL 1  /* 无验证码 */
#define QRC_LOCAL 2  /* 产线生成验证码 */
#define QRC_BSS  3  /* BSS生成验证码 */
/*
 * 2018-01-09  12:4:49 该结构体由二维码中的原始序号而来，由rand产生的序号，值在0~0x7fff(1048575)之间还有大约12bit未使用，用来做控制扩展用
 */
union qr_ctrl
{
    DWORD dwV;
    struct
    {
        DWORD bit_seq:20;/* 随机值 */
        DWORD bit_res:7;   /* 保留 */
        DWORD bit_vct:3;   /* 验证码类型:verify code type */
        DWORD bit_flag:2;  /* 头部11 */
    }detail;
};

class qrcode_tools
{
public:
    static WORD   get_pubkeyver(const char *url);
    static WORD   get_qrtype(const char *url);
    static DWORD get_qrcode_info(const char *url, char strdatetime[32], const char *pub_key);
    static DWORD zhw_qrcode_self_decrypt(const char *url, const char *self_key, qr_ctrl *ctrl);
    static const char* skip_url_head(const char *url);
    static const char* qrtype_wtos(WORD type);
    static WORD qrtype_stow(const char *type);
};

#endif