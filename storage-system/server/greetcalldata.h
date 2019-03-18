#ifndef _GREETCALLDATA_H
#define _GREETCALLDATA_H

#include "calldata.h"

using kvs::GreetRequest;
using kvs::GreetReply;

class GreetCallData final : public CallData {
public:
  GreetCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring);
  void Proceed();
private:
  // What we get from the client.
  GreetRequest request_;
  // What we send back to the client.
  GreetReply reply_;

  // The means to get back to the client.
  ServerAsyncResponseWriter<GreetReply> responder_;

  RingType &ring_;

  size_t GetNewHash();
  void ShowMeTheRing();
};

#endif