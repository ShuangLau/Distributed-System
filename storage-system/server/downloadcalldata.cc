#include "downloadcalldata.h"
#include <fstream>

DownloadCallData::DownloadCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, std::string slave_address) : CallData(service, cq), responder_(&ctx_), slave_address_(slave_address) {
  Proceed();
}

void DownloadCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;
    service_->RequestDownloadCheckPoint(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new DownloadCallData(service_, cq_, slave_address_);

    std::fstream fin0("checkpoint-" + slave_address_, std::ios::in | std::ios::binary);
    Data *data = new Data();
    if (!data->ParseFromIstream(&fin0)) std::cerr << "DownloadCallData: parse data proto failed" << std::endl;
    fin0.close();

    std::fstream fin1("log-" + slave_address_, std::ios::in | std::ios::binary);
    Log *log = new Log();
    if (!log->ParseFromIstream(&fin1)) std::cerr << "LogCallData: parse log proto failed" << std::endl;
    fin1.close();

    reply_.set_allocated_data(data);
    reply_.set_allocated_log(log);

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}