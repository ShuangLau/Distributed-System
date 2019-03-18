#ifndef _CPUTCHECKCALLDATA_H
#define _CPUTCHECKCALLDATA_H

#include "operationcalldata.h"

using kvs::CputCheckRequest;
using kvs::CputCheckReply;

class CputCheckCallData final : public OperationCallData {
public:
  CputCheckCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables);
  void Proceed();
private:
  CputCheckRequest request_;
  CputCheckReply reply_;

  ServerAsyncResponseWriter<CputCheckReply> responder_;
};

#endif