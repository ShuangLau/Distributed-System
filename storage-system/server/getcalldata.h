#ifndef _GETCALLDATA_H
#define _GETCALLDATA_H

#include "operationcalldata.h"

using kvs::GetRequest;
using kvs::GetReply;

class GetCallData final : public OperationCallData {
public:
  GetCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables);
  void Proceed();
private:
  GetRequest request_;
  GetReply reply_;
  ServerAsyncResponseWriter<GetReply> responder_;
};

#endif