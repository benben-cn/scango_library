#ifndef Dcf_zlib_h
#define Dcf_zlib_h

#if _SW_DCF_LINUX
#include <errno.h>
#endif

/****************************************************************
*文件范围 : 提供压缩方法
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-09  16:45:56
****************************************************************/
extern DWORD dcf_zlib_compress_mtom(BYTE *src,DWORD src_len,BYTE *dst,DWORD &dst_len);
extern DWORD dcf_zlib_uncompress_mtom(BYTE *src,DWORD src_len,BYTE *dst,DWORD &dst_len);
extern DWORD dcf_zlib_compress_ftof(const char *srcfilepath,const char *dstfilepath,int level = -1);
extern DWORD dcf_zlib_uncompress_ftof(const char *filepath,const char *dstfilepath);
extern DWORD dcf_zlib_compress_mtof(const BYTE *src,DWORD src_len,const char *dstfilepath,int level = 5);
extern DWORD dcf_zlib_uncompress_mtof(const BYTE *src,DWORD src_len,const char *dstfilepath);
#endif
