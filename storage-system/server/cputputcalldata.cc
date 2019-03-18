#include "cputputcalldata.h"
#include <cstdlib>

CputPutCallData::CputPutCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables) : OperationCallData(service, cq, tables), responder_(&ctx_) {
  Proceed();
}

void CputPutCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestCputPut(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new CputPutCallData(service_, cq_, tables_);
    // std::cout << "CputPut" << std::endl;
    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    {
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
        }
      }
    }

    reply_.set_message("PutCallData OK");

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}