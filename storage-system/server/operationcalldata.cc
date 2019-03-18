#include "operationcalldata.h"

OperationCallData::OperationCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &table) : CallData(service, cq), tables_(table) {

}