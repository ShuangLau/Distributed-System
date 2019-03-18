#ifndef _DELETECALLDATA_H
#define _DELETECALLDATA_H

#include "operationcalldata.h"
#include "logger.h"

using kvs::DeleteRequest;
using kvs::DeleteReply;

class DeleteCallData final : public OperationCallData {
public:
  DeleteCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, Logger &logger);
  void Proceed();
private:
  DeleteRequest request_;
  DeleteReply reply_;
  ServerAsyncResponseWriter<DeleteReply> responder_;

  Logger &logger_;
};

#endif