#include "calldata.h"

CallData::CallData(Kvs::AsyncService* service, ServerCompletionQueue* cq): service_(service), cq_(cq), status_(CREATE) {
  // Invoke the serving logic right away.
}