#ifndef _TABLECALLDATA_H
#define _TABLECALLDATA_H

#include "calldata.h"

class TableCallData final : public CallData {
public:
  TableCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, const std::map<size_t, std::list<Slave>> &ring_);
  void Proceed();
private:
  // What we get from the client.
  TableRequest request_;
  // What we send back to the client.
  TableReply reply_;

  // The means to get back to the client.
  ServerAsyncResponseWriter<TableReply> responder_;

  const RingType &ring_;
};

#endif