#include "conncalldata.h"

ConnCallData::ConnCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring) : CallData(service, cq), responder_(&ctx_), ring_(ring) {
  Proceed();
}

void ConnCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestUpdateConnection(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new ConnCallData(service_, cq_, ring_);

    for (auto &p : ring_) {
      for (auto &s : p.second) {
        if (s.addr == request_.addr()) {
          s.connections += request_.delta();
          // std::cout << "connection: " << s.connections << std::endl;
          goto exit;
        }
      }
    }
    throw "slave not found";
  exit:
    reply_.set_message("ConnCallData OK");

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}