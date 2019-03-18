#include "deletecalldata.h"

DeleteCallData::DeleteCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, Logger &logger) : OperationCallData(service, cq, tables), responder_(&ctx_), logger_(logger) {
  Proceed();
}

void DeleteCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestDelete(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new DeleteCallData(service_, cq_, tables_, logger_);
    reply_.set_is_succeeded(false);
    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    {
      // std::lock_guard<std::mutex> lock(row_monitor.mutex);
      // reply_.set_row_key(request_.row_key());
      for (int i = 0; i < request_.col_families_size(); i++) {
        const kvs::ColFamily &col_family = request_.col_families(i);
        for (int j = 0; j < col_family.cols_size(); j++) {
          const kvs::Col &col = col_family.cols(j);

          auto iter0 = row_monitor.row.find(col_family.col_family());
          if (iter0 != row_monitor.row.end()) {
            auto iter1 = iter0->second.find(col.col());
            if (iter1 != iter0->second.end()) {
              if (iter1->second.first) {
                free(iter1->second.first);
                iter1->second.second = 0;
              }
              iter0->second.erase(iter1);
              reply_.set_is_succeeded(true);
              if (iter0->second.empty()) row_monitor.row.erase(iter0); // remove column family if it's empty

              logger_.log_delete(request_);
            }
          }
        }
      }
      // don't do this. Don't remove the mutex.
      // if (row_monitor.row.empty()) tables_[request_.table_name()].erase(request_.row_key());
    }

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}