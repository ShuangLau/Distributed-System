#include "DNSLookup.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

CDNSLookup::CDNSLookup() : 
    m_bIsInitOK(false), 
    m_sock(-1),
    m_szDNSPacket(NULL)
{
    m_bIsInitOK = Init();
}

CDNSLookup::~CDNSLookup()
{
    UnInit();
}

bool CDNSLookup::DNSLookup(unsigned long ulDNSServerIP, char *szDomainName, std::vector<unsigned long> *pveculIPList, std::vector<std::string> *pvecstrCNameList,uint16_t type, unsigned long ulTimeout, unsigned long *pulTimeSpent
    )
{
    return DNSLookupCore(ulDNSServerIP, szDomainName, pveculIPList, pvecstrCNameList, ulTimeout, pulTimeSpent,type);
}

bool CDNSLookup::DNSLookup(unsigned long ulDNSServerIP, char *szDomainName, std::vector<std::string> *pvecstrIPList, std::vector<std::string> *pvecstrCNameList,uint16_t type, unsigned long ulTimeout, unsigned long *pulTimeSpent)
{
    std::vector<unsigned long> *pveculIPList = NULL;
    if (pvecstrIPList != NULL)
    {
        std::vector<unsigned long> veculIPList;
        pveculIPList = &veculIPList;
    }

    bool bRet = DNSLookupCore(ulDNSServerIP, szDomainName, pveculIPList, pvecstrCNameList, ulTimeout, pulTimeSpent,type);

    if (bRet)
    {
        pvecstrIPList->clear();
        char szIP[16] = {'\0'};
        for (std::vector<unsigned long>::iterator iter = pveculIPList->begin(); iter != pveculIPList->end(); ++iter)
        {
            unsigned char *pbyIPSegment = (unsigned char*)(&(*iter));
            //sprintf_s(szIP, 16, "%d.%d.%d.%d", pbyIPSegment[0], pbyIPSegment[1], pbyIPSegment[2], pbyIPSegment[3]);
            sprintf(szIP, "%d.%d.%d.%d", pbyIPSegment[0], pbyIPSegment[1], pbyIPSegment[2], pbyIPSegment[3]);
            pvecstrIPList->push_back(szIP);
        }
    }

    return bRet;
}


bool CDNSLookup::Init()
{
    //WSADATA wsaData;
    //if (WSAStartup(MAKEWORD(2, 2), &wsaData) == SOCKET_ERROR)
    //{
      //  return false;
    //}
    
    if ((m_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return false;
    }

    //m_event = WSACreateEvent();
    //WSAEventSelect(m_sock, m_event, FD_READ);

    m_szDNSPacket = new (std::nothrow) char[DNS_PACKET_MAX_SIZE];
    if (m_szDNSPacket == NULL)
    {
        return false;
    }

    //m_usCurrentProcID = (uint16_t)GetCurrentProcessId();

    return true;
}

bool CDNSLookup::UnInit()
{
    //WSACleanup();

    if (m_szDNSPacket != NULL)
    {
        delete [] m_szDNSPacket;
    }

    return true;
}

bool CDNSLookup::DNSLookupCore(unsigned long ulDNSServerIP, char *szDomainName, std::vector<unsigned long> *pveculIPList, std::vector<std::string> *pvecstrCNameList, unsigned long ulTimeout, unsigned long *pulTimeSpent,uint16_t type)
{
    if (m_bIsInitOK == false || szDomainName == NULL)
    {
        return false;
    }

    sockaddr_in sockAddrDNSServer; 
    sockAddrDNSServer.sin_family = AF_INET; 
    sockAddrDNSServer.sin_addr.s_addr = ulDNSServerIP;
    sockAddrDNSServer.sin_port = htons( DNS_PORT );

    if (!SendDNSRequest(sockAddrDNSServer, szDomainName,type)
        || !RecvDNSResponse(sockAddrDNSServer, ulTimeout, pveculIPList, pvecstrCNameList, pulTimeSpent))
    {
        return false;
    }

    return true;
}

bool CDNSLookup::SendDNSRequest(sockaddr_in sockAddrDNSServer, char *szDomainName ,uint16_t type)
{
    char *pWriteDNSPacket = m_szDNSPacket;
    memset(pWriteDNSPacket, 0, DNS_PACKET_MAX_SIZE);    

    DNSHeader *pDNSHeader = (DNSHeader*)pWriteDNSPacket;
    pDNSHeader->usTransID = m_usCurrentProcID;
    printf("transId: %d\n",m_usCurrentProcID);
    pDNSHeader->usFlags = htons(0x0100);
    pDNSHeader->usQuestionCount = htons(0x0001);
    pDNSHeader->usAnswerCount = 0x0000;
    pDNSHeader->usAuthorityCount = 0x0000;
    pDNSHeader->usAdditionalCount = 0x0000;

    uint16_t usQType = htons(type);  
    uint16_t usQClass = htons(0x0001);
    uint16_t nDomainNameLen = strlen(szDomainName);
    char *szEncodedDomainName = (char *)malloc(nDomainNameLen + 2);
    if (szEncodedDomainName == NULL)
    {
        return false;
    }
    if (!EncodeDotStr(szDomainName, szEncodedDomainName, nDomainNameLen + 2))
    {
        return false;
    }

    uint16_t nEncodedDomainNameLen = strlen(szEncodedDomainName) + 1;
    memcpy(pWriteDNSPacket += sizeof(DNSHeader), szEncodedDomainName, nEncodedDomainNameLen);
    memcpy(pWriteDNSPacket += nEncodedDomainNameLen, (char*)(&usQType), DNS_TYPE_SIZE);
    memcpy(pWriteDNSPacket += DNS_TYPE_SIZE, (char*)(&usQClass), DNS_CLASS_SIZE);
    free (szEncodedDomainName);

    uint16_t nDNSPacketSize = sizeof(DNSHeader) + nEncodedDomainNameLen + DNS_TYPE_SIZE + DNS_CLASS_SIZE;
    if (sendto(m_sock, m_szDNSPacket, nDNSPacketSize, 0, (sockaddr*)&sockAddrDNSServer, sizeof(sockAddrDNSServer)) == -1)
    {
        return false;
    }

    return true;
}

bool CDNSLookup::RecvDNSResponse(sockaddr_in sockAddrDNSServer, unsigned long ulTimeout, std::vector<unsigned long> *pveculIPList, std::vector<std::string> *pvecstrCNameList, unsigned long *pulTimeSpent)
{
    //unsigned long ulSendTimestamp = GetTickCountCalibrate();

    if (pveculIPList != NULL)
    {
        pveculIPList->clear();
    }
    if (pvecstrCNameList != NULL)
    {
        pvecstrCNameList->clear();
    }

    char recvbuf[1024] = {'\0'};
    char szDotName[128] = {'\0'};
    uint16_t nEncodedNameLen = 0;

    while (true)
    {
       // if (WSAWaitForMultipleEvents(1, &m_event, false, 100, false) != WSA_WAIT_TIMEOUT)
        {
     //       WSANETWORKEVENTS netEvent;
       //     WSAEnumNetworkEvents(m_sock, m_event, &netEvent);

         //   if (netEvent.lNetworkEvents & FD_READ)
            {
         //       unsigned long ulRecvTimestamp = GetTickCountCalibrate();
                int nSockaddrDestSize = sizeof(sockAddrDNSServer);

                if (recvfrom(m_sock, recvbuf, 1024, 0, (struct sockaddr*)&sockAddrDNSServer, (socklen_t*)&nSockaddrDestSize) != -1)
                {
                    DNSHeader *pDNSHeader = (DNSHeader*)recvbuf;
                    uint16_t usQuestionCount = 0;
                    uint16_t usAnswerCount = 0;

                    if (pDNSHeader->usTransID == m_usCurrentProcID
                        && (ntohs(pDNSHeader->usFlags) & 0xfb7f) == 0x8100 //RFC1035 4.1.1(Header section format)
                        && (usQuestionCount = ntohs(pDNSHeader->usQuestionCount)) >= 0
                        && (usAnswerCount = ntohs(pDNSHeader->usAnswerCount)) > 0)
                    {
                        char *pDNSData = recvbuf + sizeof(DNSHeader);

                        for (int q = 0; q != usQuestionCount; ++q)
                        {
                //printf("pDNSData :%s \n",pDNSData);
                            if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName)))
                            {
                                return false;
                            }
                            pDNSData += (nEncodedNameLen + DNS_TYPE_SIZE + DNS_CLASS_SIZE);
                        }

                        for (int a = 0; a != usAnswerCount; ++a)
                        {
                //printf("pDNSData :%s \n",pDNSData);
                            if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName), recvbuf))
                            {
                                return false;
                            }
                            pDNSData += nEncodedNameLen;
            printf("DotName :%s \n",szDotName);

                            uint16_t usAnswerType = ntohs(*(uint16_t*)(pDNSData));
                            uint16_t usAnswerClass = ntohs(*(uint16_t*)(pDNSData + DNS_TYPE_SIZE));
                            unsigned long usAnswerTTL = ntohl(*(unsigned long*)(pDNSData + DNS_TYPE_SIZE + DNS_CLASS_SIZE));
                            uint16_t usAnswerDataLen = ntohs(*(uint16_t*)(pDNSData + DNS_TYPE_SIZE + DNS_CLASS_SIZE + DNS_TTL_SIZE));
                printf("datalen : %d\n",usAnswerDataLen);
                            pDNSData += (DNS_TYPE_SIZE + DNS_CLASS_SIZE + DNS_TTL_SIZE + DNS_DATALEN_SIZE);

                            if (usAnswerType == DNS_TYPE_A && pveculIPList != NULL)
                            {
                                unsigned long ulIP = *(unsigned long*)(pDNSData);
                                pveculIPList->push_back(ulIP);
                            }
                            else if (usAnswerType == DNS_TYPE_CNAME && pvecstrCNameList != NULL)
                            {
                                if (!DecodeDotStr(pDNSData, &nEncodedNameLen, szDotName, sizeof(szDotName), recvbuf))
                                {
                                    return false;
                                }
                                pvecstrCNameList->push_back(szDotName);
                            }
                else if(usAnswerType == DNS_TYPE_MX && pvecstrCNameList != NULL)
                {
                //pDNSData += sizeof(uint16_t); //

                                if (!DecodeDotStr(pDNSData+sizeof(uint16_t), &nEncodedNameLen, szDotName, sizeof(szDotName), recvbuf))
                                {
                                    return false;
                }
                printf("pDNSData :%s \n",pDNSData+sizeof(uint16_t));
                                pvecstrCNameList->push_back(szDotName);
                }

                            pDNSData += (usAnswerDataLen);
                        }

                   //     if (pulTimeSpent != NULL)
                     //   {
                       //     *pulTimeSpent = ulRecvTimestamp - ulSendTimestamp;
                       // }

                        break;
                    }
                }
            }
        }

        //if (GetTickCountCalibrate() - ulSendTimestamp > ulTimeout)
        //{
            *pulTimeSpent = ulTimeout + 1;
          //  return false;
        //}
    }

    return true;
}

/*
 * convert "www.baidu.com" to "\x03www\x05baidu\x03com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
 */
bool CDNSLookup::EncodeDotStr(char *szDotStr, char *szEncodedStr, uint16_t nEncodedStrSize)
{
    uint16_t nDotStrLen = strlen(szDotStr);

    if (szDotStr == NULL || szEncodedStr == NULL || nEncodedStrSize < nDotStrLen + 2)
    {
        return false;
    }

    char *szDotStrCopy = new char[nDotStrLen + 1];
    //strcpy_s(szDotStrCopy, nDotStrLen + 1, szDotStr);
    strcpy(szDotStrCopy, szDotStr);

    char *pNextToken = NULL;
    //char *pLabel = strtok_s(szDotStrCopy, ".", &pNextToken);
    char *pLabel = strtok(szDotStrCopy, ".");
    uint16_t nLabelLen = 0;
    uint16_t nEncodedStrLen = 0;
    while (pLabel != NULL)
    {
        if ((nLabelLen = strlen(pLabel)) != 0)
        {
            //sprintf_s(szEncodedStr + nEncodedStrLen, nEncodedStrSize - nEncodedStrLen, "%c%s", nLabelLen, pLabel);
            sprintf(szEncodedStr + nEncodedStrLen, "%c%s", nLabelLen, pLabel);
            nEncodedStrLen += (nLabelLen + 1);
        }
        //pLabel = strtok_s(NULL, ".", &pNextToken);
        pLabel = strtok(NULL, ".");
    }

    delete [] szDotStrCopy;
    
    return true;
}

/*
 * convert "\x03www\x05baidu\x03com\x00" to "www.baidu.com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 03 63 6f 6d 00 ff
 * convert "\x03www\x05baidu\xc0\x13" to "www.baidu.com"
 * 0x0000 03 77 77 77 05 62 61 69 64 75 c0 13 ff ff ff ff
 * 0x0010 ff ff ff 03 63 6f 6d 00 ff ff ff ff ff ff ff ff
 */
bool CDNSLookup::DecodeDotStr(char *szEncodedStr, uint16_t *pusEncodedStrLen, char *szDotStr, uint16_t nDotStrSize, char *szPacketStartPos)
{
    if (szEncodedStr == NULL || pusEncodedStrLen == NULL || szDotStr == NULL)
    {
        return false;
    }

    char *pDecodePos = szEncodedStr;
    uint16_t usPlainStrLen = 0;
    unsigned char nLabelDataLen = 0;    
    *pusEncodedStrLen = 0;

    while ((nLabelDataLen = *pDecodePos) != 0x00)
    {
        if ((nLabelDataLen & 0xc0) == 0) 
        {
            if (usPlainStrLen + nLabelDataLen + 1 > nDotStrSize)
            {
                return false;
            }
            memcpy(szDotStr + usPlainStrLen, pDecodePos + 1, nLabelDataLen);
            memcpy(szDotStr + usPlainStrLen + nLabelDataLen, ".", 1);
            pDecodePos += (nLabelDataLen + 1);
            usPlainStrLen += (nLabelDataLen + 1);
            *pusEncodedStrLen += (nLabelDataLen + 1);
        }
        else 
        {
            if (szPacketStartPos == NULL)
            {
                return false;
            }
            uint16_t usJumpPos = ntohs(*(uint16_t*)(pDecodePos)) & 0x3fff;
            uint16_t nEncodeStrLen = 0;
            if (!DecodeDotStr(szPacketStartPos + usJumpPos, &nEncodeStrLen, szDotStr + usPlainStrLen, nDotStrSize - usPlainStrLen, szPacketStartPos))
            {
                return false;
            }
            else
            {
                *pusEncodedStrLen += 2;
                return true;
            }
        }
    }

    szDotStr[usPlainStrLen - 1] = '\0';
    *pusEncodedStrLen += 1;

    return true;
}

unsigned long CDNSLookup::GetTickCountCalibrate()
{
/*
    static unsigned long s_ulFirstCallTick = 0;
    static LONGLONG s_ullFirstCallTickMS = 0;

    SYSTEMTIME systemtime;
    FILETIME filetime;
    GetLocalTime(&systemtime);    
    SystemTimeToFileTime(&systemtime, &filetime);
    LARGE_INTEGER liCurrentTime;
    liCurrentTime.HighPart = filetime.dwHighDateTime;
    liCurrentTime.LowPart = filetime.dwLowDateTime;
    LONGLONG llCurrentTimeMS = liCurrentTime.QuadPart / 10000;

    if (s_ulFirstCallTick == 0)
    {
        s_ulFirstCallTick = GetTickCount();
    }
    if (s_ullFirstCallTickMS == 0)
    {
        s_ullFirstCallTickMS = llCurrentTimeMS;
    }

    return s_ulFirstCallTick + (unsigned long)(llCurrentTimeMS - s_ullFirstCallTickMS);
    */
}
const int MAXDATASIZE = 100;
#define SERVER_PORT 25
void * test_connection ( void  *args)
{
    const char * ip_addr = (const char *) args;
    printf("arg ip: %s \n",ip_addr);
    int sockfd, numbytes; 
    char buf[MAXDATASIZE]; 
    struct sockaddr_in server_addr; 

    printf("\n======================client initialization======================\n"); 
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
    perror("socket"); 
    exit(1); 
    }

    server_addr.sin_family = AF_INET; 
    server_addr.sin_port = htons(SERVER_PORT); 
    server_addr.sin_addr.s_addr = inet_addr(ip_addr); 
    bzero(&(server_addr.sin_zero),sizeof(server_addr.sin_zero)); 

    if (connect(sockfd, (struct sockaddr *)&server_addr,sizeof(struct sockaddr_in)) == -1){
    perror("connect error"); 
    exit(1);
    } 

    while(1) { 
    bzero(buf,MAXDATASIZE); 
    printf("\nBegin receive...\n"); 
    if ((numbytes = recv(sockfd, buf, MAXDATASIZE, 0)) == -1){  
        perror("recv"); 
        exit(1);
    } else if (numbytes > 0) { 
        int len, bytes_sent;
        buf[numbytes] = '\0'; 
        printf("Received: %s\n",buf);
        printf("Send:"); 
        char msg[100];
        scanf("%s",msg);
        len = strlen(msg); 
        //sent to the server
        if(send(sockfd, msg,len,0) == -1){ 
        perror("send error"); 
        }
    } else { 
        printf("soket end!\n"); 
        break;
    } 
    }  
    close(sockfd); 

//    return NULL;
}
/*

int main(int argc,char *argv[])
{
    char *szDomainName = "qq.com";//"www.baidu.com";
    if(argc >1 )
    szDomainName = argv[1];
    std::vector<unsigned long> veculIPList;
    std::vector<std::string> vecstrIPList;
    std::vector<std::string> vecCNameList;
    unsigned long ulTimeSpent = 0;
    CDNSLookup dnslookup;
    bool bRet = dnslookup.DNSLookup(inet_addr("8.8.8.8"), szDomainName, &vecstrIPList, &vecCNameList,CDNSLookup::MX, 1000, &ulTimeSpent);

    std::vector <std::string > final_ip_addr ;
    printf("DNSLookup result (%s):\n", szDomainName);
    if (!bRet)
    {
    printf("timeout!\n");
    return -1;
    }

    for (int i = 0; i != veculIPList.size(); ++i)
    {
    printf("IP%d(ULONG) = %u\n", i + 1, veculIPList[i]);
    }
    for (int i = 0; i != vecstrIPList.size(); ++i)
    {
    printf("IP%d(string) = %s\n", i + 1, vecstrIPList[i].c_str());
    }
    for (int i = 0; i != vecCNameList.size(); ++i)
    {
    std::vector <std::string > new_cname_list;
    printf("CName%d = %s\n", i + 1, vecCNameList[i].c_str());
    char tt [100]; 
    strcpy(tt,vecCNameList[i].c_str());
    vecstrIPList.clear();
    bool bRet = dnslookup.DNSLookup(inet_addr("8.8.8.8"),tt,&vecstrIPList,&new_cname_list,CDNSLookup::A,1000,&ulTimeSpent);

    for(int j = 0;j<vecstrIPList.size();++j)
    {

        final_ip_addr.push_back(vecstrIPList[j]);
        printf("IP%d(string) = %s\n", i + 1, vecstrIPList[j].c_str());
    }
    }
    printf("time spent = %ums\n", ulTimeSpent);
    pthread_t * fd_array = new pthread_t[final_ip_addr.size()];
    for(int i = 0;i<final_ip_addr.size();++i)
    {
    pthread_create(&fd_array[i],NULL, test_connection,(void *)final_ip_addr[i].c_str());
    }
    for(int i = 0;i<final_ip_addr.size();++i)
    pthread_join(fd_array[i],NULL);

    return 0;
}
*/
