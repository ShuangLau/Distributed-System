#ifndef _PUTCALLDATA_H
#define _PUTCALLDATA_H

#include "operationcalldata.h"
#include "logger.h"

using kvs::PutRequest;

class PutCallData final : public OperationCallData {
public:
  PutCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, Logger &logger);
  void Proceed();
private:
  PutRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;

  Logger &logger_;
};

#endif