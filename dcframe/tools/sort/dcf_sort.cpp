
#include "dcf_sort.h"

/****************************************************************
*功能描述 : 缺省的排序比较方法
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-23  9:7:31
****************************************************************/
int IUserDataProc::CompareKey(DWORD key1,DWORD key2)
{
    if (key1 < key2) return -1;
    if (key1 > key2) return 1;
    return 0;
}

/****************************************************************
*功能描述 : 3路快速排序算法
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-23  10:8:3
****************************************************************/
void CQuickSortTools::quickSort3Way(BYTE* a,DWORD dwNodSize, int left, int right,IUserDataProc *Proc)
{
    if(left < right)
    {
        DWORD x = Proc->GetKey(a+right*dwNodSize);
        int i = left-1, j = right, p = left-1, q = right;
        for (;;)
        {
            while (Proc->CompareKey((Proc->GetKey(a+(++i)*dwNodSize)),x)< 0);
            while (Proc->CompareKey((Proc->GetKey(a+(--j)*dwNodSize)),x)> 0){if(j==left) break;}
            if(i < j)
            {
                Proc->SwitchNode(a+i*dwNodSize,a+j*dwNodSize);
                if (Proc->CompareKey((Proc->GetKey(a+i*dwNodSize)),x) == 0)
                {
                    p++;
                    Proc->SwitchNode(a+p*dwNodSize,a+i*dwNodSize);
                }

                if (Proc->CompareKey((Proc->GetKey(a+j*dwNodSize)),x) == 0)
                {
                    q++;
                    Proc->SwitchNode(a+q*dwNodSize,a+j*dwNodSize);
                }
            }
            else
            {
                break;
            }
        }

        Proc->SwitchNode(a+i*dwNodSize,a+right*dwNodSize);
        j = i-1; i=i+1;
        for (int k=left; k<=p; k++, j--) Proc->SwitchNode(a+k*dwNodSize,a+j*dwNodSize);
        for (int k=right-1; k>=q; k--, i++) Proc->SwitchNode(a+i*dwNodSize,a+k*dwNodSize);;

        quickSort3Way(a, dwNodSize, left, j, Proc);
        quickSort3Way(a, dwNodSize, i, right, Proc);
    }
}

/****************************************************************
*功能描述 : 快速查找方法
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-23  10:30:41
****************************************************************/
DWORD CQuickFindTools::quickFind2Way(BYTE *a,DWORD dwNodSize,DWORD numsSize,DWORD keyvalue,IUserDataProc *Proc)
{
    int mid;
    int left=0;
    int right=numsSize-1;

    while(left <= right)
    {
        mid=(left+right)/2;
        int iCompare = Proc->CompareKey((Proc->GetKey(a+mid*dwNodSize)),keyvalue);
        if(iCompare == 0)
        {
            return mid;
        }
        else if(iCompare > 0)
        {
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }

    return (DWORD)-1;
}

