#ifndef _SHUTDOWNCALLDATA_H
#define _SHUTDOWNCALLDATA_H

#include "operationcalldata.h"

using kvs::ShutdownRequest;

class ShutdownCallData : public OperationCallData {
public:
  ShutdownCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, bool &is_terminated);
  void Proceed();
protected:
  // RingType &ring_;
  bool &is_terminated_;

  ShutdownRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;
};

#endif