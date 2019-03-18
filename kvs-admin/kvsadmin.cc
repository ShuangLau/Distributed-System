#include "kvsadmin.h"

using kvs::Table;

void KvsAdmin::shutdown_slave(std::string slave_address) {
  std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(slave_address, grpc::InsecureChannelCredentials())));
  ShutdownRequest request;
  request.set_is_terminated(true);
  EmptyReply reply;
  ClientContext context;
  Status status = stub->Shutdown(&context, request, &reply);
  if (status.ok()) {
    
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  }
}

// void KvsAdmin::restart_slave(std::string slave_address) {
//   std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(slave_address, grpc::InsecureChannelCredentials())));
//   ShutdownRequest request;
//   request.set_is_terminated(false);
//   EmptyReply reply;
//   ClientContext context;
//   Status status = stub->Shutdown(&context, request, &reply);
//   if (status.ok()) {
    
//   } else {
//     std::cout << status.error_code() << ": " << status.error_message() << std::endl;
//   }
// }

std::vector<Element> KvsAdmin::get_elements(std::string slave_address) {
  std::vector<Element> result;

  std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(slave_address, grpc::InsecureChannelCredentials())));
  EmptyRequest request;
  DataAndLog reply;
  ClientContext context;
  Status status = stub->DownloadCheckPoint(&context, request, &reply);
  if (status.ok()) {
    const Data &data = reply.data();

    for (int i = 0; i < data.tables_size(); i++) {
      const Table &table = data.tables(i);
      for (int j = 0; j < table.rows_size(); j++) {
        const Row &row = table.rows(j);
        for (int k = 0; k < row.col_families_size(); k++) {
          const ColFamily &col_family = row.col_families(k);
          for (int l = 0; l < col_family.cols_size(); l++) {
            const Col &col = col_family.cols(l);

            Element e;
            e.table_name = table.table_name();
            e.row_key = row.row_key();
            e.col_family = col_family.col_family();
            e.col = col.col();
            e.content = col.content();

            e.row_hash = std::to_string(std::hash<std::string>{}(e.row_key));
            result.push_back(e);
          }
        }
      }
    }
    return std::move(result);
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  }
  return std::move(result);
}

std::vector<SlaveElement> KvsAdmin::get_slaves() {
  std::vector<SlaveElement> result;

  std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(master_address_, grpc::InsecureChannelCredentials())));
  kvs::TableRequest request;
  kvs::TableReply reply;
  grpc::ClientContext context;
  grpc::Status status = stub->AskTable(&context, request, &reply);

  if (status.ok()) {
    for (int i = 0; i < reply.pairs_size(); i++) {
      SlaveElement s;
      s.hash = std::to_string(reply.pairs(i).key());
      auto pair = reply.pairs(i);
      for (int j = 0; j < pair.slaves_size(); j++) {
        // ring[i].second.push_back(Slave{ .addr = pair.slaves(j).addr(), .connections = pair.slaves(j).connections() });

        s.addr = pair.slaves(j).addr();
        s.connections = std::to_string(pair.slaves(j).connections());
        s.status = pair.slaves(j).is_alive() ? "ALIVE" : "DOWN";

        result.push_back(s);
      }
    }
    return std::move(result);
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
  }
  return std::move(result);
}