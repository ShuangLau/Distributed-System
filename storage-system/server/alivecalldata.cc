#include "alivecalldata.h"

AliveCallData::AliveCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring) : CallData(service, cq), ring_(ring), responder_(&ctx_) {
  Proceed();
}

void AliveCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;
    service_->RequestAlive(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new AliveCallData(service_, cq_, ring_);

    for (auto &p : ring_) {
      // auto iter = find(p.second.begin(), p.second.end(), request_.src_addr());
      auto iter = find_if(p.second.begin(), p.second.end(), [this](Slave &s) {
        return s.addr == request_.src_addr();
      });
      if (iter != p.second.end()) {
        for (auto &s : p.second) {
          if (s.addr != request_.src_addr()) {
            std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials())));
            EmptyRequest request;
            EmptyReply reply;
            ClientContext context;
            Status status = stub->Empty(&context, request, &reply);

            // Act upon its status.
            if (status.ok()) {
              reply_.set_alive_addr(s.addr);
              goto exit;
            } else {
              std::cout << status.error_code() << ": " << status.error_message() << std::endl;
            }
          }
        }
      }
    }
  exit:
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}