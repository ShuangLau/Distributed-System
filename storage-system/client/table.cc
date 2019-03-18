#include "table.h"
#include <functional>
#include <cassert>
#include <grpcpp/grpcpp.h>
#include "kvs.grpc.pb.h"
#include <cstdlib>
#include <chrono>
#include <thread>

Table::Table(const Table &t) : master_stub_(t.master_stub_), ring_(t.ring_), table_name_(t.table_name_) {}

Table::Table(std::unique_ptr<kvs::Kvs::Stub> &stub, std::vector<std::pair<size_t, std::vector<Slave>>> ring,
  std::string table_name) : master_stub_(stub), ring_(std::move(ring)), table_name_(std::move(table_name)) {

}

// std::vector<std::string> __find_min_conn_slave(std::vector<Slave> &v) {
//   int min_conn = INT_MAX;
//   std::string dest_slave;
//   for (auto &s : v) {
//     if (s.connections < min_conn) {
//       min_conn = s.connections;
//       dest_slave = s.addr;
//     }
//   }
//   return std::move(dest_slave);
// }

std::vector<Slave>& Table::get_sorted_dest_slaves(std::string row_key) {
  size_t hash = std::hash<std::string>{}(row_key);
  // std::cout << hash << ": " << row_key << std::endl;
  for (auto &p : ring_) {
    if (hash < p.first) {
      assert(!p.second.empty());
      sort(p.second.begin(), p.second.end(), [](Slave &a, Slave &b) { return a.connections < b.connections; });
      return p.second;
    }
  }
  assert(!ring_.rbegin()->second.empty());
  sort(ring_.rbegin()->second.begin(), ring_.rbegin()->second.end(), [](Slave &a, Slave &b) { return a.connections < b.connections; });
  return ring_.rbegin()->second;
}
std::mutex io_mutex;
void ShowConnections(std::vector<Slave> &slaves) {
  std::lock_guard<std::mutex> lock(io_mutex);
  for (auto &s : slaves) {
    std::cout << s.addr << "=" << s.connections << std::endl;
  }
  std::cout << "-----------------" << std::endl;
}

Result Table::get(const Get & g) {
  std::vector<Slave> &dest_slave = get_sorted_dest_slaves(g.get_row_key());
  int R = 1;
  sort(dest_slave.begin(), dest_slave.begin() + R, [](Slave &a, Slave &b) { return a.addr < b.addr; }); // ordered by the address to avoid deadlock

  std::string slave_to_query = dest_slave[0].addr;

  while (1) {
    std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(slave_to_query, grpc::InsecureChannelCredentials())));
    Operation::update_connections(master_stub_, dest_slave[0].addr, 1);

    kvs::GetRequest request;
    request.set_table_name(table_name_);
    request.set_row_key(g.get_row_key());

    kvs::GetReply reply;
    grpc::ClientContext context;

    grpc::Status status = stub->Get(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      RowType row;
      for (int i = 0; i < reply.col_families_size(); i++) {
        const kvs::ColFamily &col_family = reply.col_families(i);
        for (int j = 0; j < col_family.cols_size(); j++) {
          const kvs::Col &col = col_family.cols(j);
          std::pair<byte*, int> &p = row[col_family.col_family()][col.col()];
          assert(!col.content().empty() && col.content().length() != 0);
          p.first = (byte*)malloc(sizeof(byte) * col.content().length());
          memcpy(p.first, col.content().c_str(), col.content().length());
          p.second = col.content().length();
        }
      }
      // std::cout << "Get message: " << reply.message() << std::endl;
      Operation::update_connections(master_stub_, dest_slave[0].addr, -1);
      return Result(std::move(row));
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      Operation::update_connections(master_stub_, dest_slave[0].addr, -1);
      // throw "get error";
      kvs::AliveRequest alive_request;
      alive_request.set_src_addr(slave_to_query);
      kvs::AliveReply alive_reply;
      grpc::ClientContext context;
      grpc::Status status = master_stub_->Alive(&context, alive_request, &alive_reply);
      if (status.ok()) {
        slave_to_query = alive_reply.alive_addr();
      } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      }
    }
  }
  return Result(RowType());
}

void Table::put(const Put & p) {
  // W == N
  // size_t hash = std::hash<std::string>{}(p.get_row_key());
  // std::vector<Slave> *dest_slave = nullptr;

  // for (auto &pair : ring_) {
  //   if (hash > pair.first) {
  //     assert(!pair.second.empty());
  //     dest_slave = &(pair.second);
  //     break;
  //   }
  // }
  // if (!dest_slave) dest_slave = &(ring_.rbegin()->second);
  // sort((*dest_slave).begin(), (*dest_slave).end(), [](Slave &a, Slave &b) { return a.connections < b.connections; });

  std::vector<Slave> &dest_slave = get_sorted_dest_slaves(p.get_row_key());
  int W = dest_slave.size();
  sort(dest_slave.begin(), dest_slave.begin() + W, [](Slave &a, Slave &b) { return a.addr < b.addr; }); // ordered by the address to avoid deadlock

  for (int i = 0; i < W; i++) {
    // std::this_thread::sleep_for (std::chrono::seconds(rand() % 5));
    Slave &s = dest_slave[i];
    std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
    Operation::update_connections(master_stub_, s.addr, 1);

    kvs::PutRequest request;
    request.set_table_name(table_name_);
    request.set_row_key(p.get_row_key());

    kvs::EmptyReply reply;
    grpc::ClientContext context;

    const RowType &row = p.get_row();
    for (auto &p0 : row) {
      kvs::ColFamily *col_family = request.add_col_families();
      col_family->set_col_family(p0.first);
      for (auto &p1 : p0.second) {
        kvs::Col *col = col_family->add_cols();
        col->set_col(p1.first);
        assert(p1.second.first != nullptr && p1.second.second > 0);
        col->set_content(std::string((char*)(p1.second.first), p1.second.second));
      }
    }

    grpc::Status status = stub->Put(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      // {
      //   std::lock_guard<std::mutex> lock(io_mutex);
      //   for (auto &p0 : row) {
      //     for (auto &p1 : p0.second) {
      //       std::cout << "OK: " << std::string((char*)(p1.second.first), p1.second.second) << std::endl;
      //     }
      //   }
      // }
      // std::cout << "Get message: " << reply.message() << std::endl;
      Operation::update_connections(master_stub_, s.addr, -1);
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      Operation::update_connections(master_stub_, s.addr, -1);
      // throw "get error";
    }
  }
  // std::unordered_map<std::string, std::unordered_map<std::string, std::pair<byte*, int>>> &m = table_map_[p.get_row_key()];
  // for (auto &i : p.get_indices()) {
  //   for (auto &j : i.second) {
  //     m[i.first][j.first] = j.second;
  //   }
  // }
}

bool Table::del(const Delete &d) {
  std::vector<Slave> &dest_slave = get_sorted_dest_slaves(d.get_row_key());
  int W = dest_slave.size();
  sort(dest_slave.begin(), dest_slave.begin() + W, [](Slave &a, Slave &b) { return a.addr < b.addr; });
  bool delete_result = false;
  for (int i = 0; i < W; i++) {
    Slave &s = dest_slave[i];
    std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
    Operation::update_connections(master_stub_, s.addr, 1);

    kvs::DeleteRequest request;
    request.set_table_name(table_name_);
    request.set_row_key(d.get_row_key());

    kvs::DeleteReply reply;
    grpc::ClientContext context;

    const DeleteRowType &row = d.get_row();
    for (auto &p0 : row) {
      kvs::ColFamily *col_family = request.add_col_families();
      col_family->set_col_family(p0.first);
      for (auto &column : p0.second) {
        kvs::Col *col = col_family->add_cols();
        col->set_col(column);
        // assert(p1.second.first != nullptr && p1.second.second > 0);
        // col->set_content(std::string((char*)(p1.second.first), p1.second.second));
      }
    }

    grpc::Status status = stub->Delete(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      // if (i) assert(delete_result == reply.is_succeeded());
      delete_result = reply.is_succeeded();
      // std::cout << "Get message: " << reply.message() << std::endl;
      Operation::update_connections(master_stub_, s.addr, -1);
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      Operation::update_connections(master_stub_, s.addr, -1);
      // throw "get error";
    }
  }
  return delete_result;
}

bool Table::check_and_put(std::string row, std::string column_family, std::string column, byte *content, int content_len, const Put &p) {
  // currently if the row keys are different, no checking is guaranteed
  if (row != p.get_row_key()) {
    put(p);
    return true;
  }

  std::vector<Slave> &dest_slave = get_sorted_dest_slaves(row);

  sort(dest_slave.begin(), dest_slave.end(), [](Slave &a, Slave &b) { return a.addr < b.addr; });
  
  // try to lock
  int total = dest_slave.size();
  int lock_counter = -1;
  while (1) {
    // std::cout << "lock_counter = " << lock_counter << std::endl;
    lock_counter = acquire_or_abort_locks(dest_slave, total, true, row);
    // if all locks are acquired
    if (lock_counter == total) break;

    // printf("release\n");
    // release lock
    assert(acquire_or_abort_locks(dest_slave, lock_counter, false, row) == 0);

    // retry after a random time interval (0 ~ 499ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 2000 + 1000));
  }
  assert(lock_counter != -1);
  int W = dest_slave.size();
  int R = 1;

  // check request
  std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(dest_slave[0].addr, grpc::InsecureChannelCredentials())));
  Operation::update_connections(master_stub_, dest_slave[0].addr, 1);

  kvs::CputCheckRequest request;
  request.set_table_name(table_name_);
  request.set_row_key(row);
  request.set_col_family(column_family);
  request.set_col(column);
  request.set_content(std::string((char*)(content), content_len));

  kvs::CputCheckReply reply;
  grpc::ClientContext context;

  grpc::Status status = stub->CputCheck(&context, request, &reply);

  // Act upon its status.
  if (status.ok()) {
    // if check succeeded, start putting
    if (reply.is_same()) {
      for (int i = 0; i < W; i++) {
        // std::this_thread::sleep_for (std::chrono::seconds(rand() % 5));
        Slave &s = dest_slave[i];
        std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
        Operation::update_connections(master_stub_, s.addr, 1);

        kvs::PutRequest request;
        request.set_table_name(table_name_);
        request.set_row_key(p.get_row_key());

        kvs::EmptyReply reply;
        grpc::ClientContext context;

        const RowType &row = p.get_row();
        for (auto &p0 : row) {
          kvs::ColFamily *col_family = request.add_col_families();
          col_family->set_col_family(p0.first);
          for (auto &p1 : p0.second) {
            kvs::Col *col = col_family->add_cols();
            col->set_col(p1.first);
            assert(p1.second.first != nullptr && p1.second.second > 0);
            // std::cout << "cput: " << std::string((char*)(p1.second.first), p1.second.second) << std::endl;
            col->set_content(std::string((char*)(p1.second.first), p1.second.second));
          }
        }

        grpc::Status status = stub->CputPut(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
          // {
          //   std::lock_guard<std::mutex> lock(io_mutex);
          //   for (auto &p0 : row) {
          //     for (auto &p1 : p0.second) {
          //       std::cout << "OK: " << std::string((char*)(p1.second.first), p1.second.second) << std::endl;
          //     }
          //   }
          // }
          // std::cout << "Get message: " << reply.message() << std::endl;
          Operation::update_connections(master_stub_, s.addr, -1);
        } else {
          std::cout << status.error_code() << ": " << status.error_message() << std::endl;
          Operation::update_connections(master_stub_, s.addr, -1);
          // throw "get error";
        }
      }
      // std::cout << "here0" << std::endl;
      Operation::update_connections(master_stub_, dest_slave[0].addr, -1);
      assert(acquire_or_abort_locks(dest_slave, lock_counter, false, row) == 0);
      return true;
    } else {
      // std::cout << "here1" << std::endl;
      Operation::update_connections(master_stub_, dest_slave[0].addr, -1);
      assert(acquire_or_abort_locks(dest_slave, lock_counter, false, row) == 0);
      return false;
    }
    // std::cout << "Get message: " << reply.message() << std::endl;
    
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    // throw "get error";
  }
  return false;
  // std::vector<Slave> &put_slave = get_sorted_dest_slaves(p.get_row_key());
  // std::vector<Slave> &check_slave = get_sorted_dest_slaves(row);

  // int W = put_slave.size();
  // int R = 1;

  // // could use merge sort to improve the efficiency
  // std::vector<std::pair<Slave, bool>> combine_slave; // if true, it's put_slave
  // for (int i = 0; i < W; i++) combine_slave.push_back({put_slave[i], true});
  // for (int i = 0; i < R; i++) combine_slave.push_back({check_slave[i], false});
  // sort(combine_slave.begin(), combine_slave.end(), [](std::pair<Slave, bool> &a, std::pair<Slave, bool> &b) { return a.first.addr < b.first.addr; });
  
  // // try to lock
  // int total = combine_slave.size();
  // assert(total == W + R);
  // while (1) {
  //   int lock_counter = 0;
  //   for (int i = 0; i < total; i++) {
  //     Slave &s = combine_slave[i].first;
  //     std::string row_key;
  //     row_key = combine_slave[i].second ? p.get_row_key() : row;

  //     std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
  //     Operation::update_connections(master_stub_, s.addr, 1);

  //     kvs::PreCputRequest request;
  //     request.set_table_name(table_name_);
  //     request.set_row_key(row_key);
  //     request.set_acquire_or_abort(true);

  //     kvs::EmptyReply reply;
  //     grpc::ClientContext context;

  //     grpc::Status status = stub->PreCput(&context, request, &reply);

  //     // Act upon its status.
  //     if (status.ok()) {
  //       // std::cout << "Get message: " << reply.message() << std::endl;
  //       Operation::update_connections(master_stub_, s.addr, -1);
  //     } else {
  //       std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  //       throw "get error";
  //     }

  //     if (reply.is_locked()) lock_counter++; else break;
  //   }
  //   // if acquires all the locks
  //   if (lock_counter == total) break;

  //   // release lock
  //   for (int i = 0; i < lock_counter; i++) {
  //     Slave &s = combine_slave[i].first;
  //     std::string row_key;
  //     row_key = combine_slave[i].second ? p.get_row_key() : row;

  //     std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
  //     Operation::update_connections(master_stub_, s.addr, 1);

  //     kvs::PreCputRequest request;
  //     request.set_table_name(table_name_);
  //     request.set_row_key(row_key);
  //     request.set_acquire_or_abort(false);

  //     kvs::PreCputReply reply;
  //     grpc::ClientContext context;

  //     grpc::Status status = stub->PreCput(&context, request, &reply);

  //     // Act upon its status.
  //     if (status.ok()) {
  //       // std::cout << "Get message: " << reply.message() << std::endl;
  //       Operation::update_connections(master_stub_, s.addr, -1);
  //     } else {
  //       std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  //       throw "get error";
  //     }
  //   }

  //   // retry after a random time interval (0 ~ 499ms)
  //   std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 500));
  // }
}

// return value doesn't make any sense when release locks
int Table::acquire_or_abort_locks(std::vector<Slave> &slaves, int total, bool acquire_or_abort, std::string row) {
  // if (acquire_or_abort) printf("acquire\n"); else printf("abort\n");
  int lock_counter = 0;
  for (int i = 0; i < total; i++) {
    Slave &s = slaves[i];

    std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
    Operation::update_connections(master_stub_, s.addr, 1);

    kvs::PreCputRequest request;
    request.set_table_name(table_name_);
    request.set_row_key(row);
    request.set_acquire_or_abort(acquire_or_abort);

    kvs::PreCputReply reply;
    grpc::ClientContext context;

    // printf("here\n");
    grpc::Status status = stub->PreCput(&context, request, &reply);
    // std::cout << "locked: " << reply.is_locked() << std::endl;
    // std::cout << "message: " << reply.message() << std::endl;
    // Act upon its status.
    if (status.ok()) {
      // printf("return\n");
      // std::cout << "Get message: " << reply.message() << std::endl;
      Operation::update_connections(master_stub_, s.addr, -1);
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
      // throw "get error";
      Operation::update_connections(master_stub_, s.addr, -1);
    }

    if (acquire_or_abort) {
      // std::cout << "locked: " << reply.is_locked() << std::endl;
      // acquire locks
      if (reply.is_locked()) lock_counter++; else break;
    }
  }
  return lock_counter;
}

void Table::print_all_replicas(std::string r, std::string cf, std::string cl) {
  std::vector<Slave> &dest_slave = get_sorted_dest_slaves(r);
  // ShowConnections(dest_slave);
  sort(dest_slave.begin(), dest_slave.end(), [](Slave &a, Slave &b) { return a.addr < b.addr; }); // ordered by the address to avoid deadlock

  std::lock_guard<std::mutex> lock(io_mutex);
  {
    for (auto &s : dest_slave) {
      std::unique_ptr<kvs::Kvs::Stub> stub(kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));

      kvs::GetRequest request;
      request.set_table_name(table_name_);
      request.set_row_key(r);

      kvs::GetReply reply;
      grpc::ClientContext context;

      grpc::Status status = stub->Get(&context, request, &reply);

      // Act upon its status.
      if (status.ok()) {
        RowType row;
        for (int i = 0; i < reply.col_families_size(); i++) {
          const kvs::ColFamily &col_family = reply.col_families(i);
          for (int j = 0; j < col_family.cols_size(); j++) {
            const kvs::Col &col = col_family.cols(j);
            std::pair<byte*, int> &p = row[col_family.col_family()][col.col()];
            assert(!col.content().empty() && col.content().length() != 0);
            p.first = (byte*)malloc(sizeof(byte) * col.content().length());
            memcpy(p.first, col.content().c_str(), col.content().length());
            p.second = col.content().length();

            if (col_family.col_family() == cf && col.col() == cl) {
              
              std::cout << s.addr << ": ";
              std::cout << "[" << r << "]";
              std::cout << "[" << cf << "]";
              std::cout << "[" << cl << "]";
              std::cout << "=" << col.content() << std::endl;
            }
          }
        }


      } else {
        std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        // throw "get error";
      }
    }
    std::cout << "===========================" << std::endl;
  }
}