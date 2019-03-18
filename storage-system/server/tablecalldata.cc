#include "tablecalldata.h"

TableCallData::TableCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, const RingType &ring) : CallData(service, cq), responder_(&ctx_), ring_(ring) {
  Proceed();
}

void TableCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;
    service_->RequestAskTable(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new TableCallData(service_, cq_, ring_);

    // The actual processing.
    std::string prefix("Hello ");

    for (auto &p : ring_) {
      Pair *pair = reply_.add_pairs();
      pair->set_key(p.first);

      for (auto &s : p.second) {
        kvs::Slave *slave = pair->add_slaves();
        slave->set_addr(s.addr);
        slave->set_connections(s.connections);

        std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
        EmptyRequest request;
        EmptyReply reply;
        ClientContext context;
        Status status = stub->Empty(&context, request, &reply);

        // Act upon its status.
        if (status.ok()) {
          slave->set_is_alive(true);
        } else {
          slave->set_is_alive(false);
        }


        // std::cout << s.addr << std::endl;
      }
    }
    // Pair *pair = reply_.add_pairs();
    // pair->set_key("key0");
    // pair->set_value("value0");
    // pair = reply_.add_pairs();
    // pair->set_key("key1");
    // pair->set_value("value1");
    // std::cout << "matser" << std::endl;

    reply_.set_message("TableCallData OK");

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}