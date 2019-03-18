#ifndef _COMMON_H
#define _COMMON_H

#include <string>
#include <unordered_map>
#include <iostream>

typedef unsigned char byte;

typedef std::unordered_map<std::string, std::unordered_map<std::string, std::pair<byte*, int>>> RowType;

struct Slave {
  std::string addr;
  int connections;
  int hash;
  bool is_alive;
};

void log_print(std::string info);

#endif