#ifndef _ALIVECALLDATA_H
#define _ALIVECALLDATA_H

#include "calldata.h"

class AliveCallData final : public CallData {
public:
  AliveCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring);
  void Proceed();
private:
  AliveRequest request_;
  AliveReply reply_;
  ServerAsyncResponseWriter<AliveReply> responder_;

  RingType &ring_;
};

#endif