
#ifndef _DCF_MSG0_H
#define _DCF_MSG0_H
/****************************************************************
*文件范围 : 包含系统所有邮箱消息的msg0
*设计说明 : 虽然除了公共消息id之外的msg0，msg0是模块内唯一，但因涉及到其它模块需要可见，因此
                   将这个信息单独管理公示起来
*注意事项 : NA
*作   者 : zjb
*创建日期 : 2017-07-04  9:3:6
****************************************************************/
#include "dcf_def.h"

/* 2017-06-22  21:57:23 定义几组特定的消息类型宏[msg0],APP自己定义消息类型时不要和这几个常量冲突*/
/* 企业内外BS和BC间消息类型定义 */
const DWORD QMSG0_TYPE_CONST_INFO_BSC_CMD = 0xFFFFFFFE;    /* 命令:查询信息类型的消息类型 */
const DWORD QMSG0_TYPE_CONST_INFO_BSC_RSP = 0xFFFFFFFD;    /* 响应:查询信息类型的消息类型 */
/* 喆道服务后台bss和BS间消息类型定义 */
const DWORD QMSG0_TYPE_CONST_INFO_CSG_CMD = 0xFFFFFFFC;    /* 命令:查询信息类型的消息类型 */
const DWORD QMSG0_TYPE_CONST_INFO_CSG_RSP = 0xFFFFFFFB;    /* 响应:查询信息类型的消息类型 */

// 消息队列的内容
#pragma pack(4)
struct DCFQUEMSG
{
    DWORD     msg[3];      // msg[0] 通常用来描述消息类型  msg[1]&msg[2]可以用户自定义,一般来说msg[2]指消息长度
    DCFLPARAM Ptrmsg;  // 通常是消息内容指针
};
#pragma pack()

/* 2017-06-22  22:2:46 信息类型帧的邮箱入口*/
const BYTE MAIL_ENTRY_CONST_INFO_RSP = 1;      /* 接收信息类型响应报文的入口 */
/*定义消息地址*/
struct MAIL_BOX_ADDR
{
    // 地址:(服务器ID(20bit 65535*16个),进程id(12bit 1024*4)
    DWORD SysID;
    // 模块ID
    WORD   ModuleID;
    // 对应模块的邮箱入口ID
    BYTE   MailEntryID;
    // 保留
    BYTE   Res;
};

/* 以下是各个模块对应msg0的定义 */
// 内核模块
const BYTE      MSG_ENTRY_KERNEL_MAIN = 1;
const DWORD MSG_TYPE_KERNEL_TASK_DEL = 0x1;

// BSS模块
const BYTE      MSG_ENTRY_BSS_MAIN = 1;
const DWORD QMSG0_TYPE_BSS_FROMBSG = 0x1;
#endif

