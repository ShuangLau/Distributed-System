#ifndef _CONNCALLDATA_H
#define _CONNCALLDATA_H

#include "calldata.h"

using kvs::ConnRequest;

class ConnCallData final : public CallData {
public:
  ConnCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring);
  void Proceed();
private:
  ConnRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;

  RingType &ring_;
};

#endif