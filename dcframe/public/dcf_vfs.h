#ifndef Dcf_vfs_h
#define Dcf_vfs_h
/****************************************************************
*文件范围 : 提供框架虚拟文件系统能力，让应用APP只看到工作目录的相对路径，以便于多实例部署时数据隔离
*设计说明 : 设计要求:
                   1.支持绝对路径:路径不以./开头
                   2.支持相对路径:路径以./或者../开头，vfs自动添加根路径，再调用操作系统的函数
                   3.路径的创建统一由vfs完成，应用模块需要特殊的，可以在json中进行配置
*注意事项 : NA
*作   者 :     zjb
*创建日期 : 2017-05-16  9:47:13
****************************************************************/
#include "dcf_def.h"

/* 
几种常量路径
1.下面路径都是基于run_root+dt_root的相对路径
2.TMP目录下的文件，每次启动时会被清空
*/
enum
{
    DIR_DT_ROOT = 0,   /* 数据目录的根目录 */
    DIR_TEMP = 1,          /* 下面这些目录都是基于dt的根目录  */
    DIR_SQLITE_TMP,
    DIR_SQLITE,
    DIR_LOG,
    DIR_MAX
};
#define FILE_WRITE_OWERRITE 1
#define FILE_WRITE_APPEND 2

extern bool dcf_file_is_exist(const char *filename);
extern DWORD dcf_file_get_length(const char *filename);
extern void dcf_file_remove(const char *filename);
extern DWORD dcf_file_copy(const char *src_file,const char *dst_file);
extern void *dcf_file_read(char *pstrFileName, long &length, long dwLimitLen = 0, long dwOffset = 0);
extern bool dcf_file_write(char *pstrFileName,BYTE *p,DWORD len,DWORD flag = FILE_WRITE_APPEND);


extern const char *dcf_dir_get_workroot();
extern const char *dcf_dir_get_path(WORD type,char dir[DCF_DIR_FPLEN_MAX]);
extern const char *dcf_dir_get_filepath(WORD type,char filepath[DCF_DIR_FPLEN_MAX],const char *filename);
#endif


