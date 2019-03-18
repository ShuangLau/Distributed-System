#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <string>
#include <cassert>
#include <iostream>
#include <algorithm>
#include <string.h>
#include <unordered_set>

typedef struct
{
    std::string method;     
    std::string uri;        
    std::string version;    
    std::string host;       
    std::string connection; 
    std::string cookie;
    std::string boundary;
    std::vector < std::string > post_data;  
} HTTPRequest;

class Parser
{
public:
    static std::unordered_set <std::string > known_attr;
    Parser(const std::string request);
    HTTPRequest getParseResult();
private:
    static bool init_flag ;
    static void init_set();
    void parseLine();        
    void parseRequestLine(); 
    void parseHeaders();     
    void parsePostData();
    std::string _request;    
    std::vector<std::string> _lines;
    HTTPRequest _parseResult;
};

#endif // PARSER_H
