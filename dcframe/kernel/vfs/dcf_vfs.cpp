
/****************************************************************
*文件范围 : 文件操作相关的函数
*设计说明 : NA
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-07-09  17:53:28
****************************************************************/
#include "dcf_vfs.h"
#include "extern_api.h"
#include "dcf_string.h"

#if _SW_DCF_MACOS
#include <stdio.h>
#include <unistd.h>
#define _access access
#else
#include <io.h>
#endif




bool dcf_file_is_exist(const char *filename)
{
    int dwError = _access(filename, 0);
    if (dwError < 0)
    {
        return false;
    }
    return true;
}

DWORD dcf_file_get_length(const char *filename)
{
    long length = 0;
    FILE *file = fopen(filename, "rb");
    if (file)
    {
        if (fseek(file, 0, SEEK_END) == 0)
        {
            length = ftell(file);
            if (length < 0)
            {
                length = 0;
            }
        }

        /* 关闭文件 */
        fclose(file);
    }
    return length;
}

void dcf_file_remove(const char *filename)
{
    dcf_strtools::replace_char((char*)filename,'\\','/');
    remove(filename);
}
/****************************************************************
*功能描述 : 文件拷贝
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-07-09  22:45:58
****************************************************************/
DWORD dcf_file_copy(const char *src_file,const char *dst_file)
{
#define FILE_BUF_LEN 4*1024
    BYTE buffer[FILE_BUF_LEN];
    int read_len;
    DWORD dwRet = -1;
    FILE *srcfile = fopen(src_file, "rb");
    if (srcfile == NULL)
    {
        return -1;
    }

    FILE *dstfile = fopen(dst_file, "wb");
    if (!dstfile)
    {
        goto cleanup;
    }

    if (fseek(srcfile, 0, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    for(;;)
    {
        read_len = (DWORD)fread (buffer, 1, FILE_BUF_LEN, srcfile);
        if (ferror(srcfile))
        {
            goto cleanup;
        }

        if (read_len <= 0)
        {
            break;
        }

        if (fwrite(buffer,1,read_len,dstfile) != read_len || ferror(dstfile))
        {
            goto cleanup;
        }
    }

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

    return dwRet;
}


void *dcf_file_read(char *pstrFileName, long &length, long dwLimitLen, long dwOffset)
{
    FILE *file = NULL;
    char *content = NULL;
    size_t read_chars = 0;

    length = 0;
    /* open in read binary mode */
    file = fopen(pstrFileName, "rb");
    if (file == NULL)
    {
        goto cleanup;
    }

    /* get the length */
    if (fseek(file, 0, SEEK_END) != 0)
    {
        goto cleanup;
    }

    length = ftell(file);
    if ((length < 0) || (dwOffset >= length))
    {
        goto cleanup;
    }

    if (fseek(file, dwOffset, SEEK_SET) != 0)
    {
        goto cleanup;
    }

    length = length - dwOffset;
    /* allocate content buffer */
    content = (char*)dcf_mem_malloc((size_t)length + sizeof(""));
    if (content == NULL)
    {
        goto cleanup;
    }

    // init memory
    memset(content,0,(size_t)length + sizeof(""));
    /* read the file into memory */
    read_chars = fread(content, sizeof(char), (size_t)length, file);
    if ((long)read_chars != length)
    {
        dcf_mem_free((void*&)content);
        goto cleanup;
    }

    content[read_chars] = '\0';
    cleanup:
    if (file != NULL)
    {
        fclose(file);
    }

    return content;
}

bool dcf_file_write(char *pstrFileName,BYTE *p,DWORD len,DWORD flag)
{
    if ((!p) || (!pstrFileName))
    {
        return true;
    }
    bool bret = false;
    FILE *file = NULL;
    const char *pFlag = "w+";
    int iwritelen = 0;
    if (flag == FILE_WRITE_APPEND)
    {
        pFlag = "r+";
    }

    file = fopen(pstrFileName, pFlag);
    if (file == NULL)
    {
        goto cleanup;
    }

    if (flag == FILE_WRITE_APPEND)
    {
        if (fseek(file, 0, SEEK_END) != 0)
        {
            goto cleanup;
        }
    }

    iwritelen = (DWORD)fwrite(p,sizeof(BYTE),len,file);
    if (iwritelen != len)
    {
        goto cleanup;
    }

    bret = true;
    // 开始写文件
    cleanup:
    if (file != NULL)
    {
        fclose(file);
    }
    return bret;
}

