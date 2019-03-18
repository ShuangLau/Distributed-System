#include <cassert>
#include <vector>
#include <set>
#include <limits>
#include <algorithm>
#include "tablecalldata.h"
#include "greetcalldata.h"
#include "conncalldata.h"
#include "alivecalldata.h"

#include <thread>
#include <chrono>

using grpc::ClientContext;
using kvs::CheckPointRequest;

class ServerImpl final {
public:
  ~ServerImpl() {
    server_->Shutdown();
    // Always shutdown the completion queue after the server.
    cq_->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");

    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    std::thread thread(&ServerImpl::CheckPoint, this);
    HandleRpcs();
    
    thread.join();
  }

private:
  // This can be run in multiple threads if needed.
  void HandleRpcs() {
    // Spawn a new CallData instance to serve new clients.
    new TableCallData(&service_, cq_.get(), ring_);
    new GreetCallData(&service_, cq_.get(), ring_);
    new ConnCallData(&service_, cq_.get(), ring_);
    new AliveCallData(&service_, cq_.get(), ring_);

    void* tag;  // uniquely identifies a request.
    bool ok;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(cq_->Next(&tag, &ok));
      GPR_ASSERT(ok);
      static_cast<CallData*>(tag)->Proceed();
    }
  }

  void CheckPoint() {
    while (!is_terminated_) {
      std::this_thread::sleep_for(std::chrono::seconds(15));
      for (auto &p0 : ring_) {
        for (auto &s : p0.second) {
          std::unique_ptr<Kvs::Stub> stub = kvs::Kvs::NewStub(grpc::CreateChannel(s.addr, grpc::InsecureChannelCredentials()));

          CheckPointRequest request;
          request.set_version(version_);
          EmptyReply reply;
          ClientContext context;
          Status status = stub->CheckPoint(&context, request, &reply);

          if (status.ok()) {
            std::cout << "check point response OK" << std::endl;
          } else {
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
          }
        }
      }
      version_++;
    }
  }

  std::unique_ptr<ServerCompletionQueue> cq_;
  Kvs::AsyncService service_;
  std::unique_ptr<Server> server_;

  RingType ring_;
  bool is_terminated_ = false;
  int version_ = 0;
};

int main(int argc, char** argv) {
  // std::cout << std::numeric_limits<std::size_t>::max() << " " << (std::numeric_limits<std::size_t>::max() >> 1) << std::endl;
  ServerImpl server;
  server.Run();

  return 0;
}
