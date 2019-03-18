#ifndef _COMMON_H
#define _COMMON_H

#include <string>

typedef unsigned char byte;

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::pair<byte*, int>>> RowType;

struct Slave {
  std::string addr;
  int connections;
};

#endif