#ifndef _KVSADMIN_H
#define _KVSADMIN_H

#include <string>
#include <vector>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "../storage-system/client/kvs.grpc.pb.h"

using grpc::ClientContext;
using grpc::Status;

using kvs::Row;
using kvs::ShutdownRequest;
using kvs::DataAndLog;
using kvs::EmptyRequest;
using kvs::EmptyReply;
using kvs::ColFamily;
using kvs::Col;
using kvs::Kvs;
using kvs::Data;


struct Element {
  std::string table_name;
  std::string row_key;
  std::string col_family;
  std::string col;
  std::string content;

  std::string row_hash;
};

struct SlaveElement {
  std::string addr;
  std::string connections;
  std::string hash;
  std::string status;
};

class KvsAdmin {
public:
  std::string master_address_ = "127.0.0.1:50051";
  // KvsAdmin();
  void shutdown_slave(std::string slave_address);
  // void restart_slave(std::string slave_address);
  std::vector<Element> get_elements(std::string slave_address);
  std::vector<SlaveElement> get_slaves();
};

#endif