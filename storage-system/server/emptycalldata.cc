#include "emptycalldata.h"

EmptyCallData::EmptyCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq) : CallData(service, cq), responder_(&ctx_) {
  Proceed();
}

void EmptyCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;
    service_->RequestEmpty(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new EmptyCallData(service_, cq_);
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}