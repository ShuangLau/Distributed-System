
#ifndef HANDLER_H
#define HANDLER_H

#include <string>
#include <algorithm>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include "Parser.h"
#include "Buffer.h"
#include "../storage-system/client/connection.h"
#include "../storage-service/storage-service-user.h"
#include "../kvs-admin/kvsadmin.h"

class Handler 
{
public:
	enum ErrorCode{
		OK,
		FileNotFound,
		FileNotRead,
		PostDataErr,
		ParseCookieErr,
		NotLoginErr,
		PassWordNotMatch
	};
    Handler(const int connfd);
    ~Handler();
    void handle();
    const int connFd() const
    {
        return _connfd;
    }
    static Connection _conn;
private:
    void list_files(std::string current_dir, std::ostringstream &os);
    bool receiveRequest();  
    void sendResponse();    
    void sendMsg(const std::string &errorNum,
                      const std::string &shortMsg,
                      const std::string &longMsg,
		      const std::string & backUri = "",
		      const std::string & cookie_sid ="");
    void parseURI();
    int handleGet();
    int handlePost();
    void getFileType();
    void sendFile();
    void sendPage(const std::string & page);
    std::string renderUserInfo(const std::string & user_name);
    std::string renderWriteMailPage(const std::string & from ,const std::string & to);
    std::string renderMailList();
    std::string renderForwardPage(const std::string & mail_id);
    std::string renderDrivePage(std::string dir);
    std::string renderAdminPage(std::string table_addr);

    int _connfd;
    bool _isClosed;
    std:: unordered_map <std::string,std::string > parsePostArgs();
    std::unordered_map <std::string,std::string > parseSscPostArgs();
    std:: unordered_map <std::string,std::string > parseArgs(const std::string & args);
    std:: unordered_map <std::string,std::string > parseCookie();
    bool _islogin;
    std::string _userName;
    std::string _passwd;
    std::string _fileType;
    std::string _fileName;
    Buffer _inputBuffer;
    Buffer _outputBuffer;
    HTTPRequest _request;

    StorageServiceUser *_service_user = nullptr;
};

#endif // HANDLER_H
