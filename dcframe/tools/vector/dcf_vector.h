#ifndef dcf_vector_h
#define dcf_vector_h

/*
文件说明:该文件是vector工具类
作者：zjb
时间：2014-4-26 13:55
*/

#include "dcf_def.h"
#include <vector>
using namespace std;

class dcf_vector
{
public:
    dcf_vector(int  node_len,int min_size = 4,int step_size = 4);
    ~dcf_vector();
    void* operator [] (int n);
    void* get(int n);
    int length(){return m_used_size;};
    // pfunc_clear_content格式为:void func(void *p);
    // 而且需要用户自身来判断数据的有效性(可能全是0)
    void remove_at(int n,DCF_FUNCPTR pfunc_clear_content = NULL);
    void remove_and_copy(int n,void *pcopydata = NULL);
    void remove_all(DCF_FUNCPTR pfunc_clear_content = NULL);
    void exchange(int n1, int n2);
    // 在n节点前插入新节点
    void insert (int n,void*pnewdata);
    void replace(int n,void*pnewdata);
    void push(void *p);
    // 查找函数，函数模型 DWORD compare(void *pdata,void *pkey,DWORD dwkeytype);
    int find(void *pkey,DCF_FUNCPTR ptr_find_func,DWORD dwKeyType = 0);
protected:
    void ensure_size(int n);
    void adjust_size(int new_size);
    void *get_pt(int n);
    void adjust_used_size(int n);
protected:
    // 分配节点的数量
    int         m_malloc_size;
    // 最小大小
    int         m_min_size;
    // 当前已经使用的数量
    int         m_used_size;
    // 每个节点的长度
    int         m_node_len;
    // 每个节点分配的长度(用4的倍数长度，可以加快数据的访问,64位系统需要8个字节，有点浪费，先以DWORD为准吧)
    int         m_node_save_len;
    // 每次增长的长度
    int         m_node_size_step;

    // 数据区
    BYTE  *m_pData;
};

/*
2017-09-11 下面是以stl封装的指针型vector，统一封装方便大家使用，也避免多种对象
// 2017.9.9 23:36
// 快速排序法的递归实现非常耗栈空间，不适用于大数据量排序；非递归版本，需要多次申请内存，麻烦
// 堆排序方法采用非递归实现，性能也不错，值交换比较多，适用于交换内容少的大数据量场景，如仅仅是索引、指针等
// 综合考虑后采用堆排序方法
*/
struct ITEMKEY
{
    union
    {
        BYTE bkey;
        WORD wkey;
        DWORD dwkey;
        int   ikey;
        void *ptrkey;
    };
    ITEMKEY(BYTE b){bkey = b;};
    ITEMKEY(WORD w){wkey = w;};
    ITEMKEY(DWORD d){dwkey = d;};
    ITEMKEY(int i){ikey = i;};
    ITEMKEY(void* p){ptrkey = p;};
    ITEMKEY &operator=(int i)
    {
        ikey = i;
        return *this;
    }
    ITEMKEY &operator=(BYTE i)
    {
        bkey = i;
        return *this;
    }
    ITEMKEY &operator=(DWORD i)
    {
        dwkey = i;
        return *this;
    }
    ITEMKEY &operator=(WORD i)
    {
        wkey = i;
        return *this;
    }
    ITEMKEY &operator=(void * p)
    {
        ptrkey = p;
        return *this;
    }
};

// 为了屏蔽告警，增加一个数据类：warning C4251: “CPtrVector::m_Items”: class“std::vector<void *,std::allocator<_Ty>>”需要有 dll 接口由 class“CPtrVector”的客户端使用
class CPtrData
{
public:
    vector<void*> m_Items;
};

typedef int(*ITEM_CMP)(void*p, ITEMKEY key);
typedef ITEMKEY(*GET_KEY)(void*p);
typedef void(*DEL_PTR)(void*p);
class CPtrVector
{
public:
    CPtrVector(ITEM_CMP cmpFun, GET_KEY getKey, DEL_PTR delFun);
    ~CPtrVector();
    void *operator[](DWORD i);
    // 下面这个函数，可以用来一次性分配指定数量的元素，可以减少内存申请的次数
    void new_nodes(int inums);
    // 替换元素值
    bool replace(DWORD i, void *p);
    // 插入数据之后，如果不能保证大小，则调用完成后进行排序
    void insert(int ibefore, void *p);
    void push_tail(void *p);
    // 采用改进之后的快速排序方法
    void sort();
    void insert_sort(void *p);
    // 返回元素大小
    DWORD size();
    // 查找函数
    int FindItem(ITEMKEY key,int *simPt = NULL);
    // 清除节点函数
    void* remove_at(int iPt);
    void clear();
protected:
    // 排序的内部函数
    void sort_swap(int pt1, int pt2);
#if USE_QUICK
    int sort_partition(int left, int right);
    void sort_quick(int low, int high);
#else
    void sort_heap();
    void sort_heap_adjust(int i, int nLength);
#endif
protected:
    CPtrData m_Data;
    // 搞成成员变量，减少排序的栈内存消耗
    ITEM_CMP m_cmpFun;
    GET_KEY  m_getKey;
    DEL_PTR  m_delFun;
};

#endif

