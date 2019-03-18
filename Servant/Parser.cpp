#include "Parser.h"

bool Parser::init_flag = false;
std::unordered_set<std::string> Parser:: known_attr ;


void Parser::init_set()
{
    known_attr.insert("Host:");
    known_attr.insert("Connection:");
    known_attr.insert("User-Agent:");
    known_attr.insert("Accept:");
    known_attr.insert("Referer:");
    known_attr.insert("Accept-Encoding:");
    known_attr.insert("Accept-Language:");
    known_attr.insert("Content-Length:");
    known_attr.insert("Cache-Control:");
    known_attr.insert("Origin:");
    known_attr.insert("Upgrade-Insecure-Requests:");
    known_attr.insert("Content-Type:");
    known_attr.insert("Cookie:");

}

Parser::Parser(const std::string request)
{
    assert(request.size() > 0);
    if(init_flag == false)
    {
    init_set();
    init_flag = true;
    }
    this->_request = request;
}

HTTPRequest Parser::getParseResult()
{
    //parseLine();
    std::size_t found = _request.find_first_of('\n');
    _lines.push_back(_request.substr(0,found));
    while(found!= std::string::npos)
    {
        std::size_t tt = _request.find_first_of('\n',found+1);
        _lines.push_back(_request.substr(found+1,tt-found));
        found  = tt;
    }
    /*
    for(std::string line : _lines)
    {
    std::cout <<"lines :" <<line <<std::endl;
    }
    */
    parseRequestLine();
    parseHeaders();
    if(_parseResult.method == "POST")
    parsePostData();
    return _parseResult;
}
void Parser::parsePostData()
{
    for(int i = 1;i<_lines.size();++i)
    {
        std::size_t pos =_lines[i].find_first_of(':');
        if(pos == std::string::npos || 
            (pos != std::string::npos &&
            known_attr.find(_lines[i].substr(0,pos+1)) == known_attr.end())
            )
        {
            //std::cout << "judge " <<_lines[i].substr(0,pos)<<std::endl;
            if(_lines[i] .size() > 0)
              _parseResult.post_data.push_back(_lines[i]);
            //std::cout<<"parse post data" <<_lines[i]<<std::endl;
        }
    }

}

void Parser::parseLine()
{
    std::string::size_type lineBegin = 0;   
    std::string::size_type checkIndex = 0;  

    while(checkIndex < _request.size())
    {
        if(_request[checkIndex] == '\r')
        {
            if((checkIndex + 1) == _request.size())
            {
                std::cout << "Request not to read the complete." << std::endl;
                return;
            }
            else if(_request[checkIndex+1] == '\n')
            {
                _lines.push_back(std::string(_request, lineBegin,
                            checkIndex - lineBegin));
                checkIndex += 2;
                lineBegin = checkIndex;
            }
            else
            {
                std::cout << "Request error." << std::endl;
                return;
            }
        }
        else
            ++checkIndex;
    }
    return;
}

void Parser::parseRequestLine()
{
    assert(_lines.size() > 0);
    std::string requestLine = _lines[0];

    auto first_ws = std::find_if(requestLine.cbegin(), requestLine.cend(),
            [](char c)->bool { return (c == ' ' || c == '\t'); });

    if(first_ws == requestLine.cend())
    {
        std::cout << "Request error." << std::endl;
        return;
    }
    _parseResult.method = std::string(requestLine.cbegin(), first_ws);

    auto reverse_last_ws = std::find_if(requestLine.crbegin(), requestLine.crend(),
            [](char c)->bool { return (c == ' ' || c == '\t'); });
    auto last_ws = reverse_last_ws.base();
    _parseResult.version = std::string(last_ws, requestLine.cend());

    while((*first_ws == ' ' || *first_ws == '\t') && first_ws != requestLine.cend())
        ++first_ws;

    --last_ws;
    while((*last_ws == ' ' || *last_ws == '\t') && last_ws != requestLine.cbegin())
        --last_ws;

    _parseResult.uri = std::string(first_ws, last_ws + 1);
}

void Parser::parseHeaders()
{
    assert(_lines.size() > 0);
    for(int i = 1; i < _lines.size(); ++i)
    {
        if(_lines[i].empty()) 
            return;
        else if(strncasecmp(_lines[i].c_str(), "Host:", 5) == 0) 
        {
            auto iter = _lines[i].cbegin() + 5;
            while(*iter == ' ' || *iter == '\t')
                ++iter;
            _parseResult.host = std::string(iter, _lines[i].cend());
        }
        else if(strncasecmp(_lines[i].c_str(), "Connection:", 11) == 0) 
        {
            auto iter = _lines[i].cbegin() + 11;
            while(*iter == ' ' || *iter == '\t')
                ++iter;
            _parseResult.connection = std::string(iter, _lines[i].cend());
        }
    else if(_lines[i].find("Cookie:") != std::string::npos)
    {
        _parseResult.cookie = _lines[i].substr(
            _lines[i].find_first_of(':') +1);
    }
    else if(_lines[i].find("Content-Type:") != std::string::npos){
        if(_lines[i].find("multipart") !=std::string::npos)
        {
            std::string boundary = "--"+_lines[i].substr(_lines[i].find("boundary=") +9);
            // std::cout << "Test boundary: " << boundary << std::endl;
            boundary.pop_back();
            boundary.pop_back();
            _parseResult.boundary = boundary;
        }
    }
    else
        {
        }
    }
}
