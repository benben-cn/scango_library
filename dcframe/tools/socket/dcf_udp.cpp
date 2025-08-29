
/****************************************************************
*文件范围 : 框架udp的实现文件
*设计说明 : NA
*注意事项 : NA
*作   者 : zjb
*创建日期 : 2017-05-17  21:54:28
****************************************************************/


#include "dcf_udp.h"

#if _SW_DCF_WINDOWS
#include <winsock.h>
#endif
//#include <Mstcpip.h>
#include "extern_api.h"
#include "dcf_err.h"
#include "dcf_pkghelper.h"
#include "dcf_rcc_dthelper.h"
#include "dcf_string.h"
#if _SW_DCF_MACOS
#include <sys/socket.h>
#endif
//#pragma comment(lib,"WS2_32.lib")
#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)

CDcfUDP::CDcfUDP()
{
    m_socket = NULL;
    m_Buff_ProcPt = 0;
    m_Buff_ReadLen = 0;

    m_RecvTotalFrames = 0;
    m_RecvOkFrames = 0;
    m_RecvErrFrames = 0;
    m_SendTotalFrames = 0;
    m_SendOkFrames = 0;
    m_SendErrFrames = 0;
}

CDcfUDP::~CDcfUDP()
{
    Close();
}

void CDcfUDP::Close()
{
#if _SW_DCF_WINDOWS
    if (m_socket != NULL)
    {

        closesocket(m_socket);
        m_socket = NULL;

    }
#endif
#if _SW_DCF_MACOS
    if (m_socket != 0)
    {
        close(m_socket);
        m_socket = 0;
    }
#endif


}

DWORD CDcfUDP::PreInit()
{
#if _SW_DCF_WINDOWS
    WSADATA wsd;
    // 初始化套接字动态库
    if(WSAStartup(MAKEWORD(2,2),&wsd) != 0)
    {
        dcf_output("WSAStartup failed !\r\n");
        return RCC_ERR_SOCKET;
    }
#endif
    return RCC_SUCCESS;
}

DWORD CDcfUDP::Initialize(DWORD recvIP,WORD recvPort)
{
    CATCH_ERR_RET(m_socket,RCC_ERR_FAILED);
    DWORD dwRet = PreInit();
    CATCH_ERR_RET(dwRet,dwRet);

    m_socket = socket(AF_INET, SOCK_DGRAM, 0); // 创建数据报
    CATCH_ERR_RET(m_socket == INVALID_SOCKET,RCC_ERR_SOCKET);
    /* 2017-07-21  14:31:1 修改recvfrom返回10054问题 */
    BOOL bNewBehavior = FALSE;
    DWORD dwBytesReturned = 0;
#if _SW_DCF_WINDOWS
    WSAIoctl(m_socket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

    char chbuffer[16];
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;   // IP协议
    addr.sin_port = recvPort; // 端口
    addr.sin_addr.s_addr = recvIP; // 在本机的所有ip上开始监听
    if (bind(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
#if _SW_DCF_WINDOWS
        closesocket(m_socket);
        m_socket = NULL;
#endif
#if _SW_DCF_MACOS
        close(m_socket);
        m_socket = 0;
#endif

        dcf_output("bind (%s,%d) failed!\r\n", dcf_strtools::IPtoStr(ntohl(recvIP), chbuffer), ntohs(recvPort));
        return RCC_ERR_SOCKET;
    }

    dcf_output("bind (%s,%d) success!\r\n", dcf_strtools::IPtoStr(ntohl(recvIP), chbuffer), ntohs(recvPort));

    memset(m_chRecvBuf,0,sizeof(m_chRecvBuf));
    m_Buff_IP = 0;
    m_Buff_Port = 0;
    m_Buff_ReadLen = 0;
    m_Buff_ProcPt = 0;
    return RCC_SUCCESS;
}

/****************************************************************
*功能描述 : 发送帧数据
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-18  16:35:51
****************************************************************/
DWORD CDcfUDP::SendFrame(void *pData,DWORD len,DWORD toIP,WORD toPort)
{
    sockaddr_in ser;    // 服务器端地址
    ser.sin_family = AF_INET;   // IP协议
    ser.sin_port = toPort;    // 端口号
    ser.sin_addr.s_addr = toIP;    // IP地址
    int nLen = sizeof(ser); // 服务器地址长度
    m_SendTotalFrames ++;
    int iSendLen = sendto(m_socket, (char*)pData, len, 0, (sockaddr*)&ser, nLen);
    if (iSendLen != len)
    {
        m_SendErrFrames ++;
        dcf_output("sendframe failed!(%d,%d)\r\n",len,iSendLen);
        return RCC_ERR_FAILED;
    }

    m_SendOkFrames ++;
    return RCC_SUCCESS;
}

/****************************************************************
*功能描述 : 收socket数据，函数设计最大的难点:在缓存中的一堆数据中，含有众多用户的数据，如果没有非法数据，则
                   很好处理
*输入参数 : timeout : 超时时间
*输出参数 : timeout : 第一次读到数据之后会清0，后面连续读，不会再等待
*返回参数 : NA
*作   者 :     zjb
*创建日期 : 2017-05-18  11:42:48
****************************************************************/
DWORD CDcfUDP::RecvFrame(void *pBuffer,DWORD buflen,DWORD &dataLen,DWORD &dataIP,WORD &dataPort,DWORD &timeout)
{
    CATCH_ERR_RET(!m_socket,DCF_ERR_SYS_NONE_INIT);
    CATCH_ERR_RET((buflen <sizeof(CRccHeader)),DCF_ERR_PARAM);
            ASSERT(pBuffer != NULL);

    for(;;)
    {
        if (!SearchFrame((BYTE*)pBuffer,buflen, dataLen))
        {
            // 找到了节点
            dataIP = m_Buff_IP;
            dataPort = m_Buff_Port;
            timeout = 0;
            m_RecvOkFrames++;
            return RCC_SUCCESS;
        }

        // 读SOCKET缓存中的数据到本地缓存中
        DWORD dwRet = ReadRawData(timeout);
        /* 只有第一次读数据需要等待，其它时候都是只是看数据区/socket的缓冲区是否有数据 */
        timeout = 0;

        if(dwRet)
        {
            // socket缓存中没有数据了，那么类的缓存中的数据也可以清掉了
            // 理由:到达socket中的数据都是以完整帧到达的，不会存在半截的问题，很可能是因误码等原因导致
            if (m_Buff_ReadLen)
            {
                dcf_output("socket err:0x%x,reject the left data(ip:0x%08x,port:%d,len:%d,pt:%d)\r\n",dwRet,m_Buff_ProcPt,m_Buff_Port,m_Buff_ReadLen,m_Buff_ProcPt);
            }

            m_Buff_ProcPt = 0;
            m_Buff_ReadLen = 0;
            m_Buff_ProcPt = 0;
            return dwRet;
        }
    }

    // 不会走到这里
    dcf_output("?????\r\n");
    return 0;
}

/****************************************************************
*功能描述 : 在缓存中查找节点，并将信息拷贝到pBuffer中.该函数会调整缓存相关变量
*输入参数 : NA
*输出参数 : pBuffer帧信息
*返回参数 : 0:找到了节点 1:未找到节点
*作   者 :     zjb
*创建日期 : 2017-05-18  13:43:28
****************************************************************/
DWORD CDcfUDP::SearchFrame(BYTE *pBuffer,DWORD buflen,DWORD &dataLen)
{
    dataLen = 0;
    if (!m_Buff_ReadLen)
    {
        return RCC_ERR_EMPTY;
    }

    DWORD dwRet = RCC_ERR_EMPTY;
    /* 这里弄一个for循环是为了解决找到合法头部，但数据帧校验不过，还需要继续搜索的场景，不搞for循环，那么就会出现递归调用 */
    WORD HeadPt = 0;
    for(;;)
    {
        // 找疑似的头部
        HeadPt = CRccHeaderHelper::search_header(m_chRecvBuf,pBuffer,m_Buff_ProcPt,m_Buff_ReadLen);
        if (HeadPt >= m_Buff_ReadLen)
        {
            // 肯定是没有帧头了，数据需要全部丢弃
            m_Buff_ReadLen = 0;
            m_Buff_ProcPt = 0;
            return RCC_ERR_EMPTY;
        }

        if (HeadPt + sizeof(CRccHeader) > m_Buff_ReadLen)
        {
            // 需要跳到外面做数据搬移操作
            break;
        }

        // 找到一个合法的头部了，可以看看数据校验是否可以通过
        // 此时数据头一定转换在pBuffer中了
        CRccHeader *pHead = (CRccHeader*)pBuffer;

        if (pHead->PackageLen > (RCC_SIZE_TOTAL - sizeof(CRccHeader)))
        {
            // 消息帧长度错误，跳过魔术字继续找
            m_Buff_ProcPt+= sizeof(DWORD);
            m_RecvErrFrames++;
            m_RecvTotalFrames++;
            continue;
        }

        // 先看长度是否足够
        if ((pHead->PackageLen + HeadPt + sizeof(CRccHeader)) > m_Buff_ReadLen)
        {
            // 这里有可能是前面读的数据长度不够需要继续读数据再校验
            // 当然也不排除是长度有问题
            break;
        }

        // 计算数据区的CRC16校验和
        WORD wCalcCRC16 = dcf_tools_crc16_get(m_chRecvBuf+HeadPt + sizeof(CRccHeader),pHead->PackageLen);
        if (pHead->DataCrc16 != wCalcCRC16)
        {
            // 数据校验不过
            m_Buff_ProcPt+= sizeof(DWORD);
            m_RecvErrFrames++;
            m_RecvTotalFrames++;
            continue;
        }

        // 是一个合法帧
        if ((pHead->PackageLen + sizeof(CRccHeader)) <= buflen)
        {
            // 缓存也够，那么就把数据拷贝进去
            memcpy(pBuffer + sizeof(CRccHeader),m_chRecvBuf+HeadPt + sizeof(CRccHeader),pHead->PackageLen);
            dataLen = pHead->PackageLen + sizeof(CRccHeader);
            dwRet = 0;
//            pBuffer[dataLen] = 0; /* 2017-9-26 17:04:03 会导致写越界 */
        }
        else
        {
            dwRet = RCC_ERR_BUFFER_LEN;
            m_RecvErrFrames++;
            m_RecvTotalFrames++;
        }

        // 移动跳过搜索的帧头
        HeadPt += pHead->PackageLen + sizeof(CRccHeader);
        m_Buff_ProcPt = HeadPt;
        // break掉，到外面进行移动处理
        break;
    }

    if (HeadPt && HeadPt <  m_Buff_ReadLen)
    {
        memmove(m_chRecvBuf,m_chRecvBuf+HeadPt,m_Buff_ReadLen - HeadPt);
        m_Buff_ProcPt = 0;
        m_Buff_ReadLen -= HeadPt;
        m_chRecvBuf[m_Buff_ReadLen] = 0;
    }

    return dwRet;
}





/****************************************************************
*功能描述 : 从SOCKET缓存中读取数据
*输入参数 : NA
*输出参数 : NA
*返回参数 : NA
*作   者 : zjb
*创建日期 : 2017-05-18  15:31:17
****************************************************************/
DWORD CDcfUDP::ReadRawData(DWORD timeout)
{
    // 正常不会出现这种情况
            ASSERT(m_Buff_ProcPt < RCC_SIZE_TOTAL);
    timeval tout;    // 定时变量
    fd_set rfd;     // 读描述符集
    int maxfdp = (int)m_socket  + 1;
    // 设置超时时间为6s
    tout.tv_sec = 0;// timeout/1000;
    tout.tv_usec = timeout*1000;  /* 应用层的单位是毫秒,socket是微妙 */

    FD_ZERO(&rfd);
    FD_SET(m_socket, &rfd);
    /* windows版本的第一个参数忽略，但linux需要填写最大句柄集的值+1 */
    int nRet = select(maxfdp, &rfd, NULL, NULL, &tout);  /* 检查可读 */
    if (nRet == SOCKET_ERROR)
    {
        dcf_output("select error,select!\r\n");
        return RCC_ERR_SOCKET;
    }
    else if(nRet == 0) /* 正常的超时 */
    {
        return RCC_ERR_TIMEOUT;
    }

    // 下面是有数据的情形
    if (!FD_ISSET(m_socket, &rfd))
    {
        return RCC_ERR_EMPTY;
    }

    sockaddr_in client;
    int nRecLen = sizeof(client);
#if _SW_DCF_MACOS
    int nRecEcho = recvfrom(m_socket, (char*)m_chRecvBuf + m_Buff_ReadLen, sizeof(m_chRecvBuf) - m_Buff_ReadLen , 0, (sockaddr*)&client,(socklen_t*)&nRecLen);

    if (nRecEcho == -1)
    {
        dcf_output("socket error(0x%x,%s)\r\n", m_socket, errno);
        return RCC_ERR_SOCKET;
    }
#endif

#if _SW_DCF_WINDOWS
    int nRecEcho = recvfrom(m_socket, (char*)m_chRecvBuf + m_Buff_ReadLen, sizeof(m_chRecvBuf) - m_Buff_ReadLen , 0, (sockaddr*)&client, &nRecLen);

    if (nRecEcho == INVALID_SOCKET)
    {
        dcf_output("socket error(0x%x,%d)\r\n", m_socket, WSAGetLastError());
        return RCC_ERR_SOCKET;
    }
#endif

    if (nRecEcho <= 0)
    {
        // 有消息，但没有数据?
        dcf_output("recvfrom null len:%d\r\n",nRecEcho);
        return 0;
    }

    if (!m_Buff_ReadLen)
    {
        // 以前就没有数据
#if _SW_DCF_MACOS
        m_Buff_IP = (DWORD)client.sin_addr.s_addr;
#endif
#if _SW_DCF_WINDOWS
        m_Buff_IP = (DWORD)client.sin_addr.S_un.S_addr;
#endif
        m_Buff_Port = client.sin_port;
        m_Buff_ReadLen = nRecEcho;
        m_Buff_ProcPt = 0;
        return 0;
    }

            ASSERT(m_Buff_IP != 0);
            ASSERT(m_Buff_Port != 0);

    // 看一下IP是否一样，否则以前的数据需要丢弃
#if _SW_DCF_WINDOWS
    if ((client.sin_addr.S_un.S_addr == m_Buff_IP) && (client.sin_port != m_Buff_Port))
    {
        // 端口还是一样的，不用进行下面的异常处理
        m_Buff_ReadLen += nRecEcho;
        return 0;
    }
#endif
#if _SW_DCF_MACOS
    if ((client.sin_addr.s_addr == m_Buff_IP) && (client.sin_port != m_Buff_Port))
    {
        // 端口还是一样的，不用进行下面的异常处理
        m_Buff_ReadLen += nRecEcho;
        return 0;
    }
#endif



    // 更换了IP或者端口，原来的数据需要丢弃
    dcf_output("ip or port is changed ,we must reject the data!(len:%d,from:%d),(org:0x%08x,%d),(new:0x%08x,%d)\r\n", \
                            m_Buff_ReadLen,m_Buff_ProcPt, \
                            m_Buff_IP,m_Buff_Port,client.sin_addr,client.sin_port);
    memmove(m_chRecvBuf,m_chRecvBuf+m_Buff_ReadLen,nRecEcho);
    m_Buff_ReadLen = (WORD)nRecEcho;
    m_Buff_ProcPt = 0;
#if _SW_DCF_MACOS
    m_Buff_IP = client.sin_addr.s_addr;
#endif
#if _SW_DCF_WINDOWS
    m_Buff_IP = client.sin_addr.S_un.S_addr;
#endif
    m_Buff_Port = client.sin_port;
    m_RecvErrFrames++;
    m_RecvTotalFrames++;
    return 0;
}










