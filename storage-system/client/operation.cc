#include "operation.h"

Operation::Operation(std::string row_key) : row_key(std::move(row_key)) {}

const std::string & Operation::get_row_key() const {
  return row_key;
}

void Operation::update_connections(std::unique_ptr<kvs::Kvs::Stub> &master_stub, const std::string &slave_addr, int delta) {
  kvs::ConnRequest request;
  request.set_addr(slave_addr);
  request.set_delta(delta);

  kvs::EmptyReply reply;
  grpc::ClientContext context;

  grpc::Status status = master_stub->UpdateConnection(&context, request, &reply);

  // Act upon its status.
  if (status.ok()) {
    // std::cout << "Get message: " << reply.message() << std::endl;
  } else {
    std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    throw "get error";
  }
}