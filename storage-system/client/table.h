#ifndef _TABLE_H
#define _TABLE_H

#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include "get.h"
#include "put.h"
#include "delete.h"
#include "result.h"
#include "../common.h"

class Table {
  std::unordered_map<std::string, std::unordered_map<std::string, std::unordered_map<std::string, std::pair<byte*, int>>>> table_map_; // do not use
  
  std::vector<std::pair<size_t, std::vector<Slave>>> ring_;
  std::unique_ptr<kvs::Kvs::Stub> &master_stub_;
  std::string table_name_;

  std::vector<Slave> &get_sorted_dest_slaves(std::string row_key);

  int acquire_or_abort_locks(std::vector<Slave> &slaves, int total, bool acquire_or_abort, std::string row);
public:
  Table(const Table &t);
  Table(std::unique_ptr<kvs::Kvs::Stub> &stub, std::vector<std::pair<size_t, std::vector<Slave>>> ring, std::string table_name);
  Result get(const Get & g);
  void put(const Put & p);
  bool del(const Delete &d);
  bool check_and_put(std::string row, std::string column_family, std::string column, byte *content, int content_len, const Put &p);

  // for testing
  void print_all_replicas(std::string r, std::string cf, std::string cl);
};

#endif