/****************************************************************
*文件范围 : 二维码工具类
*设计说明 : 二维码的标准样式:
                   1.完整型:           https://zhedao.org/s/01010bTUCsNB9lvONG9pWz3VBg~~soXI6aWY2TVk5vFgF1sFeA~~
                   2.已经去头型:    /s/01010bTUCsNB9lvONG9pWz3VBg~~soXI6aWY2TVk5vFgF1sFeA~~
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-19  14:40:55
****************************************************************/
#include "dcf_qrcode.h"
#include "dcf_string.h"
#include "dcf_base64.h"
#include "extern_api.h"


static const char *zhw_qrcode_flag[] = {"i", "k", "p", "a", "g", "f", "m", "d", "c", "z"};
static const QR_TYPE zhw_qrcode_type[] = {
        QR_TYPE_S,
        QR_TYPE_BOX,
        QR_TYPE_BATTER,
        QR_TYPE_A,
        QR_TYPE_G,
        QR_TYPE_F,
        QR_TYPE_M,
        QR_TYPE_D,
        QR_TYPE_C,
        QR_TYPE_Z
};

const char *dcf_sv_qrcode_get_type(BYTE type) {
    return qrcode_tools::qrtype_wtos(type);
}

WORD dcf_sv_qrcode_get_pubkey_ver() {
    return 0x0101;
}

const char *qrcode_tools::qrtype_wtos(WORD type) {
    for (WORD i = 0; i < sizeof(zhw_qrcode_type) / sizeof(int); i++) {
        if (zhw_qrcode_type[i] == type) {
            return zhw_qrcode_flag[i];
        }
    }

    return NULL;
}

WORD qrcode_tools::qrtype_stow(const char *flag) {
    for (WORD i = 0; i < sizeof(zhw_qrcode_flag) / sizeof(int); i++) {
        if (_stricmp(zhw_qrcode_flag[i], flag) == 0) {
            return zhw_qrcode_type[i];
        }
    }

    return -1;
}


/****************************************************************
*功能描述 : 获取二维码的版本
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-19  14:42:24
****************************************************************/
WORD qrcode_tools::get_pubkeyver(const char *url) {
    WORD ver = 0;
    int iPts[8];
    int iLens[8];
    int iNums = 0;

    dcf_strtools::dcf_split_string(url, '/', iPts, iLens, sizeof(iPts) / sizeof(int), &iNums);
    if (iNums < 2) {
        return ver;
    }

    if (iLens[iNums - 1] < 24) {
        return ver;
    }

    char data[7] = {'0', 'x', 0};
    memcpy(&data[2], &url[iPts[iNums - 1]], 4);
    data[6] = 0;
    ver = (WORD) dcf_strtools::strtol(data);
    return ver;
}

/****************************************************************
*功能描述 : 获取二维码中的类型
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-19  14:54:20
****************************************************************/
WORD qrcode_tools::get_qrtype(const char *url) {
    int iPts[8];
    int iLens[8];
    int iNums = 0;

    dcf_strtools::dcf_split_string(url, '/', iPts, iLens, sizeof(iPts) / sizeof(int), &iNums);
    if ((iNums < 2) || (iLens[iNums - 1] < 24)) {
        return QR_TYPE_MAX;
    }

    char data[4] = {0};
    dcf_strtools::get_string(url, data, iPts[iNums - 2], iLens[iNums - 2], sizeof(data));
    data[3] = 0;
    return qrtype_stow(data);
}

/****************************************************************
*功能描述 : 从url中获取相关企业id和二维码日期
*输入参数 : url:二维码地址
                   pub_key:公钥
*输出参数 : strdatetime:二维码日期
*返回参数 : 制作二维码的企业id
*作   者 :     zjb
*创建日期 : 2017-07-19  15:36:0
 *
 *
****************************************************************/
DWORD qrcode_tools::get_qrcode_info(const char *url, char strdatetime[32], const char *key) {
    int iPts[8];
    int iLens[8];
    int iNums = 0;
    DWORD bs_id = 0;
    // 1.获取字符串
    dcf_strtools::dcf_split_string(url, '/', iPts, iLens, sizeof(iPts) / sizeof(int), &iNums);
    if (iNums < 2) {
        return bs_id;
    }

    char temp[32] = {0};
    if (iLens[iNums - 1] >= sizeof(temp)) {
        return bs_id;
    }
    // 获取到字符串
    iLens[iNums - 1] -= 4;
    dcf_strtools::get_string(url, temp, iPts[iNums - 1] + 4, iLens[iNums - 1], sizeof(temp));
    // 2.补齐去掉了的~符号
    for (int i = iLens[iNums - 1]; i < 24; i++) {
        temp[i] = '~';
    }
    temp[24] = 0;

    // 3.base64解码
    BYTE decode[32];
    dcf_tools_base64_decode((BYTE *) temp, decode, 24);

    // 4.解密
    dcf_tools_3des64_decrypt(decode, 8, (BYTE *) temp, key);
    // 5.得到企业id
    DWORD *p = (DWORD *) temp;
    bs_id = ntohl(*p);
    // 6.得到日期
    zhw_datetime zhw_dt = {0};
    dcf_tools_datetime_dwtodts(ntohl(*(++p)), zhw_dt);
    sprintf(strdatetime, "%d-%02d-%02d %02d:%02d:%02d", 2015 + zhw_dt.Year, zhw_dt.Month, zhw_dt.Day, zhw_dt.Hour, zhw_dt.Minute, zhw_dt.Second);
    return bs_id;
}

/****************************************************************
*功能描述 : 用企业私钥解析二维码，得到验证码原始数据
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :  wlq
*创建日期 : 2017-11-1
****************************************************************/
DWORD qrcode_tools::zhw_qrcode_self_decrypt(const char *strFullUrl, const char *self_key, qr_ctrl *ctrl) {
    int iPts[8];
    int iLens[8];
    int iNums = 0;

    // 1.获取字符串
    dcf_strtools::dcf_split_string(strFullUrl, '/', iPts, iLens, sizeof(iPts) / sizeof(int), &iNums);
    if (iNums < 2) {
        return -1;
    }

    char temp[32] = {0};
    if (iLens[iNums - 1] >= sizeof(temp)) {
        return -1;
    }

    // 获取到字符串
    iLens[iNums - 1] -= 4;
    dcf_strtools::get_string(strFullUrl, temp, iPts[iNums - 1] + 4, iLens[iNums - 1], sizeof(temp));
    // 2.补齐去掉了的~符号
    for (int i = iLens[iNums - 1]; i < 24; i++) {
        temp[i] = '~';
    }
    temp[24] = 0;

    // 3.base64解码
    BYTE decode[32];
    dcf_tools_base64_decode((BYTE *) temp, decode, 24);

    // 4.解密
    dcf_tools_3des64_decrypt(decode + 8, 8, (BYTE *) temp, self_key);
    // 5.得到  验证码|(1 << 20)的值
    DWORD *p = (DWORD *) temp;
    DWORD randNum = ntohl(*p);
    randNum = ((1 << 20) - 1) & randNum;

    /* 2018-01-09  13:9:51 添加控制码*/
    if (ctrl) {
        p = (DWORD *) (temp + sizeof(DWORD));
        ctrl->dwV = ntohl(*p);
    }
    return randNum;
}

/****************************************************************
*功能描述 : 跳过url的二维码头部
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-19  15:42:41
****************************************************************/
const char *qrcode_tools::skip_url_head(const char *url) {
    if (*url == '/') {
        return url;
    }

    WORD ver = 0;
    int iPts[8];
    int iLens[8];
    int iNums = 0;

    dcf_strtools::dcf_split_string(url, '/', iPts, iLens, sizeof(iPts) / sizeof(int), &iNums);
    if ((iNums < 2) || (iLens[iNums - 1] < 24)) {
        return NULL;
    }

    return url + (iPts[iNums - 2] - 1);
}