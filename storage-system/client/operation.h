#ifndef _QUERY_H
#define _QUERY_H

#include <string>
#include "kvs.grpc.pb.h"

class Operation {
  std::string row_key;
public:
  Operation(std::string row_key);
  const std::string & get_row_key() const;
  static void update_connections(std::unique_ptr<kvs::Kvs::Stub> &master_stub, const std::string &slave_addr, int delta);
};

#endif