#include "precputcalldata.h"

PreCputCallData::PreCputCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables) : OperationCallData(service, cq, tables), responder_(&ctx_) {
  Proceed();
}

void PreCputCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestPreCput(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new PreCputCallData(service_, cq_, tables_);
    // RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    // if (request_.acquire_or_abort()) {
    //     if (row_monitor.mutex.try_lock()) {
    //       reply_.set_is_locked(true);
    //       reply_.set_message("Hello");
    //     }
    //   }
    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    if (request_.acquire_or_abort()) {
      // printf("here0\n");
      if (row_monitor.mutex.try_lock()) {
        // printf("LOCK\n");
        reply_.set_is_locked(true);
        reply_.set_message("Hello");
        // std::cout << reply_.is_locked() << std::endl;
      } else {
        reply_.set_is_locked(false);
      }
      // printf("here1\n");
    } else {
      // printf("UNLOCK\n");
      assert(!row_monitor.mutex.try_lock());
      row_monitor.mutex.unlock();
    }
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}