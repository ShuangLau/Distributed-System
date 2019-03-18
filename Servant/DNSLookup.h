#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_DOMAINNAME_LEN  255
#define DNS_PORT            53
#define DNS_TYPE_SIZE       2
#define DNS_CLASS_SIZE      2
#define DNS_TTL_SIZE        4
#define DNS_DATALEN_SIZE    2
#define DNS_TYPE_A          0x0001 //1 a host address
#define DNS_TYPE_CNAME      0x0005 //5 the canonical name for an alias
#define DNS_PACKET_MAX_SIZE (sizeof(DNSHeader) + MAX_DOMAINNAME_LEN + DNS_TYPE_SIZE + DNS_CLASS_SIZE)

struct DNSHeader
{
    uint16_t usTransID; //id
    uint16_t usFlags; 
    uint16_t usQuestionCount; // the segment number of questions
    uint16_t usAnswerCount; //the segment number of answer
    uint16_t usAuthorityCount; //the segment number of authority
    uint16_t usAdditionalCount; //the segment number of additional
};

class CDNSLookup
{
    public:
	CDNSLookup();
	~CDNSLookup();

	bool DNSLookup(unsigned long ulDNSServerIP, char *szDomainName, std::vector<unsigned long> *pveculIPList = NULL, std::vector<std::string> *pvecstrCNameList = NULL, unsigned long ulTimeout = 1000, unsigned long *pulTimeSpent = NULL);
	bool DNSLookup(unsigned long ulDNSServerIP, char *szDomainName, std::vector<std::string> *pvecstrIPList = NULL, std::vector<std::string> *pvecstrCNameList = NULL, unsigned long ulTimeout = 1000, unsigned long *pulTimeSpent = NULL);

    private:
	bool Init();
	bool UnInit();
	bool DNSLookupCore(unsigned long ulDNSServerIP, char *szDomainName, std::vector<unsigned long> *pveculIPList, std::vector<std::string> *pvecstrCNameList, unsigned long ulTimeout, unsigned long *pulTimeSpent);
	bool SendDNSRequest(sockaddr_in sockAddrDNSServer, char *szDomainName);
	bool RecvDNSResponse(sockaddr_in sockAddrDNSServer, unsigned long ulTimeout, std::vector<unsigned long> *pveculIPList, std::vector<std::string> *pvecstrCNameList, unsigned long *pulTimeSpent);
	bool EncodeDotStr(char *szDotStr, char *szEncodedStr, uint16_t nEncodedStrSize);
	bool DecodeDotStr(char *szEncodedStr, uint16_t *pusEncodedStrLen, char *szDotStr, uint16_t nDotStrSize, char *szPacketStartPos = NULL);
	unsigned long GetTickCountCalibrate();

    private:
	bool m_bIsInitOK;
	int m_sock;
//	WSAEVENT m_event;
	uint16_t m_usCurrentProcID;
	char *m_szDNSPacket;
};
