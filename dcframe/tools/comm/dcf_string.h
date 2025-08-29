#ifndef dcf_string_h
#define dcf_string_h

/*
文件说明:该文件包含字符串的一些常用高效函数
作者：zjb
时间：2014-4-25 22:23
*/

class dcf_strtools
{
public:
    static const char* dcf_split_string(const char *pstr, char chsplit, int *ptr_pos_array = NULL, int *strlen_array = NULL, int size_array = 0, int *array_num = NULL);
    // 字符串中搜索引号(可以跳过转义引号)
    static const char* search_quote(const char *pstr);
    // 跳过空格
    static const char* skip_space(const char *pstr)
    {
        if (!pstr) return NULL;
        while ((*pstr == ' ') || (*pstr == '\t')) pstr++;
        return pstr;
    }
    /* 将字符串中的字符替换掉 */
    static char *replace_char(char *pstr,char srcchar,char dstchar);

    // 扫描到本行的结束(下一行的开始)
    static const char *skip_to_line_end(const char *pstr);
    // 跳过注释行(可以跳多行)
    static const char *skip_note_line(const char *pstr);
    // 从原字符串流中拷贝指定长度的字符串
    static const char* get_string(const char*src_str, char*dst_str, int ifromPt_src, int iLen_src, int ibufferLen_dst);
    // 支持自动识别10进制和16进制
    static long strtol(const char *pstr,long defvalue = 0,char **pEnd = NULL);
    // 是SQL的空字符串
    static const char *filter_sqlnull_str(const char *pstr);
    // 通过malloc方式申请一块内存并复制字符串
    static char *strmalloc(const char *pstr,int limit_len = 0,bool bIgnoreSqlNull = false);
    // 去掉注释行(支持两种注释行; 和// 这两种,不支持)
    static void reject_note_line(char *src_str,DWORD *len = NULL);
    // 安全添加函数:pbuffer是buffer的首地址,append_pt是连续追加的指针
    // 函数返回为空，表示缓存不够，否则返回pbuffer首地址
    static char *strcat(char *pbuffer,char *psrc,int &append_pt,int buffer_len = 0);
    // 安全的字符串拷贝(保证结尾有0)
    static int strcpy_s(char *pbuffer,const char *psrc,int buffer_len);
    /* 获取短文件名 */
    static const char *get_short_filename(const char *pfilename);
    /* IP地址转换函数 */
    static char * IPtoStr(DWORD dwIP,char chbuffer[16]);
    /* 特别注意:这个IP地址是主机字节序的,如果是网络的，还需要调用htonl */
    static DWORD StrtoIP(char *p,char **pEnd = NULL);
    /* 日期比较 小于-1 等于 0 大于1*/
    static int datetime_compare(const char* time1, const char* time2);
    /* 一组时间和序号的组合函数，各占4个字节 */
    static DWORD date_strtol(char chbuffer[32]);
    static void date_ltostr(DWORD v,char chbuffer[32]);
    /* 64位序号 */
    static uint64 uint64_strtov(DWORD no_t,char chbuffer[32]);
    static void uint64_vtostr(uint64 v,DWORD &no_t,char chbuffer[32]);
    static uint64 uint64_combine(DWORD dt_v,DWORD idx_v);
    static void uint64_split(uint64 v,DWORD &dt_v,DWORD &idx_v);
    /* 32位序号 年份减去2015 */
    static DWORD uint_strtov(DWORD idx_t,char chbuffer[32]);
    static void uint_vtostr(DWORD v,DWORD &idx_t,char chbuffer[32]);
    static DWORD uint_combine(DWORD dt_v,DWORD idx_v);
    static void uint_split(DWORD v,DWORD &dt_v,DWORD &idx_v);
};

// 下面这个类用于字符串遍历查找，没有申请，效率非常高
#define MAX_ITEM_LEN 511 // 支持的值项最大长度(不含0)
class dcf_strsearch
{
public:
    dcf_strsearch(const char*pstr, char chsplit);
    ~dcf_strsearch() {};
    const char *get_first(int *pstrlen = NULL);
    const char *get_next(int *pstrlen = NULL);
protected:
    const char *find(int *pstrlen);
protected:
    const char *m_pstr_org;  // 原始字符串
    const char *m_pstr_cur;  // 当前查找的游标处
    char  m_chsplit;         // 分解符
    char  m_chbuffer[MAX_ITEM_LEN + 1];
};

// 2017.5.4 22:20 zjb
// 封装这个函数的目的:方便进行字符做拼接，并自动释放内存
// 这个函数有如下几个坑，一定要防范好:
// 1.构造函数抛异常:原因是最后忘记输入0(极其重要)了，必须是如下样式:dcfstring dt("str1","str2",0);
// 2.字符串变成了乱码(实际上是野指针):使用了无名构造，而且字符串长度超过了缺省的128,申请的内存被析构函数释放了
//   无名构造对象的方式:   char *p = dcfstring("SmartDogBaseEntity",0);  这种方式在这条语句完成时就会被析构，p得到的指针可能是无效
// 3.内存泄漏:detach时，没有保存和释放新申请的内存,特别注意：这个函数在原字符串小于128时，会新申请内存
// 这个类有这么多坑，为何还要推荐出来给大家用？其优势：
// 1.提供了便捷的字符串连接操作
// 2.效率高:普通的使用一般128个字符足矣，避免多次申请内存
// 推荐的正常使用方法:
// dcfstring dt;
// char *p = dt + "str1" + "str2" + "str3";

class dcfstring
{
public:
    dcfstring();
    // 下面构造函数的参数均应该是字符串类型
    // 特别注意，最后一个参数一定要写个0，否则会抛异常
    dcfstring(char *p,...);
    ~dcfstring();
    // 重载参数类型转换，返回字符串指针，该指针
    operator char*();
    dcfstring &operator +(char *pstr);
    // 分离字符串，使用者一定要自己释放内存，而且要刷新内存变量，否则变量可能是重新申请的内存
    char *detach();
    void release();
protected:
    int      m_strlen;
    // m_pbuffer指向m_fixbuffer，表示是固有内存，不能释放
    char *m_pbuffer;
    // 一般来说128个字节足够了
    char  m_fixbuffer[128];
};
#endif
