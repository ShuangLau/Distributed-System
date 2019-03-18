#include "cputcheckcalldata.h"

CputCheckCallData::CputCheckCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables) : OperationCallData(service, cq, tables), responder_(&ctx_) {
  Proceed();
}

void CputCheckCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestCputCheck(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    // std::cout << "CputCheck" << std::endl;
    new CputCheckCallData(service_, cq_, tables_);

    RowMonitor &row_monitor = tables_[request_.table_name()][request_.row_key()];
    {
      assert(!row_monitor.mutex.try_lock());

      std::pair<byte*, int> &pair = row_monitor.row[request_.col_family()][request_.col()];
      // reply_.set_is_same(!memcmp(pair.first, request_.content().c_str(), pair.second));
      if (pair.first != nullptr && pair.second != 0) {
        // std::cout << "pair.first: " << std::string((char*)pair.first, pair.second) << std::endl;
        // std::cout << "content: " << request_.content() << std::endl;
        reply_.set_is_same(!memcmp(pair.first, request_.content().c_str(), pair.second));
      } else {
        // std::cout << "pair.first empty" << std::endl;
        reply_.set_is_same(false);
      }
      // if (!memcmp(pair.first, request_.content().c_str(), pair.second)) {
        // for (int i = 0; i < put_request.col_families_size(); i++) {
        //   const kvs::ColFamily &col_family = put_request.col_families(i);
        //   for (int j = 0; j < col_family.cols_size(); j++) {
        //     const kvs::Col &col = col_family.cols(j);
        //     std::pair<byte*, int> &p = row_monitor.row[col_family.col_family()][col.col()];
        //     if (p.first != NULL) {
        //       free(p.first);
        //       p.second = 0;
        //     }
        //     assert(!col.content().empty() && col.content().length() != 0);
        //     p.first = (byte*)malloc(sizeof(byte) * col.content().length());
        //     memcpy(p.first, col.content().c_str(), col.content().length());
        //     p.second = col.content().length();
        //   }
        // }
        // reply_.set_is_same(true);
      // } else {
        // reply_.set_is_same(false);
      // }
    }

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}