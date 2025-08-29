#ifndef _EXTERN_API_H
#define _EXTERN_API_H
/*
文件说明:该文件包含用户可以直接使用的API函数
作者：zjb
时间：2014-4-26
*/

#include "dcf_def.h"
#include "dcf_self_api.h"
#include "dcf_msg0.h"

/*****************************************************************
********** 系统初始化API **********************************
*****************************************************************/
extern bool dcf_init(char *pcfgfilename);
extern void dcf_loop();
extern void dcf_exit();
extern char *dcf_tools_get_shortname(const char *pfilename);

// 该类用于获取整个系统的模块配置对象(cJSON*),特别注意，对象使用(子系统加载完)后需要主动释放一次，否则会造成对象
extern DWORD dcf_cjson_get_numvar(void *root,const char *valuename,DWORD def_value = 0,DWORD max_value = 0);
extern char* dcf_cjson_get_stringvar(void *root,const char *valuename,char *chbuffer,DWORD buflen,const char *valuedef = 0);
extern void dcf_reboot_ex(DWORD sysid,DWORD reset_type,WORD modid,char *pinfo);
extern void *dcf_sys_get_frame();
extern const char *dcf_sys_get_com_pubkey(WORD ver);  /* 获取通信frame层的公钥 */

/*
SYSID编址定义:
编址规则: snode(20bit 1048576) + pnode(12bit 4096)
解释:
snode:服务节点
pnode:服务进程节点，隶属于对应snode
*/
inline DWORD dcf_sysid_get_snode(DWORD sysid)
{
    return (sysid >> 12);
}
inline DWORD dcf_sys_get_pnode(DWORD sysid)
{
    return (sysid & 0xFFF);
}
inline DWORD dcf_sys_gen_sysid(DWORD snode_id,DWORD pnode_id)
{
    return (snode_id<<12)|(pnode_id &0xFFF);
}
extern DWORD dcf_sysid_get_self();
extern DWORD dcf_sysid_atol(const char *cs_sysid);
extern char *dcf_sysid_ltoa(DWORD sysid,char chbuffer[32]);

// 返回对应模块的配置项:如:"dcframe.s_kernel.filesystem"
extern void *dcf_cfg_get_module(const char *pmodulelist);
extern void *dcf_get_module_next_cfg(void *pRoot,const char *pmodulelist);
/*****************************************************************
********** 系统输出API **********************************
*****************************************************************/
extern void dcf_output(const char *fmt,...);
#define NEED_DAY  1
#define NEED_TIME 2
#define NEED_MS    4
#define NEED_ALL (-1)
extern char* dcf_get_time_string(char chbuffer[32],int iflag = NEED_ALL);
// 为了避免过多消耗栈内存，声明一个非线程安全的函数，可以用于不重要的，而且调用极其不频繁的地方
extern char *dcf_get_time_string_unsafe(int iflag = NEED_ALL);
/*****************************************************************
********** 任务、队列、模块相关API **********************************
特别注意顺序:1.任务 2.队列 3.模块 弄反之后会引起注册失败
*****************************************************************/
extern DWORD dcf_task_register(const char *pTaskName,BYTE byTaskPrio,DWORD dwStackSize, DCF_FUNCPTR taskEntry, DCFLPARAM lParam);
extern DWORD dcf_task_unregister(const char *pTaskName);
extern DWORD dcf_task_unregister(WORD tid);
extern DWORD dcf_task_startwork(const char *pTaskName);
extern DWORD dcf_task_get_info(const char *pTaskName,WORD &taskPid,DWORD &threadid);
extern DWORD dcf_task_get_tid_bythreadid(DWORD Threadid,WORD &tid);
extern void  dcf_task_get_tid_curthread(WORD &tid,DWORD &threadid);  // 因为该函数threadid一定会成功，所以这个函数不会返回失败，通过判断tid有效性区分吧

// 任务收发事件
extern DWORD dcf_task_recv_event(WORD tid,DWORD dwWantEvent,DWORD dwTimeOut,DWORD &dwRecvEvent);
// extern DWORD dcf_task_recv_event(DWORD dwWantEvent,DWORD dwTimeOut,DWORD &dwRecvEvent);
extern DWORD dcf_task_send_event(WORD dstTid,DWORD dwEvent);
extern void  dcf_task_wait_for_start_work(void *arg);
extern void dcf_task_loadplug_end();



// 消息队列相关
extern DWORD dcf_msg_que_register(const char *msgQuename,DWORD msgQueNums,WORD &wQueIdx,WORD wQueFlag = DCF_MSGQUE_FLAG_FIFO);
extern DWORD dcf_msg_que_unregister(const char *msgQuename);
extern DWORD dcf_msg_que_unregister(WORD Queid);
extern DWORD dcf_msg_que_get_info(const char *msgQuename,WORD &msgQueid);
extern DWORD dcf_msg_que_get_id_bythreadid(DWORD Threadid,WORD &queid,DWORD &que_event);
extern DWORD dcf_msg_que_push(WORD Queid,DCFQUEMSG &msg,DWORD Flag = DCF_MSGQUE_FLAG_FIFO);
extern DWORD dcf_msg_que_pop(WORD Queid,DCFQUEMSG &msg);
extern DWORD dcf_msg_que_count(WORD Queid,WORD &count);

extern BOOL dcf_msg_que_is_timerspool(void *p);

// 模块相关
struct module_entry
{
    const char *chtaskname;
    const char *chmsgquename;/* 可以是没有队列，只接收广播事件 */
    WORD        EntryID;/* BYTE类型 */
    WORD        timer_type;  /* 定时器类型:0是邮箱定时器(占用消息队列)，1:任务定时器(32个定时器) */
    DWORD     dwEventID;/* 消息进队列后通知任务的事件ID */
    DWORD     dwTimerEventID;  /* 定时器到队列之后通知任务的事件ID */
};
extern DWORD dcf_module_register(const char *modname,WORD modid,WORD wEntryNums,module_entry *entrys);
extern DWORD dcf_module_get_id_bythreadid(DWORD Threadid,WORD &Modid);
extern DWORD dcf_module_get_task_and_msgque_id(WORD wModID,BYTE byEntryid,WORD &tid,WORD &msgQueid,DWORD &eventid);

// 综合
extern DWORD dcf_task_msgque_regiser(const char *task_name,const char *que_name = NULL,WORD ModID = 0,DWORD QueEventID = 0);

// 定时器相关(自有任务定时器)
// 特别注意:禁止在对外接口函数中进行定时器操作，否则会将定时器创建在调用者的任务上
extern DWORD dcf_tasktimer_flush(DWORD dwTimerID,DWORD dwTimeOut,DWORD dwFlag = 0 /*0:超时定时器 1:周期定时器*/);
extern DWORD dcf_tasktimer_get_events(DWORD &dwTimerIDs);
extern DWORD dcf_tasktimer_get_event(DCFQUEMSG &msg,DWORD &dwTimerID,WORD *pModid = NULL);
extern DWORD dcf_tasktimer_cancel(DWORD dwTimerID,WORD tid);   /* 2017-07-17  8:37:20 出错之后影响很大，增加一个校验tid*/

/*****************************************************************
********** 内存申请相关API **********************************
*****************************************************************/
extern void* dcf_mem_malloc_ex(DWORD dwSize, DWORD dwFileLine, char *pFileName);
extern DWORD dcf_mem_free_ex(void *&p, DWORD dwFileLine, char *pFileName);
extern void *dcf_get_sys_module_cfg(const char *pmodulelist);

#define dcf_mem_malloc(size) dcf_mem_malloc_ex(size, __LINE__,dcf_tools_get_shortname(__FILE__))
#define dcf_mem_free(p) dcf_mem_free_ex(p, __LINE__,dcf_tools_get_shortname(__FILE__))

/*****************************************************************
********** 文件系统相关API **********************************
*****************************************************************/
extern void dcf_vfs_set_run_root(char *p);
/*****************************************************************
********** 日志相关API **********************************
*****************************************************************/
extern void dcf_sys_write_log(DWORD sysid,WORD logtypeid,WORD level,char *pinfo);
extern void dcf_sys_checkerr_ex(DWORD dwRet,BYTE byLevel,WORD modid,char *errinfo,const char *pfilename,DWORD dwFileLine,const char *pfuncname);
#define dcf_sys_checkerr(dwRet,bylevel,modid,errinfo) \
    dcf_sys_checkerr_ex(dwRet,bylevel,modid,errinfo,dcf_tools_get_shortname(__FILE__), __LINE__,__FUNCTION__)
extern void dcf_sys_checker_fmt_ex(DWORD dwRet,BYTE byLevel,WORD modid,const char *pfilename,DWORD dwFileLine,const char *pfuncname,char *fmt,...);
#define dcf_sys_checkerr_fmt(dwRet,bylevel,modid,errinfo,...) \
    dcf_sys_checker_fmt_ex(dwRet,bylevel,modid,dcf_tools_get_shortname(__FILE__), __LINE__,__FUNCTION__,errinfo,__VA_ARGS__)
/*****************************************************************
********** 加解密相关API **********************************
*****************************************************************/
extern void dcf_tools_md5_encry(const char *pin,char *pout,DWORD dstlen);
/* 计算CRC16的值 */
extern WORD dcf_tools_crc16_get(BYTE *q,int ilen);
/* CES128加密函数 支持按流的方式进行加密和解密，第一次调用时需要输入key     */
/* 2017-05-25  9:19:53 特别注意:加密报文必须是16字节的倍数否则解密会不成功,所以输出缓存一定要是16的整数倍*/
extern DWORD dcf_tools_ces128_encrypt(BYTE* input, DWORD datalen, BYTE* output, const char* key);
/* 密文中无法知道解密之后的报文长度，一定要在另外的地方去描述 */
extern DWORD dcf_tools_ces128_decrypt(BYTE* input, DWORD length,BYTE* output,const char* key);
/* 2017-07-25  10:14:0 增加3des加解密函数，研究qrcode的短码方式 */
extern DWORD dcf_tools_3des64_encrypt(BYTE* input, DWORD datalen, BYTE* output,const char* key);
extern DWORD dcf_tools_3des64_decrypt(BYTE* input, DWORD length,BYTE* output,const char* key);
/*****************************************************************
********** 其它杂类相关API **********************************
*****************************************************************/
// 获取一个随机数
extern DWORD dcf_tools_get_rand(bool no_zero = true);
extern const char *dcf_tools_get_cmd_fmt_info(WORD wCmd,BYTE bcmd);
extern int dcf_tools_get_current_path(char buf[],char *pFileName);
extern DWORD dcf_ini_get_string(const char *title,const char *key,const char *l_default,char *buffer,int buf_len,const char *filename);
extern int dcf_ini_get_int(const char *title,const char *key,int def,const char *filename);

/*****************************************************************
********** 业务相关API **********************************
*****************************************************************/
extern const char *dcf_sv_qrcode_get_url();
extern WORD dcf_sv_qrcode_get_pubkey_ver();
extern const char *dcf_sv_qrcode_get_type(BYTE type);
/* 信息服务提供者注册 */
extern DWORD dcf_sv_info_provider_register(WORD info_type,WORD mod_id,BYTE entry_id = 1);
extern DWORD dcf_sv_info_provider_get(WORD info_type,WORD &mod_id,BYTE &entry_id);

#define dcf_min(a,b) (((a)>(b))?(b):(a))


#endif

