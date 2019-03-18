#ifndef _CHECKPOINTCALLDATA_H
#define _CHECKPOINTCALLDATA_H

#include "operationcalldata.h"
#include "logger.h"
#include <fstream>

using kvs::CheckPointRequest;
using kvs::Col;
using kvs::ColFamily;

class CheckPointCallData final : public OperationCallData {
public:
  CheckPointCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, std::string slave_address, Logger &logger);
  void Proceed();
private:

  CheckPointRequest request_;
  EmptyReply reply_;
  ServerAsyncResponseWriter<EmptyReply> responder_;
  std::string slave_address_;

  Logger &logger_;
};

#endif