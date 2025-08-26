#ifndef dcf_self_api_h
#define dcf_self_api_h

/*
文件说明:该文件中包含各种API，但不希望用户直接调用
作者：zjb
时间：2014-4-26 10:58
*/
#include "dcf_def.h"


extern void dcf_prepareCryptTable();
extern DWORD dcf_hash_string(const char *lpszFileName, DWORD dwHashType);

#endif

