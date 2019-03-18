#include "putcalldata.h"
#include <cstdlib>

PutCallData::PutCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, Logger &logger) : OperationCallData(service, cq, tables), responder_(&ctx_), logger_(logger) {
  Proceed();
}

void PutCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestPut(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new PutCallData(service_, cq_, tables_, logger_);

    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    {
      // std::lock_guard<std::mutex> lock(row_monitor.mutex);
      
      for (int i = 0; i < request_.col_families_size(); i++) {
        const kvs::ColFamily &col_family = request_.col_families(i);
        for (int j = 0; j < col_family.cols_size(); j++) {
          const kvs::Col &col = col_family.cols(j);
          std::pair<byte*, int> &p = row_monitor.row[col_family.col_family()][col.col()];
          if (p.first != NULL) {
            free(p.first);
            p.second = 0;
          }
          assert(!col.content().empty() && col.content().length() != 0);
          p.first = (byte*)malloc(sizeof(byte) * col.content().length());
          memcpy(p.first, col.content().c_str(), col.content().length());
          p.second = col.content().length();

          logger_.log_put(request_);
        }
      }
      // std::pair<byte*, int> &p = row_monitor.row[request_.col_family()][request_.col()];
      // if (p.first != NULL) {
      //   free(p.first);
      //   p.second = 0;
      // }
      // assert(!request_.content().empty() && request_.content().length() != 0);
      // p.first = (byte*)malloc(sizeof(byte) * request_.content().length());
      // memcpy(p.first, request_.content().c_str(), request_.content().length());
      // p.second = request_.content().length();
    }

    reply_.set_message("PutCallData OK");

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}