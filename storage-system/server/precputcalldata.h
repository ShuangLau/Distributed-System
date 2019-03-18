#ifndef _PRECPUTCALLDATA_H
#define _PRECPUTCALLDATA_H

#include "operationcalldata.h"

using kvs::PreCputRequest;
using kvs::PreCputReply;

class PreCputCallData final : public OperationCallData {
public:
  PreCputCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &table);
  void Proceed();
private:
  PreCputRequest request_;
  PreCputReply reply_;
  ServerAsyncResponseWriter<PreCputReply> responder_;
};

#endif