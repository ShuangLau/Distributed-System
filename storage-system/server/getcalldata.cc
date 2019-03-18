#include "getcalldata.h"

GetCallData::GetCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables) : OperationCallData(service, cq, tables), responder_(&ctx_) {
  Proceed();
}

void GetCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestGet(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new GetCallData(service_, cq_, tables_);

    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    {
      // std::lock_guard<std::mutex> lock(row_monitor.mutex);
      // reply_.set_row_key(request_.row_key());
      for (auto &p0 : row_monitor.row) {
        kvs::ColFamily *col_families = reply_.add_col_families();
        col_families->set_col_family(p0.first);
        for (auto &p1 : p0.second) {
          if (p1.second.first == nullptr) {
            assert(p1.second.second == 0);
            continue;
          }
          kvs::Col *col = col_families->add_cols();
          col->set_col(p1.first);
          // assert(p1.second.first != nullptr && p1.second.second != 0);
          col->set_content(std::string((char*)(p1.second.first), p1.second.second));
          // std::cout << request_.row_key() << ": " << std::string((char*)(p1.second.first), p1.second.second) << std::endl;
        }
      }
    }
    reply_.set_message("GetCallData OK");

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}