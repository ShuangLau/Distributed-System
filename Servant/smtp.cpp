#include "smtp.h"
#include "database.h"
#include "DNStest/DNSLookup.h"
#include <sstream>

SMTP * SMTP::_instance = nullptr;

void SMTP::run()
{
    int client_sockfd;
    socklen_t sin_size;
    struct sockaddr_in server_addr, client_addr;
    if ((_smtp_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

	perror("S:socket create error！\n");
	exit(1);
    }
    int optval = 1;
    setsockopt(_smtp_sock,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(optval));
    memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SMTP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_addr.sin_zero), 8);

    if (bind(_smtp_sock, (struct sockaddr *) &server_addr,
		sizeof(struct sockaddr)) == -1) {
	perror("S:bind error！\n");
	exit(1);
    }
    fcntl(_smtp_sock, F_SETFL, fcntl(_smtp_sock, F_GETFL, 0)|O_NONBLOCK);

    if (listen(_smtp_sock, MAX_CLIENTS - 1) == -1) {
	perror("S:listen error！\n");
	exit(1);
    }

    std::cout << "===============================\n";
    std::cout << "-XSMTP mail server started..." << std::endl;
    sin_size = sizeof(client_addr);
    while (1) {
	if ((client_sockfd = accept(_smtp_sock,
			(struct sockaddr *) &client_addr, &sin_size)) == -1) {
	    sleep(1);
	    continue;
	}
	std::cout << "S:received a connection from " 
	    << inet_ntoa(client_addr.sin_addr) << " at " 
	    << time(NULL) <<std:: endl;

	pthread_t id;
	pthread_create(&id, NULL, mail_proc, &client_sockfd);
	pthread_join(id, NULL);
    }
    close(client_sockfd);

}

SMTP * SMTP::get_instance()
{
    if(_instance == nullptr)
	_instance = new SMTP();
    return _instance;
}
SMTP::SMTP():
    from_user(new char[100]),user_info(new char[100]),
    userstat(new char[100]),rcpt_user_num(0),mail_stat(0)
{

}

void * mail_proc(void* param) {
    int client_sockfd, len;
    char buf[BUF_SIZE];

    memset(buf, 0, sizeof(buf));
    client_sockfd = *(int*) param;

    SMTP::get_instance()->send_data(client_sockfd, reply_code[4]); //send 220
    SMTP::get_instance()->mail_stat = 1;

    while (1) {
	memset(buf, 0, sizeof(buf));
	len = recv(client_sockfd, buf, sizeof(buf), 0);
	if (len > 0) {
	    std::cout << "Request stream: " << buf;
	    SMTP::get_instance()->respond(client_sockfd, buf);
	} else {
	    std::cout << "S: no data received from client. The server exit permanently.\n";
	    break;
	}
    }
    std::cout << "S:[" << client_sockfd << "] socket closed by client." << std::endl;
    std::cout << "============================================================\n\n";
    return NULL;
}

void SMTP::respond(int client_sockfd, char* request) {
    char output[1024];
    memset(output, 0, sizeof(output));

    if (strncmp(request, "HELO", 4) == 0) {
	if (mail_stat == Login) {
	    send_data(client_sockfd, reply_code[6]);
	    rcpt_user_num = 0;
	    memset(rcpt_user, 0, sizeof(rcpt_user));
	    mail_stat = SayedHELO;
	} else {
	    send_data(client_sockfd, reply_code[15]);
	}
    } else if (strncmp(request, "MAIL FROM", 9) == 0) {
	if (mail_stat == SayedHELO || mail_stat == SayedEHLO) {
	    char *pa, *pb;
	    pa = strchr(request, '<');
	    pb = strchr(request, '>');
	    strncpy(from_user, pa + 1, pb - pa - 1);
	    printf("from user : %s\n",from_user);
	    //if (DataBase::get_instance() 
	//	    ->has_user(from_user)) {
		    if(true){
		send_data(client_sockfd, reply_code[6]);
		mail_stat = MailFromOk;
	    } else {
		send_data(client_sockfd, reply_code[15]);
	    }
	} else if (mail_stat == NeedAuth) {
	    send_data(client_sockfd, reply_code[23]);
	} else {
	    send_data(client_sockfd, "503 Error: send HELO/EHLO first\r\n");
	}
    } else if (strncmp(request, "RCPT TO", 7) == 0) {
	if ((mail_stat == MailFromOk || mail_stat == RcptToOk) && rcpt_user_num < MAX_RCPT_USR) {
	    char *pa, *pb;
	    pa = strchr(request, '<');
	    pb = strchr(request, '>');
	    strncpy(rcpt_user[rcpt_user_num++], pa + 1, pb - pa - 1);
	    send_data(client_sockfd, reply_code[6]);
	    mail_stat = RcptToOk;
	} else {
	    send_data(client_sockfd, reply_code[16]);
	}
    } else if (strncmp(request, "DATA", 4) == 0) {
	if (mail_stat == RcptToOk) {
	    send_data(client_sockfd, reply_code[8]);
	    mail_data(client_sockfd);
	    mail_stat = SendMailOk;
	} else {
	    send_data(client_sockfd, reply_code[16]);
	}
    } else if (strncmp(request, "RSET", 4) == 0) {
	mail_stat = Login;
	send_data(client_sockfd, reply_code[6]);
    } else if (strncmp(request, "NOOP", 4) == 0) {
	send_data(client_sockfd, reply_code[5]);
    } else if (strncmp(request, "QUIT", 4) == 0) {
	mail_stat = NotLogin;
	//user_quit();
	send_data(client_sockfd, reply_code[5]);
	pthread_exit((void*)1);
    }
    else if (strncmp(request, "EHLO", 4) == 0) {
	if (mail_stat == Login) {
	    send_data(client_sockfd, reply_code[24]);
	    mail_stat = 13;
	} else {
	    send_data(client_sockfd, reply_code[15]);
	}
    } else if (strncmp(request, "AUTH LOGIN", 10) == 0) {
	auth(client_sockfd);
    } else if (strncmp(request, "AUTH LOGIN PLAIN", 10) == 0) {
	auth(client_sockfd);
    } else if (strncmp(request, "AUTH=LOGIN PLAIN", 10) == 0) {
	auth(client_sockfd);
    } else {
	send_data(client_sockfd, reply_code[15]);
    }
}
void SMTP::auth(int sockfd)
{
    char ename[50], epass[50];
    char *name, *pass;
    int len;

    send_data(sockfd, reply_code[25]); // require username
    sleep(3);
    len = recv(sockfd, ename, sizeof(ename), 0);
    if (len > 0) {
	std::cout << "Request stream: " << ename << std::endl;
	name = base64_decode(ename);
	std::cout << "Decoded username: " << name << std::endl;
	send_data(sockfd, reply_code[26]); // require passwd
	sleep(3);
	len = recv(sockfd, epass, sizeof(epass), 0);
	if (len > 0) {
	    std::cout << "Request stream: " << epass << std::endl;
	    pass = base64_decode(epass);
	    std::cout << "Decoded password: " << pass << std::endl;
	    if (DataBase::get_instance()
		    ->check_user(name, pass)) { // check username and passwd
		mail_stat = 13;
		send_data(sockfd, reply_code[27]);
	    } else {
		send_data(sockfd, reply_code[16]);
	    }
	} else {
	    send_data(sockfd, reply_code[16]);
	}
    } else {
	send_data(sockfd, reply_code[16]);
    }
}

void SMTP::send_data(int sockfd, const char* data) {
    if (data != NULL) {
	send(sockfd, data, strlen(data), 0);
	std::cout << "Reply stream: " << data;
    }
}
void SMTP::mail_data(int sockfd) {
    sleep(1);
    char buf[BUF_SIZE];
    memset(buf, 0, sizeof(buf));
    recv(sockfd, buf, sizeof(buf), 0); 
    std::cout << "Mail Contents: \n" << buf << std::endl;


    for(int i = 0;i< rcpt_user_num ;++i)
    {
	std::string rcpt = std::string(rcpt_user[i]);
	std::string user_name = rcpt.substr(0,rcpt.find_first_of('@'));
	std::cout <<"rcpt user_name :"<<user_name <<std::endl;
	DataBase::get_instance()->append_mail(user_name,buf);
    }
    /*
    int tm = time(NULL), i;
    char file[80], tp[20];

    for (i = 0; i < rcpt_user_num; i++) {
	strcpy(file, data_dir);
	strcat(file, rcpt_user[i]);
	if (access(file,0) == -1) {
	    mkdir(file,0777);
	}
	sprintf(tp, "/%d", tm);
	strcat(file, tp);

	FILE* fp = fopen(file, "w+");
	if (fp != NULL) {
	    fwrite(buf, 1, strlen(buf), fp);
	    fclose(fp);
	} else {
	    cout << "File open error!" << endl;
	}
    }
    */
    send_data(sockfd, reply_code[6]);
}

static const int buffer_len = 10240;
static char  tt[buffer_len];

std::unordered_map <std::string,std::string > SMTP:: parse_email(const std::string & content)
{
    std::unordered_map <std::string,std::string > ans;
    std::istringstream is(content);
    std::cout <<"mail content:\n" <<content <<std::endl;

    bool content_flag = false;

    std::string boundary;
    while(is.getline(tt,buffer_len))
    {
	std::string line = tt;
	std::cout <<"line :" <<tt << std::endl;
	
	 if(boundary != "" &&
		line.find(boundary) != std::string ::npos)
	{
	    std::cout <<"first" <<std::endl;
	    if(!is.getline(tt,buffer_len))  break; //Content-Type:
	    if(std::string(tt).find("Content-Type:") == std::string::npos)
		break;
	    std::cout <<"second" <<std::endl;
	    if(!is.getline(tt,buffer_len)) break; // charset
	    std::cout <<"thrid" <<std::endl;
	    if(!is.getline(tt,buffer_len)) break;  //content-Transfer-Encoding
	    std::cout <<"fouth" <<std::endl;
	    int base64_flag = false;
	    if(std::string(tt) .find("base64") != std::string::npos)
		base64_flag = true;
	    if(!is.getline(tt,buffer_len)) break;  //empty
	    std::cout <<"five" <<std::endl;
	    if(!is.getline(tt,buffer_len)) break;  //content
	    std::cout <<"six" <<std::endl;
	    if(base64_flag )
	    {
		ans["content"] += "\n" + std::string(base64_decode(tt))+ "\n";
	    }else ans["content"]+= "\n" +std::string(tt)+"\n";
	//    if(!is.getline(tt,buffer_len)) break;  //empty

	}
	else if(line.find ("boundary=") != std::string::npos)
	{
	    boundary = line.substr(line.find_first_of('\"')+1,
		    line.find(line.find_last_of('\"')) - line.find_first_of('\"') -1
		    );
	}

	else {
	    std::size_t pos = line.find_first_of(':');
	    if( pos != std::string::npos)
	    {
		std::string head = line.substr(0,pos);
		if(head == "Content-Type") content_flag = true;
		if(head == "From" || head == "To")
		{
		    std::size_t pos_1 = line.find_first_of('<');
		    std::size_t pos_2 = line.find_first_of('>');
		    if( pos_1 != std::string::npos&&
			    pos_2 != std::string::npos)
		    {
			ans[head] = line.substr(pos_1+1,pos_2-pos_1-1);
		    }
		}else ans[head] = line.substr(pos+1);
		std::cout <<"\n*************head :" <<head<<std::endl;
		std::cout <<"\n************cut result : " <<line.substr(pos+1)<<std::endl;
	    }
	}
	if(content_flag == true)
	{
	    if(!is.getline(tt,buffer_len)) break;  //empty
	    std::cout <<"five" <<std::endl;
	    if(!is.getline(tt,buffer_len))   break;//content
	    std::string res(tt);
	    std::size_t begin_pos = 0;
	    std::size_t pos = res.find_first_of("%%0D%%0A");
	    do
	    {
		std::string tmp = res.substr(begin_pos,pos==std::string::npos?
std::string::npos: pos-begin_pos) ;
		for(int i = 0;i<tmp.length();++i) tmp[i]=(tmp[i]=='+'?' ':tmp[i]);
		ans["content"] += tmp+ "<br/>";
		begin_pos = pos + std::string("%0D%0A").length();
		pos = res.find_first_of("%0D%0A",begin_pos);
	    }
	    while(pos != std::string::npos);
	}
    }
    //delete[] tt;

    return ans;

}
static char buffer2[102400];
char * SMTP::base64_decode(char *s) { 
    std::cout <<"in decode .. "<<std::endl;
    char *p = s, *e, *r, *_ret;
    int len = strlen(s);
    unsigned char i, unit[4];

    e = s + len;
    len = len  * 3 + 1;
    r = _ret = (char *) buffer2;

    while (p < e) {
	memcpy(unit, p, 4);
	if (unit[3] == '=')
	    unit[3] = 0;
	if (unit[2] == '=')
	    unit[2] = 0;
	p += 4;

	for (i = 0; unit[0] != B64[i] && i < 64; i++)
	    ;
	unit[0] = i == 64 ? 0 : i;
	for (i = 0; unit[1] != B64[i] && i < 64; i++)
	    ;
	unit[1] = i == 64 ? 0 : i;
	for (i = 0; unit[2] != B64[i] && i < 64; i++)
	    ;
	unit[2] = i == 64 ? 0 : i;
	for (i = 0; unit[3] != B64[i] && i < 64; i++)
	    ;
	unit[3] = i == 64 ? 0 : i;
	*r++ = (unit[0] << 2) | (unit[1] >> 4);
	*r++ = (unit[1] << 4) | (unit[2] >> 2);
	*r++ = (unit[2] << 6) | unit[3];
    }
    *r = 0;
    //strcpy(s,_ret);
    return _ret;
}




std::unordered_map <std::string,std::string > EmailClient::RenderEmail(const std::string &from_user ,const std::string &to_user ,
	const std::string &subject,
	const std::string &content)
{
    std::unordered_map <std::string,std::string> ans;
    std::ostringstream os;
    std::size_t atpos = from_user.find("%40");
    if( atpos == std::string::npos) 
	return ans;
    std::string user_name =  from_user.substr(0,atpos);
    std::string com = from_user.substr(atpos+3);
    ans["from_user_name"] = user_name;
    ans["from_user_com"] = com;
    os << "From: \""<<user_name<<"\" " << "<" <<user_name<<"@"<<com<<">\n";
    atpos = to_user.find("%40");
    if( atpos == std::string::npos) 
	return ans;
    user_name = to_user.substr(0,atpos);
    com = to_user.substr(atpos+3);
    ans["to_user_name"] = user_name ;
    ans["to_user_com"] = com;
    os << "To: \""<<user_name<<"\" " << "<" <<user_name<<"@"<<com<<">\n";
    ans["subject"] = subject; 
    os <<"Subject: " << subject <<"\n";
    os <<"Mime-Version: 1.0\n";

    ans["content"] = content;
    os << "Content-Type: text/plain;\n";
    os <<"\n";

    os << content <<std::endl;

    os <<"\r\n.\r\n";
    ans["result"] = os.str();

    return ans;

}



bool EmailClient::SendEmail(const std::string &from_user ,
	const std::string &to_user ,
	const std::string &subject,
	const std::string &content)
{
    std::cout <<"from_user:" <<from_user<<std::endl;
    std::cout << "to_user:" << to_user<<std::endl;
    std::cout <<"subject:" << subject <<std::endl;
    std::cout << "content:" << content <<std::endl;

//    std::cout << RenderEmail(from_user,to_user,subject,content);
    std::vector <std::string > final_ip_addr ;
    auto result = RenderEmail(from_user,to_user,subject,content);
    if( result.find("result") == result.end()) {
    	// std::cout << "FFFFFF" << std::endl;
		return false;
	}
//query DNS
    std::vector <std::string > vecstrIPList;
    std::vector <std::string > vecCNameList;
    char szDomainName [100];
    if(result["to_user_com"] != "localhost")
    {
    strcpy(szDomainName,result["to_user_com"].c_str());
    unsigned long ulTimeSpent = 0;
    CDNSLookup dnslookup;
    unsigned long DNS_server_ip = inet_addr("8.8.8.8");
    std::cout << "QQQQQQQQQQQQQQQQQQQQ" << std::endl;
    bool bRet = dnslookup.DNSLookup(DNS_server_ip,szDomainName, &vecstrIPList, &vecCNameList,CDNSLookup::MX, 1000, &ulTimeSpent);
  	std::cout << "EEEEEEEEEEEEEEEEEee" << std::endl;
      //printf("DNSLookup result (%s):\n", szDomainName);
      if (!bRet)
      {
          printf("timeout!\n");
          return -1;
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
          bool bRet = dnslookup.DNSLookup(DNS_server_ip,tt,&vecstrIPList,&new_cname_list,CDNSLookup::A,1000,&ulTimeSpent);
  
          for(int j = 0;j<vecstrIPList.size();++j)
          {
  
              final_ip_addr.push_back(vecstrIPList[j]);
              printf("IP%d(string) = %s\n", i + 1, vecstrIPList[j].c_str());
          }
      }

    std::cout << "track " <<final_ip_addr.size() << "IPs.." <<std::endl;
    }
    else final_ip_addr.push_back("127.0.0.1");

      for(int i = 0;i<final_ip_addr.size();++i)
      {

	  sockaddr_in sin;
	  int fd;
	  if((fd = socket(PF_INET,SOCK_STREAM,0) ) == -1 )
	  {
	      perror("socket");
	      return false;
	  }

	  sin.sin_family = AF_INET;
	  sin.sin_port = htons(25);
	  sin.sin_addr.s_addr = inet_addr(final_ip_addr[i].c_str());
	  if(connect(fd,(sockaddr*) &sin,sizeof(sin)) == -1)
	  {
	      perror("connect error");
	      continue;
	  }
	  char bufferRecv[2048];
	  char bufferSend[2048];
	  // hello msg
	  int len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout << "hello msg:"<< bufferRecv<<std::endl;
	

	  // EHLO
	  char bufferHello[] = "HELO tiny.com\r\n";

	  std::cout <<"sending ehlo to server .." <<std::endl;
	  if(send(fd,bufferHello,strlen(bufferHello),MSG_DONTWAIT) <0)
	  {
	      perror("sending error");
	      return false;
	  }
	  std::cout <<"send over ehlo ..";
	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout << "reply EHLO:"<< bufferRecv<<std::endl;


	  // mail from
	  sprintf(bufferSend,"MAIL FROM: <%s@%s>\r\n",
		 // result["from_user_name"].c_str(),
		  result["from_user_name"].c_str(),
		  result["from_user_com"].c_str());

	  if(send(fd,bufferSend,strlen(bufferSend),0) <0)
	  {

	      perror("sending error");
	      return false;
	    }
	  //send(fd,bufferSend,strlen(bufferSend),0);

	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout <<"reply MAIL FROM:" << bufferRecv<<std::endl;

	  //rcpt to :
	  sprintf(bufferSend,"RCPT TO: <%s@%s>\r\n",
		//  result["to_user_name"].c_str(),
		  result["to_user_name"].c_str(),
		  result["to_user_com"].c_str());

	  if(send(fd,bufferSend,strlen(bufferSend),0) <0)
	  {
	      perror("sending error");
	      return false;
	  }
	  //send(fd,bufferSend,strlen(bufferSend),0);

	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout <<"reply RCPT TO:" <<bufferRecv<<std::endl;

	  //data
	  char bufferData[] = "DATA\r\n";
	  if(send(fd,bufferData,strlen(bufferData),0)<0)
	  {
		perror("senddata error");
	  }

	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout << "reply DATA:"<<bufferRecv<<std::endl;

	  //content
	  send(fd,result["result"].c_str(),result["result"].length(),0);
	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout <<"reply content:" <<bufferRecv<<std::endl;

	  char bufferBye[] = "QUIT\r\n";
	  send(fd,bufferBye,strlen(bufferBye),0);
	  len = recv(fd,bufferRecv,2048,0);
	  bufferRecv[len] = 0;
	  std::cout <<"reply QUIT:" <<bufferRecv<<std::endl;
	  return true;
      }



    return false;

}





