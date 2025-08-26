/******************************************************
************* 框架内部看到的统一头文件 *****************
*******************************************************/
#ifndef _DCF_H
#define _DCF_H

#include "dcf_def.h"
#include "dcf_self_api.h"
#include "extern_api.h"

// 用于配置本子系统下对应模块的配置
struct SYS_MODULE_CFG
{
    // 模块名
    char *dcf_module_name;
    // 配置文件中的名称
    char *cfg_module_keyname;
    // 初始化函数
    DCF_SYSINIT_FUNCPTR func_init;
    // 系统退出函数
    DCF_FUNCPTR_VOID func_exit;
};

#endif
