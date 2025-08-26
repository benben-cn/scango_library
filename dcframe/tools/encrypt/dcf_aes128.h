
#ifndef _DCF_AES128_H
#define _DCF_AES128_H
/****************************************************************
*文件范围 : 本文是AES128加密算法
                   来自网上开源代码:https://github.com/kokke/tiny-AES128-C
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-22  10:52:29
****************************************************************/

#include "dcf_def.h"


// #define the macros below to 1/0 to enable/disable the mode of operation.
//
// CBC enables AES128 encryption in CBC-mode of operation and handles 0-padding.
// ECB enables the basic ECB 16-byte block algorithm. Both can be enabled simultaneously.

// The #ifndef-guard allows it to be configured before #include'ing or at compile time.

/* 
固定用CBC模式:密码分组链接模式（Cipher Block Chaining (CBC)）
做了一些线程安全方面的改造
*/

#endif

void aes_encrypt(const BYTE in[], BYTE out[], const UINT key[], int keysize);
void aes_decrypt(const BYTE in[], BYTE out[], const UINT key[], int keysize);
DWORD dcf_tools_ces128_encrypt(BYTE* input, DWORD datalen, BYTE* output,const char* key);
DWORD dcf_tools_ces128_decrypt(BYTE* input, DWORD length,BYTE* output,const char* key);