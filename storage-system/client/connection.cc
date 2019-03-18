 #include "connection.h"

Connection::Connection(std::string master_addr) : stub_(kvs::Kvs::NewStub(grpc::CreateChannel(master_addr, grpc::InsecureChannelCredentials()))) {
  
}

void Connection::create_table(const TableDescriptor &table_descriptor) {
  // tables[table_descriptor.get_table_name()] = Table();
}

Table Connection::get_table(std::string table_name) {
  kvs::TableRequest request;
  request.set_table_name(table_name);

  kvs::TableReply reply;
  grpc::ClientContext context;

  // The actual RPC.
  grpc::Status status = stub_->AskTable(&context, request, &reply);
  std::vector<std::pair<size_t, std::vector<Slave>>> ring(reply.pairs_size(), std::pair<size_t, std::vector<Slave>>());

  // Act upon its status.
  if (status.ok()) {
    for (int i = 0; i < reply.pairs_size(); i++) {
      ring[i].first = reply.pairs(i).key();
      // std::cout << "key " << reply.pairs(i).key() << ": ";
      auto pair = reply.pairs(i);
      for (int j = 0; j < pair.slaves_size(); j++) {
        ring[i].second.push_back(Slave{ .addr = pair.slaves(j).addr(), .connections = pair.slaves(j).connections() });
        // std::cout << pair.slaves(j).addr() << std::endl;
      }
      // std::cout << std::endl;
      // std::cout << "Get message: " << reply.message() << std::endl;
      
    }
    return Table(stub_, ring, table_name);
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    throw "get_table error";
  }
  return Table(stub_, ring, table_name);
  // return tables[table_name];
}

void Connection::delete_table(std::string table_name) {
  tables.erase(table_name);
}
