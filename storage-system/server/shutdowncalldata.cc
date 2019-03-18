#include "shutdowncalldata.h"
#include <cstdlib>

ShutdownCallData::ShutdownCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, bool &is_terminated) : OperationCallData(service, cq, tables), responder_(&ctx_), is_terminated_(is_terminated) {
  Proceed();
}

void ShutdownCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;
    service_->RequestShutdown(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new ShutdownCallData(service_, cq_, tables_, is_terminated_);

    for (auto &p0 : tables_) {
      for (auto &p1 : p0.second) {
        for (auto &p2 : p1.second.row) {
          for (auto &p3 : p2.second) {
            if (p3.second.first) free(p3.second.first);
          }
        }
      }
    }
    
    tables_.clear();
    is_terminated_ = request_.is_terminated();
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    
    log_print("Here");
    delete this;
  }
}