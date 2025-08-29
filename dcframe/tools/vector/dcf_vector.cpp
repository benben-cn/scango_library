
#include "dcf_vector.h"
#include "dcf.h"
/*
函 数 名：构造函数
参        数：node_len:用户期望的节点长度
                          min_size:vector的最小长度
                          step_size:分配节点的步长
输出参数:
返回参数:
作    者：zjb
时    间：2017-4-26 15:24
*/
dcf_vector::dcf_vector(int  node_len,int min_size,int step_size)
{
    if (node_len < sizeof(BYTE))
    {
        node_len = sizeof(BYTE);
    }

    if (min_size < 4)
    {
        min_size = 4;
    }

    if (step_size < 1)
    {
        step_size = 1;
    }

    m_node_len = node_len;
    m_node_save_len = node_len ;
    if (m_node_save_len > sizeof(WORD))
    {
        // 大于2个字节之后，按照DWORD字长倍数分配
        m_node_save_len = ((m_node_save_len + sizeof(DWORD) - 1)/sizeof(DWORD))*sizeof(DWORD);
    }

    m_pData = NULL;
    int iBufferLen = m_node_save_len * min_size;
    m_pData = (BYTE*)dcf_mem_malloc(iBufferLen);
    memset(m_pData,0,iBufferLen);
    m_min_size = min_size;
    m_malloc_size = min_size;
    m_used_size = 0;
    m_node_size_step = step_size;
}

dcf_vector::~dcf_vector()
{
    if (m_pData)
    {
        dcf_mem_free((void*&)m_pData);
    }
}

/*
函 数 名：校验尺寸
参        数：new_size:期望访问的节点
输出参数:
返回参数:
作    者：zjb
时    间：2017-4-26 15:24
*/
void dcf_vector::ensure_size(int n)
{
    if (n < m_malloc_size)
    {
        // 还够用
        return;
    }

    // 不够用了
    adjust_size(n + 1);
}

/*
函 数 名：以新的尺寸调整缓冲区
参        数：new_size:期望的大小
输出参数:
返回参数:
作    者：zjb
时    间：2017-4-26 15:24
*/
void dcf_vector::adjust_size(int new_size)
{
    if (new_size < m_min_size)
    {
        new_size = m_min_size;
    }

    int need_size = ((new_size + m_node_size_step - 1)/m_node_size_step)*m_node_size_step;
    if (m_malloc_size == need_size)
    {
        // 是一样的，无需变更
        return;
    }

    // 数据移动
    m_malloc_size = need_size;
    need_size = need_size * m_node_save_len;
    BYTE *pnewdata = (BYTE*)dcf_mem_malloc(need_size);
    memset(pnewdata,0,need_size);
    memcpy(pnewdata,m_pData, m_node_save_len*m_used_size);
    dcf_mem_free((void*&)m_pData);
    m_pData = pnewdata;
}

void dcf_vector::adjust_used_size(int n)
{
    if ((n + 1) > m_used_size)
    {
        m_used_size = n + 1;
    }
}

void* dcf_vector::operator [] (int n)
{
    return get(n);
}

void*  dcf_vector::get(int n)
{
    if (n < 0) n = 0;
    ensure_size(n);
    adjust_used_size(n);
    return get_pt(n);
}

void *dcf_vector::get_pt(int n)
{
    return &m_pData[n*m_node_save_len];
}

/*
函 数 名：删除节点
参        数：n:删除的位置
输出参数: puserdata用户数据,长度必须是用户配置的长度
返回参数:
作    者：zjb
时    间：2017-4-26 15:24
*/
void dcf_vector::remove_at(int n,DCF_FUNCPTR pfunc_clear_content)
{
    if (n >= m_used_size)
    {
        return;
    }

    if (pfunc_clear_content)
    {
        // 调用其回调函数进行删除前数据处理
        pfunc_clear_content(get_pt(n));
    }

    int left_num = m_used_size - n - 1;
    if (left_num)
    {
        // 数据移动
        memmove(get_pt(n),get_pt(n + 1),left_num*m_node_save_len);
    }

    // 将最后一个节点清空
    memset(get_pt(m_used_size),0,m_node_save_len);

    // 调整大小
    m_used_size = (m_used_size>0)?(m_used_size - 1):0;
    // 伸缩
    adjust_size(m_used_size);
}

void dcf_vector::remove_and_copy(int n,void *pcopydata)
{
    if (n >= m_used_size)
    {
        return;
    }

    if (pcopydata)
    {
        // 调用其回调函数进行删除前数据处理
        memcpy(pcopydata,get_pt(n),m_node_len);
    }
    remove_at(n);
}

/*
函 数 名：删除所有节点
参        数：pfunc_clear_content:节点信息清除函数
输出参数: 无
返回参数:
作    者：zjb
时    间：2017-4-27 9:27
*/
void dcf_vector::remove_all(DCF_FUNCPTR pfunc_clear_content)
{
    if (pfunc_clear_content)
    {
        for(int i = 0;i<m_used_size;i++)
        {
            pfunc_clear_content(get_pt(i));
        }
    }
    memset(m_pData,0,m_malloc_size*m_node_save_len);
    adjust_size(m_min_size);
    m_used_size = 0;
}


/*
函 数 名：交换对象的值
参        数：n1:第一个节点数据
                          n2:第二个节点数据
输出参数: 
返回参数:
作    者：zjb
时    间：2017-4-26 15:24
*/
void dcf_vector::exchange(int n1, int n2)
{
    // 先确保空间是OK的，否则地址会发生变更
    ensure_size(n1);
    ensure_size(n2);
    adjust_used_size(n1);
    adjust_used_size(n2);

    BYTE *pTemp = (BYTE*)dcf_mem_malloc(m_node_save_len);
    memcpy(pTemp,get_pt(n1),m_node_save_len);
    memcpy(get_pt(n1),get_pt(n2),m_node_save_len);
    memcpy(get_pt(n2),pTemp,m_node_save_len);
    dcf_mem_free((void*&)pTemp);
}

void dcf_vector::insert (int n,void*pnewdata)
{
    if (n >= m_used_size)
    {
        // 拷贝即可
        memcpy(get(n),pnewdata,m_node_len);
        return;
    }

    // 收下确保空间OK
    ensure_size(m_used_size);

    int copylen = (m_used_size - n )*m_node_save_len;
    memmove(get_pt(n+1),get_pt(n),copylen);
    memcpy(get_pt(n),pnewdata,m_node_len);
    m_used_size++;
}
void dcf_vector::replace(int n,void*pnewdata)
{
    memcpy(get(n),pnewdata,m_node_len);
}

void dcf_vector::push(void *p)
{
    void *pDst = get(m_used_size);
    memcpy(pDst,p,m_node_len);
}
/*
函 数 名：用户自定义查找函数
参        数：pkey自定义查找关键字
              ptr_find_func:查找函数
              dwKeyType:自定义查找类型
输出参数: 
返回参数: 节点位置
作    者：zjb
时    间：2017-5-1 20:39
*/
int dcf_vector::find(void *pkey,DCF_FUNCPTR ptr_find_func,DWORD dwKeyType)
{
    if (!ptr_find_func || !pkey)
    {
        return -1;
    }

    for(int i = 0;i < m_used_size;i++)
    {
        if (!ptr_find_func(get_pt(i),pkey,dwKeyType))
        {
            return i;
        }
    }

    return -1;
}

/*******************************************************************************************************
********************************************************************************************************/
// http://blog.csdn.net/hancunai0017/article/details/7032383

CPtrVector::CPtrVector(ITEM_CMP cmpFun, GET_KEY getKey, DEL_PTR delFun)
{
    m_cmpFun = cmpFun;
    m_getKey = getKey;
    m_delFun = delFun;
}


CPtrVector::~CPtrVector()
{
    clear();
}

void CPtrVector::push_tail(void *p)
{
    m_Data.m_Items.push_back(p);
}
void *CPtrVector::operator[](DWORD i)
{
    if (i >= m_Data.m_Items.size())
    {
        return NULL;
    }
    return (void*)m_Data.m_Items[i];
}
DWORD CPtrVector::size()
{
    return (DWORD)m_Data.m_Items.size();
}

void CPtrVector::new_nodes(int inums)
{
            ASSERT(inums >= 0);
    inums += (int)m_Data.m_Items.size();
    if (m_Data.m_Items.capacity() < (size_t)inums)
    {
        // 这个函数可以改变当前缓存节点数，但不影响size等函数，可以用来避免多次申请内存
        m_Data.m_Items.reserve(inums);
    }
}
bool CPtrVector::replace(DWORD i, void *p)
{
    if (i >= m_Data.m_Items.size())
    {
        return false;
    }
    m_Data.m_Items[i] = p;
    return true;
}

void CPtrVector::insert(int ibefore, void *p)
{
    if (ibefore > (int)m_Data.m_Items.size())
    {
        m_Data.m_Items.push_back(p);
    }
    else
    {
        m_Data.m_Items.insert(m_Data.m_Items.begin() + ibefore, p);
    }
}

void CPtrVector::clear()
{
    for (;;)
    {
        void *p = remove_at(0);
        if (!p)
        {
            break;
        }

        if (m_delFun)
        {
            m_delFun(p);
        }
    }
}
void *CPtrVector::remove_at(int iPt)
{
    if (iPt >= (int)m_Data.m_Items.size())
    {
        return NULL;
    }
    void *p = m_Data.m_Items[iPt];
    m_Data.m_Items.erase(m_Data.m_Items.begin() + iPt);
    return p;
}

int CPtrVector::FindItem(ITEMKEY key,int *simPt)
{
    DWORD dwNums = (DWORD)m_Data.m_Items.size();
    int left, right, mid;
    mid = -1;
    if ((!dwNums) || (!m_cmpFun))
    {
        goto ret;
    }

    left = 0;
    right = dwNums - 1;
    while (left <= right)
    {
        mid = left + (right - left) / 2;
        int iResult = m_cmpFun(m_Data.m_Items[mid], key);
        if (iResult == 0)
        {
            if (simPt) {*simPt = mid;}
            return mid;
        }
        if (iResult > 0) right = mid - 1;
        else left = mid + 1;
    }

    ret:
    if (simPt)
    {
        *simPt = mid;
    }

    return -1;
}

/****************************************************************
*功能描述 : 按序插入节点
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-12  8:54:39
****************************************************************/
void CPtrVector::insert_sort(void *p)
{
            ASSERT(m_getKey != NULL);
            ASSERT(m_cmpFun != NULL);
    ITEMKEY key = m_getKey(p);
    int iSimPt;
    int iPt = FindItem(key,&iSimPt);
    if (iSimPt < 0)
    {
        push_tail(p);
        return;
    }

    if (m_cmpFun(m_Data.m_Items[iSimPt],key) >= 0)
    {
        // 将该节点插入在对应节点前面
        insert(iSimPt,p);
    }
    else
    {
        // 将该节点插入在对应节点后面
        insert(iSimPt + 1,p);
    }
}

/****************************************************************
*功能描述 : 对外排序接口
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-11  9:32:17
****************************************************************/
void CPtrVector::sort()
{
    if ((!m_cmpFun) || (!m_getKey))
    {
                ASSERT(0);
        return;
    }

#if USE_QUICK
    sort_quick(0, m_Data.m_Items.size() - 1);
#else
    sort_heap();
#endif
}
void CPtrVector::sort_swap(int pt1, int pt2)
{
    void *ptmp = m_Data.m_Items[pt1];
    m_Data.m_Items[pt1] = m_Data.m_Items[pt2];
    m_Data.m_Items[pt2] = ptmp;
}
#if USE_QUICK
/****************************************************************
*功能描述 : 采用快速排序法，使用递归，不能进行大数据量的排序，否则会栈溢出
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-09-11  9:31:27
****************************************************************/

void CPtrVector::sort_quick(int left, int right)
{
    if (left >= right)
    {
        return;
    }

    int privotLoc = sort_partition(left, right);  //将表一分为二
    sort_quick(left, privotLoc - 1);          //递归对低子表递归排序
    sort_quick(privotLoc + 1, right);         //递归对高子表递归排序
}

int CPtrVector::sort_partition(int left, int right)
{
    ASSERT(m_getKey != NULL);
    ASSERT(m_cmpFun != NULL);
    ITEMKEY privotKey = m_getKey(m_Data.m_Items[left]);
    while (left < right)
    {
        while (left < right  && (m_cmpFun(m_Data.m_Items[right], privotKey)>=0)) --right;
        sort_swap(left, right);
        while (left < right  && (m_cmpFun(m_Data.m_Items[left], privotKey)<=0)) ++left;
        sort_swap(left, right);
    }
    return left;
}
#else
/****************************************************************
*功能描述 : 采用堆排序法
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-09-11  9:30:50
****************************************************************/
void CPtrVector::sort_heap()
{
    // from:http://blog.csdn.net/hguisu/article/details/7776068
    // 1.初始堆,调整完之后第一个元素是序列的最小的元素
    int length = (int)m_Data.m_Items.size();
    for (int i = length / 2 - 1; i >= 0; --i)
    {
        sort_heap_adjust(i, length);
    }
    // 2.从最后一个元素开始对序列进行调整
    for (int i = length - 1; i > 0; --i)
    {
        // 交换堆顶元素H[0]和堆中最后一个元素
        sort_swap(0, i);
        //每次交换堆顶元素和堆中最后一个元素之后，都要对堆进行调整
        sort_heap_adjust(0, i);
    }
}

/**
* 已知H[s…m]除了H[s] 外均满足堆的定义
* 调整H[s],使其成为大顶堆.即将对第s个结点为根的子树筛选,
*
* @param s是待调整的数组元素的位置
* @param length是数组的长度
*
*/
void CPtrVector::sort_heap_adjust(int s, int length)
{
    void *pTemp = m_Data.m_Items[s];
    int  child = 2 * s + 1;  // 左孩子结点的位置。(i+1为当前调整结点的右孩子结点的位置)
    while (child < length)
    {
        if ((child + 1 < length) && (m_cmpFun(m_Data.m_Items[child], m_getKey(m_Data.m_Items[child+1])) < 0)) //H[child] < H[child + 1])
        {
            ++child;
        }
        if (m_cmpFun(m_Data.m_Items[s], m_getKey(m_Data.m_Items[child])) >= 0)//(H[s]<H[child])
        {
            // 如果当前待调整结点大于它的左右孩子，则不需要调整，直接退出
            break;
        }
        // 如果较大的子结点大于父结点

        m_Data.m_Items[s] = m_Data.m_Items[child]; // 那么把较大的子结点往上移动，替换它的父结点
        s = child;  // 重新设置s ,即待调整的下一个结点的位置
        child = 2 * s + 1;
        m_Data.m_Items[s] = pTemp;  // 当前待调整的结点放到比其大的孩子结点位置上
    }
}
#endif

