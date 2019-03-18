#ifndef _CALLDATA_H
#define _CALLDATA_H

#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <set>
#include <map>
#include <cassert>
#include <mutex>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>

#include "kvs.grpc.pb.h"
#include "../common.h"

using grpc::Server;
using grpc::ServerAsyncResponseWriter;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerCompletionQueue;
using grpc::Status;
using kvs::Pair;
using kvs::TableRequest;
using kvs::TableReply;
using kvs::Kvs;
using kvs::EmptyReply;
using kvs::EmptyRequest;
using kvs::AliveRequest;
using kvs::AliveReply;
using kvs::ShutdownRequest;
using grpc::ClientContext;
using kvs::GreetRequest;
using kvs::GreetReply;
using kvs::Table;
using kvs::Row;
using kvs::Data;
using kvs::DataAndLog;
using kvs::Log;
using kvs::Type;
using kvs::Operation;

struct RowMonitor {
   RowType row;
   std::mutex mutex;
};

typedef std::unordered_map<std::string, std::unordered_map<std::string, RowMonitor>> TableType;
typedef std::map<size_t, std::list<Slave>> RingType;

// Class encompasing the state and logic needed to serve a request.
class CallData {
public:
  // Take in the "service" instance (in this case representing an asynchronous
  // server) and the completion queue "cq" used for asynchronous communication
  // with the gRPC runtime.
  CallData(Kvs::AsyncService* service, ServerCompletionQueue* cq);

  virtual void Proceed() = 0;

protected:
  // The means of communication with the gRPC runtime for an asynchronous
  // server.
  Kvs::AsyncService* service_;
  // The producer-consumer queue where for asynchronous server notifications.
  ServerCompletionQueue* cq_;
  // Context for the rpc, allowing to tweak aspects of it such as the use
  // of compression, authentication, as well as to send metadata back to the
  // client.
  ServerContext ctx_;

  // Let's implement a tiny state machine with the following states.
  enum CallStatus { CREATE, PROCESS, FINISH };
  CallStatus status_;  // The current serving state.
};

#endif