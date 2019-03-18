#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <unordered_map>
#include <grpcpp/grpcpp.h>
#include "table.h"
#include "table-descriptor.h"

#include "kvs.grpc.pb.h"

class Connection {
  std::unordered_map<std::string, Table> tables;

  std::unique_ptr<kvs::Kvs::Stub> stub_;
public:
  Connection(std::string master_addr);
  void create_table(const TableDescriptor &table_descriptor);
  Table get_table(std::string table_name);
  void delete_table(std::string table_name);
};

#endif