#include "greetcalldata.h"

void GreetCallData::ShowMeTheRing() {
  std::cout << "Ring: " << std::endl;
  for (auto &p : ring_) {
    std::cout << p.first << ": ";
    for (auto &s : p.second) {
      std::cout << s.addr << " ";
    }
    std::cout << std::endl;
  }
}

GreetCallData::GreetCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, RingType &ring) : CallData(service, cq), responder_(&ctx_), ring_(ring) {
  Proceed();
}

void GreetCallData::Proceed() {
  if (status_ == CREATE) {
    // Make this instance progress to the PROCESS state.
    status_ = PROCESS;

    // As part of the initial CREATE state, we *request* that the system
    // start processing SayHello requests. In this request, "this" acts are
    // the tag uniquely identifying the request (so that different CallData
    // instances can serve different requests concurrently), in this case
    // the memory address of this CallData instance.
    service_->RequestGreet(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state.
    new GreetCallData(service_, cq_, ring_);

    // std::cout << "new hash: " << new_hash << std::endl;
    // ring_[new_hash] = request_.slave_address();
    for (auto &p : ring_) {
      for (auto &s : p.second) {
        if (request_.replicate_slave_address() == s.addr) {
          auto iter = find_if(p.second.begin(), p.second.end(), [this](Slave &s) {
            return s.addr == request_.slave_address();
          });
          if (iter == p.second.end()) {
            p.second.push_back(Slave{ .addr = request_.slave_address(), .connections = 0 });
            reply_.set_is_new(true);
          } else {
            reply_.set_is_new(false);
          }
          goto exit;
        }
      }
    }
    reply_.set_is_new(true);
    ring_[GetNewHash()].push_back(Slave{ .addr = request_.slave_address(), .connections = 0 });
  exit:
    // The actual processing.
    // std::string prefix("row key: ");
    ShowMeTheRing();
    // reply_.set_message(prefix + request_.slave_address() + ", " + request_.replicate_slave_address());

    // And we are done! Let the gRPC runtime know we've finished, using the
    // memory address of this instance as the uniquely identifying tag for
    // the event.
    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    // Once in the FINISH state, deallocate ourselves (CallData).
    delete this;
  }
}

size_t GreetCallData::GetNewHash() {
  std::map<size_t, std::list<Slave>>::iterator pre = ring_.end();
  size_t max_pre = 0;
  size_t max = 0;
  for (auto iter = ring_.begin(); iter != ring_.end(); iter++) {
    if (pre != ring_.end() && iter->first - pre->first > max) {
      max = iter->first - pre->first;
      max_pre = pre->first;
    }
    pre = iter;
  }
  if (!ring_.empty()) {
    if (std::numeric_limits<std::size_t>::max() - ring_.rbegin()->first + ring_.begin()->first > max) {
      max = std::numeric_limits<std::size_t>::max() - ring_.rbegin()->first + ring_.begin()->first;
      max_pre = ring_.rbegin()->first;
    }
    return (max >> 1) + max_pre;
  }

  return std::numeric_limits<std::size_t>::max() >> 1;
}