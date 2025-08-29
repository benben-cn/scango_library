/****************************************************************
*文件范围 : 常用的压缩相关的方法
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-09  17:29:59
****************************************************************/
#include "zlib.h"
#include "dcf_vfs.h"
#include "extern_api.h"

#if _SW_DCF_MACOS
#include <errno.h>
#endif


DWORD dcf_zlib_compress_mtom(BYTE *src,DWORD src_len,BYTE *dst,DWORD &dst_len)
{
    int zerror = 0;

#if _SW_DCF_MACOS
    if ((zerror = compress(dst,(uLongf*)&dst_len,src,src_len)) != Z_OK)
#else
        if ((zerror = compress(dst,&dst_len,src,src_len)) != Z_OK)
#endif
    {
        dcf_output("[%s]compress mem failed!(ret%d:(srclen:%d,dstlen:%d))",dcf_get_time_string_unsafe(),src_len,dst_len);
        return -1;
    }

    return 0;
}

DWORD dcf_zlib_uncompress_mtom(BYTE *src,DWORD src_len,BYTE *dst,DWORD &dst_len)
{
    int zerror = 0;
#if _SW_DCF_MACOS
    if ((zerror = uncompress(dst,(uLongf*)&dst_len,src,src_len)) != Z_OK)
#else
        if ((zerror = uncompress(dst,&dst_len,src,src_len)) != Z_OK)
#endif
    {
        dcf_output("[%s]uncompress mem failed!(ret%d:(srclen:%d,dstlen:%d))",dcf_get_time_string_unsafe(),src_len,dst_len);
        return -1;
    }

    return 0;
}

#define ZLIB_SIZE 4*1024
DWORD dcf_zlib_uncompress_ftof(const char *srcfilepath, const char *dstfilepath)
{
    DWORD dwRet = -1;
    /* 压缩需要的缓存 */

    bool bNeedEnd = false;
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[ZLIB_SIZE];
    unsigned char out[ZLIB_SIZE];

    FILE *srcfile = fopen(srcfilepath, "rb");
    FILE *dstfile = NULL;
    if (srcfile == NULL)
    {
        return dwRet;
    }

    if (fseek(srcfile, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* 创建新文件 */
    dstfile = fopen(dstfilepath, "wb");
    if (!dstfile)
    {
        goto cleanup;
    }

    bNeedEnd = true;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
        goto cleanup;
    }

    do
    {
        strm.avail_in = (DWORD)fread(in,1,ZLIB_SIZE,srcfile);
        if (ferror(srcfile))
        {
            goto cleanup;
        }

        if (0 == strm.avail_in)
        {
            break;
        }

        strm.next_in = in;
        do
        {
            strm.avail_out = ZLIB_SIZE;
            strm.next_out = out;

            ret = inflate(&strm, Z_NO_FLUSH);
            if ((ret != Z_OK) && (ret != Z_STREAM_END))
            {
                goto cleanup;
            }

            have = ZLIB_SIZE - strm.avail_out;
            if (fwrite(out,1,have,dstfile) != have || ferror(dstfile))
            {
                goto cleanup;
            }
        }while (strm.avail_out == 0);
    }while(ret != Z_STREAM_END);

    dwRet = 0;
    cleanup:
    if (srcfile)
    {
        fclose(srcfile);
    }
    if (dstfile)
    {
        fclose(dstfile);
    }

    if (bNeedEnd)
    {
        (void)inflateEnd(&strm);
        if (dwRet)
        {
            /* 失败了需要删除目标文件 */
            dcf_file_remove(dstfilepath);
        }
    }

    return dwRet;
}

DWORD dcf_zlib_compress_ftof(const char *srcfilepath, const char *dstfilepath, int level)
{
    DWORD dwRet = -1;
    /* 压缩需要的缓存 */

    bool bNeedEnd = false;
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[ZLIB_SIZE];
    unsigned char out[ZLIB_SIZE];

    FILE *srcfile = fopen(srcfilepath, "rb");
    FILE *dstfile = NULL;
    if (srcfile == NULL)
    {
        return dwRet;
    }

    if (fseek(srcfile, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    /* 创建新文件 */
    dstfile = fopen(dstfilepath, "wb");
    if (!dstfile)
    {
        int i = errno;
        goto cleanup;
    }

    /*allocate defalte state*/
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm,level);
    if (ret != Z_OK)
    {
        goto cleanup;
    }

    bNeedEnd = true;
    do
    {
        strm.avail_in = (DWORD)fread (in, 1, ZLIB_SIZE, srcfile);
        if (ferror(srcfile))
        {
            goto cleanup;
        }
        flush = feof(srcfile)?Z_FINISH:Z_NO_FLUSH;
        strm.next_in = in;
        do
        {
            strm.avail_out = ZLIB_SIZE;
            strm.next_out = out;
            ret = deflate(&strm,flush);
            have = ZLIB_SIZE - strm.avail_out;
            if (fwrite(out,1,have,dstfile) != have || ferror(dstfile))
            {
                goto cleanup;
            }
        }while(strm.avail_out == 0);
    }while(flush != Z_FINISH);

    dwRet = 0;
    cleanup:
    if (srcfile)
    {
        fclose(srcfile);
    }
    if (dstfile)
    {
        fclose(dstfile);
    }

    if (bNeedEnd)
    {
        (void)deflateEnd(&strm);
        if (dwRet)
        {
            /* 失败了需要删除目标文件 */
            dcf_file_remove(dstfilepath);
        }
    }

    return dwRet;
}


DWORD dcf_zlib_compress_mtof(const BYTE *src,DWORD src_len,const char *dstfilepath,int level)
{
    DWORD dwRet = -1;
    /* 压缩需要的缓存 */

    bool bNeedEnd = false;
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char out[ZLIB_SIZE];

    /* 创建新文件 */
    FILE *dstfile = fopen(dstfilepath, "wb");
    if (!dstfile)
    {
        goto cleanup;
    }

    /*allocate defalte state*/
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit(&strm,level);
    if (ret != Z_OK)
    {
        goto cleanup;
    }

    bNeedEnd = true;
    strm.avail_in = src_len;
    flush = Z_FINISH;
    strm.next_in = (BYTE*)src;
    do
    {
        strm.avail_out = ZLIB_SIZE;
        strm.next_out = out;
        ret = deflate(&strm,flush);
        have = ZLIB_SIZE - strm.avail_out;
        if (fwrite(out,1,have,dstfile) != have || ferror(dstfile))
        {
            goto cleanup;
        }
    }while(strm.avail_out == 0);

    dwRet = 0;
    cleanup:

    if (dstfile)
    {
        fclose(dstfile);
    }

    if (bNeedEnd)
    {
        (void)deflateEnd(&strm);
        if (dwRet)
        {
            /* 失败了需要删除目标文件 */
            dcf_file_remove(dstfilepath);
        }
    }

    return dwRet;
}
DWORD dcf_zlib_uncompress_mtof(const BYTE *src,DWORD src_len,const char *dstfilepath)
{
    DWORD dwRet = -1;
    /* 压缩需要的缓存 */

    bool bNeedEnd = false;
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[ZLIB_SIZE];

    /* 创建新文件 */
    FILE *dstfile = fopen(dstfilepath, "wb");
    if (!dstfile)
    {
        return dwRet;
    }

    bNeedEnd = true;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
        goto cleanup;
    }

    strm.avail_in = src_len;
    strm.next_in = (BYTE*)src;
    do
    {
        strm.avail_out = ZLIB_SIZE;
        strm.next_out = out;

        ret = inflate(&strm, Z_NO_FLUSH);
        if ((ret != Z_OK) && (ret != Z_STREAM_END))
        {
            goto cleanup;
        }

        have = ZLIB_SIZE - strm.avail_out;
        if (fwrite(out,1,have,dstfile) != have || ferror(dstfile))
        {
            goto cleanup;
        }
    }while (strm.avail_out == 0);

    dwRet = 0;
    cleanup:
    if (dstfile)
    {
        fclose(dstfile);
    }

    if (bNeedEnd)
    {
        (void)inflateEnd(&strm);
        if (dwRet)
        {
            /* 失败了需要删除目标文件 */
            dcf_file_remove(dstfilepath);
        }
    }

    return dwRet;
}

