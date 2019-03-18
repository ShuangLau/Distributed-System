#ifndef _OPERATIONCALLDATA_H
#define _OPERATIONCALLDATA_H

#include "calldata.h"

class OperationCallData : public CallData {
public:
  OperationCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables);
protected:
  TableType &tables_;
};

#endif