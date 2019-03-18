#ifndef _EMPTYCALLDATA_H
#define _EMPTYCALLDATA_H

#include "calldata.h"

class EmptyCallData final : public CallData {
public:
  EmptyCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq);
  void Proceed();
private:
  EmptyRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;
};

#endif