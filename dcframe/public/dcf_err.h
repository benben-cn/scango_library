
#ifndef dcf_err_h
#define dcf_err_h
/*
系统错误码统一定义区

*/
/*
// 公共的错误码
*/
#define DCF_SUCCESS ((DWORD)0)
#define DCF_ERR_FAILED ((DWORD)-1)
#define DCF_ERR_REPEAT ((DWORD)1)    // 节点重复
#define DCF_ERR_FULL   ((DWORD)2)    // 节点满
#define DCF_ERR_PARAM   ((DWORD)3)   // 错误参数
#define DCF_ERR_TIMEOUT ((DWORD)4)  // 超时
#define DCF_ERR_EMPTY    ((DWORD)5)  // 节点空
#define DCF_ERR_SYS_NONE_INIT ((DWORD)6)  // 系统尚未初始化
#define DCF_ERR_NOT_EXIST ((DWORD)7) // (节点等)不存在
#define DCF_ERR_MSG_EVENT_EMPTY ((DWORD)8) // 消息或者事件为空
#define DCF_ERR_INVALID_DATA ((DWORD)9)   /* 无效数据 */
#define DCF_ERR_INVALID_MEM ((DWORD)10)   /* 内存问题:过大、过小等 */
#define DCF_ERR_BUFLEN  ((DWORD)11)   /* 缓存空间错误 */
#define DCF_ERR_W_DB ((DWORD)12)   /* 写DB错误 */
#define DCF_ERR_R_DB ((DWORD)13)   /* 读DB错误 */
#define DCF_ERR_STATE ((DWORD)14)   /* 错误状态 */

// DCF的任务错误码定义区域
#define DCF_ERR_TASK_BEGIN 0x100
#define DCF_ERR_TASK_NAME (DCF_ERR_TASK_BEGIN + 1)

/*  邮箱错误码*/
#define DCF_ERR_MEMC_BEGIN 0x200
#define DCF_ERR_MEMC_EMPTY (DCF_ERR_MEMC_BEGIN + 1)

/*  数据库错误码*/
#define DCF_SQL_SUCCESS 0x300
#define DCF_ERR_SQL_FAILED (DCF_SQL_SUCCESS + 1)

#define DCF_ERR_MYSQL_INSERT (DCF_SQL_SUCCESS + 2)    //MYSQL插入
#define DCF_ERR_MYSQL_DELETE   (DCF_SQL_SUCCESS + 3)    // MYSQL删除
#define DCF_ERR_MYSQL_EXCUTESQL   (DCF_SQL_SUCCESS + 4)   // MYSQL执行SQL语句
#define DCF_ERR_MYSQL_QUERYSQL   (DCF_SQL_SUCCESS + 5)   //MYSQL执行SQL语句
#define DCF_ERR_MYSQL_UPDATE (DCF_SQL_SUCCESS + 6)  // MYSQL修改


/* 登录错误码*/
#define DCF_ERR_LOGIN_BEGIN 0x400
#define DCF_ERR_LOGIN_FAILD_NO_DEVS   (DCF_ERR_LOGIN_BEGIN+1) //没有找到此设备
#define DCF_ERR_LOGIN_FAILD_KEY       (DCF_ERR_LOGIN_BEGIN+2) //密码错误
#define DCF_ERR_LOGIN_FAILD_NO_SPC    (DCF_ERR_LOGIN_BEGIN+3) //没有spc
#define DCF_ERR_LOGIN_FAILD_VERIFY    (DCF_ERR_LOGIN_BEGIN+4) //设备验证失败
#define DCF_ERR_LOGIN_LOGIN_FREQUENCY (DCF_ERR_LOGIN_BEGIN+5) //频繁登录

#endif