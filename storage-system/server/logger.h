#ifndef _LOGGER_H
#define _LOGGER_H

#include "calldata.h"
#include "kvs.pb.h"
#include <string>

using kvs::PutRequest;
using kvs::DeleteRequest;
using kvs::ColFamily;
using kvs::Col;

class Logger {
public:
  Logger(std::string slave_address);
  void log_put(PutRequest &request);
  void log_delete(DeleteRequest &request);
  void print_log();
  void clear();
private:
  std::string slave_address_;
};

#endif