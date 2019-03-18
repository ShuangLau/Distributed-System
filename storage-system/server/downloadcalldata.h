#ifndef _DOWNLOADCALLDATA_H
#define _DOWNLOADCALLDATA_H

#include "calldata.h"

class DownloadCallData final : public CallData {
public:
  DownloadCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, std::string slave_address);
  void Proceed();
private:
  EmptyRequest request_;
  DataAndLog reply_;
  ServerAsyncResponseWriter<DataAndLog> responder_;

  std::string slave_address_;
};

#endif