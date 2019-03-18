
#include <sstream>
#include "Handler.h"
#include "database.h"
#include "smtp.h"

Connection Handler::_conn = Configuration::connect("127.0.0.1:50051");

Handler::Handler(const int connfd)
:_connfd(connfd)
{
    _isClosed = false;
}

Handler::~Handler()
{
    if(!_isClosed)
        close(_connfd);
    if (_service_user)
        free(_service_user);
}

void Handler::sendFile()
{
    struct stat fileInfo;
    if(stat(_fileName.c_str(), &fileInfo) < 0)
    {
        std::cout << "404 Not found: Server couldn't find this file." << std::endl;
        sendMsg("404", "Not Found", "Server couldn't find this file");
//      return FileNotFound;
    }
    if(!(S_ISREG(fileInfo.st_mode)) || !(S_IRUSR & fileInfo.st_mode))
    {
        std::cout << "403 Forbidden: Server couldn't read this file." << std::endl;
        sendMsg("403", "Forbidden", "Server couldn't read this file");
//      return FileNotRead;
    }
    getFileType();
    std::string msg = "HTTP/1.1 200 OK\r\n";
    msg += "Server: Tiny Web Server\r\n";
    msg += "Content-length: " + std::to_string(fileInfo.st_size) + "\r\n";
    msg += "Content_type: " + _fileType + "\r\n\r\n";
    _outputBuffer.append(msg.c_str(), msg.size());
    int fd = open(_fileName.c_str(), O_RDONLY, 0);
    _outputBuffer.readFd(fd);
    _outputBuffer.sendFd(_connfd);
    close(_connfd);
    _isClosed = true;

}


void Handler::sendPage(const std::string & page)
{
    std::ostringstream msg;
    msg << "HTTP/1.1 200 OK\r\n";
    msg << "Server: Tiny Web Server\r\n";
    std::string result = msg.str() ;
    if(_request.method!= "HEAD")
    {
        msg << "Content-length: " << page.length() << "\r\n";
        msg << "Content_type: " + _fileType + "\r\n\r\n";
        result= msg.str()+ page;
    }

    _outputBuffer.append(result.c_str(), result.size());
    _outputBuffer.sendFd(_connfd);
    close(_connfd);
    _isClosed = true;

}

std::string Handler::renderWriteMailPage(const std::string & from ,const std::string & to)
{
    std::ostringstream os;
    os << "<html><head><title>WriteMail </title></head>\n";
    os << "<body><h1>Write Mail  </h1>\n"; 
    os << "<a href=\"/userinfo\"> Return to Main Page </a><br/>";
    os << "<form action=\"sendmail\" id=\"mailform\" method=\"POST\" >";
    os << "From : <input type=\"text\" name=\"from_user\" value=\""<<to<<"\"  ><br/>";
    if( to == std::string(""))
        os << "To : <input type=\"text\" name=\"to_user\" ><br/>";
    else 
        os << "To : <input type=\"text\" name=\"to_user\" value=\""<<from<<"\"  ><br/>";
    os << "Subject : <input type=\"text\" name=\"subject\" ><br/>";
    os << "<input type=\"submit\" > <br/> ";
    os <<"<textarea name=\"content\" form=\"mailform\"></textarea>";
    os << "</body> </html>" ;
    return os.str();
}

std::string Handler::renderForwardPage(const std::string & mail_id)
{
    std::string mail_no;
    auto mail_list = DataBase::get_instance()->get_mail(_userName);
    for(std::string mail : mail_list )
    {
        std::istringstream is(mail);
        is >>mail_no;
        if( mail_no == mail_id)
        {
            auto mp = SMTP::parse_email(mail);

            std::ostringstream os;
            os << "<html><head><title>WriteMail </title></head>\n";
            os << "<body><h1>Write Mail  </h1>\n"; 
            os << "<a href=\"/userinfo\"> Return to Main Page </a><br/>";
            os << "<form action=\"sendmail\" id=\"mailform\" method=\"POST\" >";
            os << "From : <input type=\"text\" name=\"from_user\" value=\""<<mp["To"]<<"\"  ><br/>";
            os << "To : <input type=\"text\" name=\"to_user\" value=\""<<"\"  ><br/>";
            os << "Subject : <input type=\"text\" name=\"subject\" value=\""<<mp["Subject"]<<"\"><br/>";
            os << "<input type=\"submit\" > <br/> ";
            os <<"<textarea name=\"content\" form=\"mailform\"> "<<mp["content"]<<"</textarea><br/>";
            os << "</body> </html>" ;
            return os.str();
        }
    }

}
std::string Handler::renderUserInfo(const std::string & username)
{
    std::ostringstream os;
    os << "<html><head><title>UserInfo </title></head>\n";
    os << "<body><h1>Welcome ! </h1>\n"; 
    os << username <<"\n";
    os << "<h2>you can : </h2>\n";
    os << "<a href=\"/drive\"> go to drive </a>\r\n" <<"<br/>";
    os << "<a href=\"/changepasswd\"> change your password </a>\r\n" <<"<br/>";
    os << "<a href=\"/maillist\" > view your mail list </a> \r\n <br/>";
    os << "<a href=\"/writemail?from="<<username<<"&to=\"> write a mail  </a> \r\n <br/>";
    os << "<a href=\"/admin\"> admin console </a> <br/>";
    os << "<a href=\"/logout\"> logout </a>\r\n";
    os << "</body> </html>" ;

    return os.str();
}
std::string Handler::renderMailList()
{
    std::ostringstream os;
    os << "<html><head><title>UserInfo </title></head>\n";
    os << "<body><h1>Welcome ! " <<_userName<<" </h1> <br/>\n"; 
    auto mail_list = DataBase::get_instance() ->get_mail(_userName);
    int mail_count = 0;
    int mail_no ;
    for(std::string mail : mail_list)
    {
        std::istringstream is(mail);
        is >>mail_no;
        auto mp = SMTP::parse_email(mail);
    //if(mp.size()<=0) continue;
        os <<"<h3> mail NO:" <<mail_count <<" </h3>";
        os << "<h4>From :" << mp["From"]<<"</h4>";
        os << "<h4>To :" << mp["To"]<<"</h4>";
        os << "<h4>Subject :" << mp["Subject"]<<"</h4>";

        os << mp["content"];
    //os << "<textarea>";
    //os << 
    //os << "</textarea><br/>";
    //os << "<a href=\"/maildetail?mailid="<<mail_count<<"\" >full_text </a><br/>";
        os<<"<br/>";
        os << "<a href=\"/writemail?from="<<mp["From"]
        <<"&to="<<mp["To"]<<"\" >Reply this Mail </a><br/>";
        os << "<a href=\"/deletemail?mailid="<<mail_no<<"\" >Delete this Mail </a><br/>";
        os <<"<a href=\"/forwardmail?mailid="<<mail_no<<"\">Forward this Mail  </a>"; 
        os << "<br/> <br/>";
        mail_count ++;
    }

    os << "</body> </html>" ;
    return os.str();

}

std::string __get_parent_dir(std::string dir) {
    return dir.substr(0, dir.find_last_of('/'));
}

void Handler::list_files(std::string current_dir, std::ostringstream &os) {
    std::vector<FileInfo> infos = _service_user->list_dir(current_dir);
    os << "<pre>";
    os << "<b>Name</b>" << "\t\t";
    os << "<b>Is Directory</b>" << "\t\t";
    os << "<b>Size</b>" << "\t";
    os << "<hr>";

    os << "<a href=\"/drive" << __get_parent_dir(current_dir) << "\">..(parent dir)" << "</a><br><br>";
    for (auto &info : infos) {
        std::string path = current_dir + "/" + info.name;
        os << "<a href=\"/drive" << path << "\">" << info.name << "</a>";
        os << (info.name.length() < 8 ? "\t" : "") << "\t" << std::boolalpha << info.is_dir << (info.name.length() >= 8 ? "\t" : "") << "\t\t" << (info.name.length() < 8 ? "\t" : "") << info.size << "</a>";
        os << "\t" << "<a href=\"/erase" << path << "\">" << "delete" << "</a>";
        os << "<form action=\"/rename\" style=\"margin: 0; padding: 0;\" method=\"post\"><input size=\"35\" type=\"text\" name=\"filename\">";
        os << "<input type=\"hidden\" name=\"filepath\" value=\"" << path << "\">";
        os << "<input style=\"display: inline;\" type=\"submit\" value=\"Rename\" />" << "</form>";

        os << "<form action=\"/move\" style=\"margin: 0; padding: 0;\" method=\"post\"><input size=\"35\" type=\"text\" name=\"newfilepath\"";
        os << " value=\"[absolute path] e.g. /usr/folder\">";
        os << "<input type=\"hidden\" name=\"filepath\" value=\"" << path << "\">";
        os << "<input style=\"display: inline;\" type=\"submit\" value=\"Move\" />" << "</form><br>";
    }
    os << "<hr>";
    os << "</pre>";
}

std::string Handler::renderDrivePage(std::string current_dir)
{
    std::ostringstream os;
    os << "<html><head><title>Drive </title></head>\n";
    os << "<body>";
    os << "<a href=\"/\">Go Back to Home Page</a>";
    os << "<h1>Drive </h1>\n"; 
    os << "<h2>" << current_dir << "</h2>\n";

    list_files(current_dir, os);

    os << "<h2>Create a Folder : </h2>\n";
    os << "<form action=\"/createfolder\" style=\"margin: 0; padding: 0;\" method=\"post\">";
    os << "Folder name: <input size=\"35\" type=\"text\" name=\"foldername\">";
    os << "<input type=\"hidden\" name=\"crtdir\" value=\"" << current_dir << "\">";
    os << "<input type=\"submit\" value=\"Create\" />" << "</form><br>";

    os << "<h2>Upload a File : </h2>\n";
    os << "<form enctype=\"multipart/form-data\" action=\"/upload\" style=\"margin: 0; padding: 0;\" method=\"post\">";
    os << "<input type=\"file\" name=\"file\">";
    os << "<input type=\"hidden\" name=\"" << current_dir << "\">";
    os << "<input type=\"submit\" value=\"Upload\" />" << "</form><br>";

    // os << "<h2>Upload a File : </h2>\n";
    // os << "<input type=\"file\" name=\"file\" value=\"\"";
    // os << "<form action=\"upload\" id=\"uploadFile\" method=\"POST\" >";
    // os << "<input type=\"submit\" value = \"Upload\" > <br/> </form>";

    return os.str();
}

std::string __get_type(std::string dir) {
    dir = dir.substr(dir.find_last_of('/') + 1);
    const char *name = dir.c_str();
    if(strstr(name, ".html"))
        return "text/html";
    else if(strstr(name, ".pdf"))
        return "application/pdf";
    else if(strstr(name, ".png"))
        return "image/png";
    else if(strstr(name, ".gif"))
        return "image/gif";
    else if(strstr(name, ".jpg"))
        return "image/jpg";
    else if(strstr(name, ".jpeg"))
        return "image/jpeg";
    else if(strstr(name, ".css"))
        return "test/css";
    else
        return "text/plain";
}

std::string Handler::renderAdminPage(std::string table_addr){
    KvsAdmin kvsadmin;
    std::vector<SlaveElement> slaves = kvsadmin.get_slaves();
    std::ostringstream os;
    os << "<html><head><title>Admin Console</title>\n";
    os << "<style>";
    os << "table {";
    os << "font-family: arial, sans-serif";
    os << "border-collapse: collapse;";
    os << "width: 100%;";
    os << "}";
    os << "td, th {";
    os << "border: 1px solid #dddddd;";
    os << "text-align: left;";
    os << "padding: 8px;";
    os << "}";
    os << "tr:nth-child(even) {";
    os << "background-color: #dddddd;";
    os << "}";
    os << "</style>";
    os << "</head>";
    os << "<a href=\"/\">Go Back to Home Page</a>";
    os << "<body><h1>Welcome ! </h1>\n"; 
    os << "<h2>Frontend servers</h2>\n";
    os << "<p> 127.0.0.1:80, Active</p>\n";
    os << "<h2>Backend Servers</h2>\n";
    os << "<table>";
    os << "<tr>";
    os << "<th>Address</th>";
    os << "<th>Hash Value</th>";
    os << "<th>Connections</th>";
    os << "<th>Status</th>";
    os << "<th></th><th></th>";
    os << "</tr>";

    os << "<tr>";
    os << "<td>" << kvsadmin.master_address_ << "</td>";
    os << "<td>Master</td><td></td><td>ALIVE</td><td></td><td></td>";
    os << "</tr>";

    for (auto &s : slaves) {
        os << "<tr>";
        os << "<td>" << s.addr << "</td>";
        os << "<td>" << s.hash << "</td>";
        os << "<td>" << s.connections << "</td>";
        if (s.status == "ALIVE")
            os << "<td><font color=\"green\">" << s.status << "</font></td>";
        else
            os << "<td><font color=\"red\">" << s.status << "</font></td>";
        os << "<td><a href=\"/shutdown?addr=" << s.addr << "\">Shut Down</a></td>";
        os << "<td><a href=\"/showtable?addr=" << s.addr << "\">Show Table</a></td>";
        os << "</tr>";
    }

    os << "</table>";
    if (!table_addr.empty()) {
        os << "<h2>Slave Table (" << table_addr << ")</h2>";
        os << "<table>";
        os << "<tr>";
        os << "<th>Table</th>";
        os << "<th>Row</th>";
        os << "<th>Row Hash Value</th>";
        os << "<th>Column Family</th>";
        os << "<th>Column</th>";
        os << "<th>Raw Content</th>";
        os << "</tr>";

        std::vector<Element> table = kvsadmin.get_elements(table_addr);
        for (auto &e : table) {
            os << "<tr>";
            os << "<td>" << e.table_name << "</td>";
            os << "<td>" << e.row_key << "</td>";
            os << "<td>" << e.row_hash << "</td>";
            os << "<td>" << e.col_family << "</td>";
            os << "<td>" << e.col << "</td>";
            os << "<td>" << (e.content.length() > 100 ? e.content.substr(0, 100) : e.content) << "</td>";
            os << "</tr>";
        }
        os << "</table>";
    }
    os << "</body> </html>";

    return os.str();
}

int Handler::handleGet()
{
    //std::size_t pos;
    if (_request.uri == "/")
    {
        if(_islogin == false)
            sendFile();
        else sendPage(renderUserInfo(_userName));
    }

    /***** Admin Console *****/
    else if(_request.uri == "/admin"){
        sendPage(renderAdminPage(""));
    }
    else if (_request.uri.find("/showtable?") != std::string::npos) {
        auto args = parseArgs(_request.uri);
        auto iter = args.find("addr");
        assert(iter != args.end());
        sendPage(renderAdminPage(iter->second));
    }
    else if (_request.uri.find("/shutdown?") != std::string::npos) {
        auto args = parseArgs(_request.uri);
        auto iter = args.find("addr");
        assert(iter != args.end());
        KvsAdmin().shutdown_slave(iter->second);
        sendPage(renderAdminPage(""));
    }
    /***** Admin Console *****/

    else if(    _request.uri== "/index" ||
        _request.uri == "/register" ||
        _request.uri == "/changepasswd")
    {
        sendFile();
    }else if(_request.uri == "/userinfo" )
    {
        if(_islogin == false) {
            sendMsg("404","not found ","need login","/index");
        }
        else sendPage(renderUserInfo(_userName));
    }
    else if(_request.uri == "/maillist")
    {
        if(_islogin == false)
        {
            sendMsg("404","not found ","need login","/index");
        } else sendPage(renderMailList());
    }

    else if(_request.uri.length() >= 6 && _request.uri.substr(0, 6) == "/drive")
    {
        std::string dir = _request.uri.substr(6);
        if(_islogin == false)
        {
            sendMsg("404","not found ","need login","/index");
        } else if (!_service_user->is_exist(dir)) {
            sendMsg("404","not found ","file not found","/index");
        } else if (!_service_user->is_folder(dir)) {
            assert(_service_user != nullptr);
            std::ostringstream msg;
            msg << "HTTP/1.1 200 OK\r\n";
            msg << "Server: Tiny Web Server\r\n";
            std::string result = msg.str() ;
            std::string content = _service_user->download_file(dir);
            std::string type = __get_type(dir);
            if(_request.method!= "HEAD")
            {
                msg << "Content-length: " << content.length() << "\r\n";
                msg << "Content_type: " + type + "\r\n\r\n";
                result= msg.str() + content;
            }

            _outputBuffer.append(result.c_str(), result.size());
            _outputBuffer.sendFd(_connfd);
            close(_connfd);
            _isClosed = true;
        } else {
            sendPage(renderDrivePage(dir));
        }
    }
    else if (_request.uri.length() >= 6 && _request.uri.substr(0, 6) == "/erase") {
        std::string dir = _request.uri.substr(6);
        assert(_service_user != nullptr && _service_user->is_exist(dir));
        _service_user->erase(dir);
        sendPage(renderDrivePage(__get_parent_dir(dir)));
    }

    else if(_request.uri == "/logout")
    {
        if(_islogin == false)
            sendMsg("404","not found ","need login","/index");
        else 
            sendMsg("200","OK" ,"logout success","/index"," ");
    }else if(( _request.uri.find("/writemail?")) != std::string::npos)
    {
        if(_islogin == false)
            sendMsg("404","not found ","need login","/index");
        auto args = parseArgs(_request.uri);
        if(args.find("from") == args.end() ||
            args.find("to") == args.end())
        {
            sendMsg("500","server error" , "args prase eorr","/userinfo");
        }
        else {
            sendPage(renderWriteMailPage(args["from"],args["to"]));
        }
    } else if(_request.uri.find("/deletemail?") != std::string::npos)
    {
        if(_islogin == false)
            sendMsg("404","not found ","need login","/index");
        auto args = parseArgs(_request.uri);
        if(args.find("mailid") == args.end())
        {
            sendMsg("500","server error" , "args prase eorr","/userinfo");
        }
        else if(DataBase::get_instance() ->
            delete_mail(_userName,args["mailid"]) == DataBase::OK) {
            sendMsg("200","OK","delete OK","/maillist");
    }
    else sendMsg("500","server error","delete mail error","/maillist");

}
else if(_request.uri.find("/forwardmail?") != std::string::npos)
{
    if(_islogin == false)
        sendMsg("404","not found ","need login","/index");
    auto args = parseArgs(_request.uri);
    if(args.find("mailid") == args.end())
    {
        sendMsg("500","server error" , "args prase eorr","/userinfo");
    }
    sendPage(renderForwardPage(args["mailid"]));
    return OK;

}

return OK;
}

std::unordered_map <std::string,std::string > Handler::parseCookie()
{
    std::unordered_map <std::string,std::string > ans;
    std::string data = _request.cookie;
    std::size_t found = data.find_first_of("&=");
    while(found != std::string::npos)
    {
        data[found] = ' ';
        found = data.find_first_of("&=",found+1);
    }
    std::istringstream is(data);
    std::string key,value;
    while( is >> key >> value)
    {
        std::cout<<"key : " <<key <<"value :" <<value<<std::endl;
        ans[key] = value;
    } 

    return ans;
}

std::unordered_map <std::string,std::string > Handler::parseArgs(const std::string &args)
{
    std::unordered_map <std::string,std::string > ans;
    std::string data = args;
    std::size_t found ;
    if((found = args.find_first_of('?') )!= std::string::npos)
    {
        ans["uri"] = data.substr(0,found);
        data = data.substr(found+1);
    }

    found = data.find_first_of("&=");
    while(found != std::string::npos)
    {
        data[found] = ' ';
        found = data.find_first_of("&=",found+1);
    }
    std::istringstream is(data);
    std::string key,value;
    while( is >> key )
    {
        if( is >>value) 
        {
            ans[key] = value;
        }else ans[key]= "";
        std::cout<<"prase args : key : " <<key <<"value :" <<value<<std::endl;
    } 
    return ans;

}

char __from_hex(char ch) {
    return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

std::string __url_decode(std::string text) {
    char h;
    std::ostringstream escaped;
    escaped.fill('0');

    for (auto i = text.begin(), n = text.end(); i != n; ++i) {
        std::string::value_type c = (*i);

        if (c == '%') {
            if (i[1] && i[2]) {
                h = __from_hex(i[1]) << 4 | __from_hex(i[2]);
                escaped << h;
                i += 2;
            }
        } else if (c == '+') {
            escaped << ' ';
        } else {
            escaped << c;
        }
    }

    return escaped.str();
}

std::unordered_map <std::string,std::string > Handler::parsePostArgs()
{
    std::unordered_map <std::string,std::string > ans;
    std::string data ;
    if(_request.post_data.size() >0)
    {
        data = _request.post_data[_request.post_data.size()-1];
        data = __url_decode(data);
        std::size_t found = data.find_first_of("&=");
        while(found != std::string::npos)
        {
            data[found] = ' ';
            found = data.find_first_of("&=",found+1);
        }
        std::cout << "parse post .."  << data<<std::endl;
        std::istringstream is(data);
        std::string key,value;
        while( is >> key )
        {
            if(is>>value)
            {
                ans[key] = value;
            }else ans[key] ="";
            std::cout<<"key : " <<key <<"value :" <<value<<std::endl;
        } 

    }

    return ans;
}

std::unordered_map <std::string,std::string > Handler::parseSscPostArgs()
{
    std::unordered_map <std::string,std::string > ans;
    std::string data ;
    if(_request.post_data.size() >0)
    {
        data = _request.post_data[_request.post_data.size()-1];
        std::size_t found = data.find_first_of("&=");
        while(found != std::string::npos)
        {
            data[found] = ' ';
            found = data.find_first_of("&=",found+1);
        }
        std::cout << "parse post .."  << data<<std::endl;
        std::istringstream is(data);
        std::string key,value;
        while( is >> key )
        {
            if(is>>value)
            {
                ans[key] = value;
            }else ans[key] ="";
            std::cout<<"key : " <<key <<"value :" <<value<<std::endl;
        } 

    }

    return ans;
}

int Handler :: handlePost()
{
    //std::cout << "int post" <<std::endl;
    if(_request.uri == "/login")
    {
        std::cout<< "login .." <<std::endl;
        if(_request.post_data.size() > 0)
        {
            std::string data = _request.post_data[0];
            for( std::string _data : _request.post_data)
            {
                if(_data.find("username") != std::string::npos)
                {
                    data = _data;
                    break;
                }
            }
            std::cout <<"post_data :" << data <<std::endl;
            std::size_t diff = data.find_first_of('&');
            std::size_t equal1 = data.find_first_of('=');
            std::size_t equal2 = data.find_last_of('=');
            if(diff == std::string::npos ||
                equal1 == std::string:: npos ||
                equal2 == std::string::npos ) return -1;
                std::string user_name = data.substr(equal1+1,diff-equal1-1);
            std::string password = data.substr(equal2+1);
            std::cout <<"user:" << user_name <<std::endl;
            std::cout <<"pass :" <<password <<std::endl;
            if(DataBase ::get_instance()->
                check_user(user_name,password)!= DataBase::OK)
            {
                sendMsg("200","OK","no such user or invaild password !","/index");
                return OK;
            }
            else {
                std::string sid = Cookie::get_sid(user_name,password);
                sendMsg("200","OK","login success!","/userinfo",sid);

                if (_service_user) free(_service_user);
                _service_user = new StorageServiceUser(user_name, Handler::_conn);
                return OK;

            }
        }

    }

    /******  Drive *******/
    else if (_request.uri == "/rename") {
        // auto args = parseArgs(_request.uri);
        auto mm = parsePostArgs();
        assert(_service_user != nullptr);
        _service_user->rename(mm["filepath"], mm["filename"]);
        sendPage(renderDrivePage(__get_parent_dir(mm["filepath"])));
    }

    else if (_request.uri == "/move") {
        auto mm = parsePostArgs();
        assert(_service_user != nullptr);
        _service_user->move(mm["filepath"], mm["newfilepath"]);
        sendPage(renderDrivePage(__get_parent_dir(mm["filepath"])));        
    }

    else if (_request.uri == "/createfolder") {
        auto mm = parsePostArgs();
        assert(_service_user != nullptr);
        assert(mm.find("crtdir") != mm.end() && mm.find("foldername") != mm.end());
        _service_user->create_folder(mm["crtdir"] + "/" + mm["foldername"]);
        sendPage(renderDrivePage(mm["crtdir"]));
    }

    else if (_request.uri == "/upload") {
        // auto dd = parsePostArgs();
        // TODO: get these three parameters
        std::string filename; // filename parameter
        std::string content; // file content
        std::string crtdir; // the current crtdir parameter
        int content_read_flag = 0;
        int first_content_dis = 0;
        int first_line_skip = 0;
        for( std::string _data : _request.post_data)
        {
            if(strncasecmp(_data.c_str(), "Content-Disposition:", 20) == 0 && first_content_dis == 0) {
                std::size_t found = _data.find("filename", 20, 8);
                filename = _data.substr(found+10);
                // filename.erase(filename.find("\"")+1, filename.end());
                filename.pop_back();
                filename.pop_back();
                filename.pop_back();
                first_content_dis += 1;
                // std::cout << filename.size() << std::endl;
                // std::cout <<"real file_name :" << filename <<std::endl;
            }
            else if(strncasecmp(_data.c_str(), "Content-Disposition:", 20) == 0 && first_content_dis == 1){
                std::size_t found = _data.find("name=", 20, 5);
                crtdir = _data.substr(found+6);
                crtdir.pop_back();
                crtdir.pop_back();
                crtdir.pop_back();
                first_content_dis += 1;
            }
            else{
                if(strncmp(_data.c_str(), _request.boundary.c_str(), _request.boundary.size()) == 0){
                    content_read_flag = 1 - content_read_flag;
                    first_line_skip = 1;
                    // std::cout << "flag" << content_read_flag << std::endl;
                    continue;
                }
                
                if(content_read_flag){
                   if(first_line_skip == 1){
                      first_line_skip = 0;
                      continue;
                  }
                  content += _data;
              }
          }
      }
      content.pop_back();
      content.pop_back();
      _service_user->create_file(crtdir + "/" + filename, content);
      sendPage(renderDrivePage(crtdir));
  }
    /******  Drive *******/

  else if(_request.uri == "/register")
  {
    auto mm = parsePostArgs();
    if(mm.find("username") == mm.end() ||
        mm.find("password") == mm.end() || 
        mm.find("confirm_password") == mm.end())
    {
        sendMsg("500","server error.","post data error","/register");
        return PostDataErr;
    }
    std::cout <<"username " <<mm["username"] <<std::endl;
    std::cout <<"password " <<mm["password"] <<std::endl;
    std::cout <<"confirm_password " <<mm["confirm_password"] <<std::endl;
    if(mm["password"]!= mm["confirm_password"])
    {
        sendMsg("200","OK","password not the same","/register");
        return PostDataErr;
    }
    int errorCode = DataBase ::get_instance() ->
    register_user(mm["username"],mm["password"]);
    if(errorCode== DataBase::OK
        )
    {
        sendMsg("200","OK","register ok.","/index");
        return OK;
    }else if(errorCode == DataBase::ExsitingUser  ){
        sendMsg("200","OK","invalid username .. register fail ..","/register");
        return OK;
    }

}
else if(_request.uri == "/changepasswd")
{
    auto re =parsePostArgs();
    if(!_islogin)
    {
        sendMsg("404","not found","need login ","/index");
        return  NotLoginErr;
    }
    if( re.find("current_password") == re.end() ||
        re.find("password") == re.end() ||
        re.find("confirm_password") == re.end())
    {
        sendMsg("500","server error","post data error" ,"/userinfo");
        return PostDataErr;
    }
    if( DataBase::check_user(_userName,re["current_password"])
        != DataBase::OK)
    {
        std::cout << "let us see where is the bug*******************0" << _passwd <<std::endl;
        sendMsg("200" ,"OK","passwd not right .","/changepasswd");
        return PassWordNotMatch;
    }
    // if(re["current_password"] != _passwd)
    // {
    //     sendMsg("200" ,"OK","passwd not right .","/changepasswd");
    //     return PassWordNotMatch;
    // }
    DataBase::get_instance() ->update_password(_userName,re["password"]);
    sendMsg("200","OK","change password Ok","/userinfo",
        Cookie::get_sid(_userName,re["password"]));
    return OK;

}
else if(_request.uri == "/sendmail")
{
    auto args = parseSscPostArgs();
    if( args.find("from_user") == args.end() ||
        args.find("to_user") == args.end() ||
        args.find("subject") == args.end() ||
        args.find("content") == args.end()
        )
    {
        sendMsg("500","server error","prase arg error.." ,"/userinfo");
    }
    std::cout <<"from_user " << args["from_user"]<<std::endl;
    std::cout <<"to_user " << args["to_user"]<<std::endl;
    std::cout <<"subject " << args["subject"]<<std::endl;
    std::cout <<"content " << args["content"]<<std::endl;
    if (EmailClient ::SendEmail(args["from_user"],
        args["to_user"],
        args["subject"],
        args["content"]))
    {
        std::cout << "here0" << std::endl;
        sendMsg("200","OK" ,"send email ok !" ,"/userinfo");
    }
}


return OK;
}
void Handler::handle()
{
    if(!receiveRequest())
    {
        close(_connfd);
        _isClosed = true;
        return;
    }
    if(_request.method != "GET" &&
        _request.method != "POST" &&
        _request.method != "HEAD")
    {
        sendMsg("501", "Not Implemented",
         "Server doesn't implement this method");
        return;
    }
    _islogin= false;
    auto re = parseCookie();
    if(re.find("sid") != re.end()) 
    {
    //  sendMsg("500","server error","parse cookie error " ,"/index");
    auto userinfo = Cookie::get_user_info_by_sid(re["sid"]); // size_t ?= ull
    if(userinfo!= "" )
    {
        _userName = userinfo;
        //_passwd = userinfo.second;
        _islogin = true;

        if (_service_user) free(_service_user);
        _service_user = new StorageServiceUser(_userName, Handler::_conn);
       // sendPage(renderUserInfo(userinfo.first));
    }
}
parseURI();
if(_request.method == "GET" || _request.method == "HEAD")
{
    handleGet();
}else if(_request.method == "POST")
{
    handlePost();
}

}

bool Handler::receiveRequest()
{ 
    if(_inputBuffer.readFd(_connfd) == 0)
        return false;
    std::string request = _inputBuffer.readAllAsString();
    std::cout << "---------------------------Request---------------------------" << std::endl;
    std::cout << request << std::endl;
    std::cout << "-------------------------------------------------------------" << std::endl;
    Parser p(request);
    _request = p.getParseResult();
    return true;
}

void Handler::sendMsg(const std::string &errorNum,
  const std::string &shortMsg,
  const std::string &longMsg,
  const std::string &backUri,
  const std::string &cookie_sid)
{
    std::ostringstream os;
    std::ostringstream msg;
    os<< "<html><title>penn cloud</title>";
    os<<  "<body bgcolor=""ffffff"">\r\n";
    os<<  errorNum + ": " + shortMsg + "\r\n";
    os<<  "<p>" + longMsg + ": " + _fileName + "\r\n";
    os<< "<hr><em>The Tiny Web server</em>\r\n";
    if(backUri != "")
    {
        os<< "<a href=\""<<backUri<<"\" >\r\n" <<
        "Back" << "</a>" ;
    }
    os <<"</hr>\r\n</body></html>"; 
    msg << "HTTP /1.0 " + errorNum + " " + shortMsg + "\r\n";
    if(cookie_sid !="")
    {
     msg <<"Set-Cookie: sid="<<cookie_sid <<"\r\n";
 }
 if(_request.method != "HEAD")
 {
    msg << "Content_type: text/html\r\n";
    msg << std::string("Content-length:" )<< os.str().length() <<"\r\n";
}
std::string result = msg.str() + "\r\n";
if(_request.method != "HEAD")
 result += os.str();
_outputBuffer.append(result.c_str(), result.size());
_outputBuffer.sendFd(_connfd);
close(_connfd);
_isClosed = true;
}

void Handler::parseURI()
{
    _fileName = ".";
    if(_request.uri == "/" || _request.uri == "/index")
        _fileName += "/home.html";
    else if(_request.uri == "/register")
        _fileName +="/register.html";
    else if(_request.uri== "/userinfo")
        _fileName +="/userinfo.html";
    else if(_request.uri== "/changepasswd")
        _fileName +="/changepasswd.html";
}

void Handler::getFileType()
{
    const char *name = _fileName.c_str();
    if(strstr(name, ".html"))
        _fileType = "text/html";
    else if(strstr(name, ".pdf"))
        _fileType = "application/pdf";
    else if(strstr(name, ".png"))
        _fileType = "image/png";
    else if(strstr(name, ".gif"))
        _fileType = "image/gif";
    else if(strstr(name, ".jpg"))
        _fileType = "image/jpg";
    else if(strstr(name, ".jpeg"))
        _fileType = "image/jpeg";
    else if(strstr(name, ".css"))
        _fileType = "test/css";
    else
        _fileType = "text/plain";
}