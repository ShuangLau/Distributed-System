#ifndef _CPUTPUTCALLDATA_H
#define _CPUTPUTCALLDATA_H

#include "operationcalldata.h"

using kvs::PutRequest;

class CputPutCallData final : public OperationCallData {
public:
  CputPutCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables);
  void Proceed();
private:
  PutRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;
};

#endif