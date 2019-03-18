#include <cassert>
#include "getcalldata.h"
#include "putcalldata.h"
#include "deletecalldata.h"
#include "cputcheckcalldata.h"
#include "cputputcalldata.h"
#include "precputcalldata.h"

#include <iostream>
#include <thread>
#include <chrono>

#include "logger.h"
#include "shutdowncalldata.h"
#include "emptycalldata.h"
#include "downloadcalldata.h"
#include "checkpointcalldata.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using kvs::GreetRequest;

class SlaveClient {
public:
  SlaveClient(std::shared_ptr<Channel> channel)
      : stub_(Kvs::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  bool Greet(const std::string& slave_address, const std::string& replicate_slave_address) {
    // Data we are sending to the server.
    GreetRequest request;
    request.set_slave_address(slave_address);
    request.set_replicate_slave_address(replicate_slave_address);

    // Container for the data we expect from the server.
    GreetReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->Greet(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      // for (int i = 0; i < reply.pairs_size(); i++) {
      //   std::cout << "key " << reply.pairs(i).key() << ": " << reply.pairs(i).value() << std::endl;
      // }
      return reply.is_new();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

private:
  std::unique_ptr<Kvs::Stub> stub_;
};

class ServerImpl final {
public:
  ServerImpl(std::string slave_address) : slave_address_(slave_address), logger_(slave_address) {}

  ~ServerImpl() {
    if (!is_terminated_) {
      server_->Shutdown();
      cq_->Shutdown();
    }
  }

  // There is no shutdown handling in this code.
  void Run() {
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(slave_address_, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&service_);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    cq_ = builder.AddCompletionQueue();
    // Finally assemble the server.
    server_ = builder.BuildAndStart();
    std::cout << "Server listening on " << slave_address_ << std::endl;

    // Proceed to the server's main loop.
    // std::thread thread(&ServerImpl::Dump, this);
    HandleRpcs();
  }
  TableType tables_;
  bool is_terminated_ = true;
private:
  // This can be run in multiple threads if needed.
  void HandleRpcs() {
    // Spawn a new CallData instance to serve new clients.
    new GetCallData(&service_, cq_.get(), tables_);
    new PutCallData(&service_, cq_.get(), tables_, logger_);
    new DeleteCallData(&service_, cq_.get(), tables_, logger_);
    new CputCheckCallData(&service_, cq_.get(), tables_);
    new CputPutCallData(&service_, cq_.get(), tables_);
    new PreCputCallData(&service_, cq_.get(), tables_);

    // for fault tolerance and recovery
    new CheckPointCallData(&service_, cq_.get(), tables_, slave_address_, logger_);
    new ShutdownCallData(&service_, cq_.get(), tables_, is_terminated_);
    new EmptyCallData(&service_, cq_.get());
    new DownloadCallData(&service_, cq_.get(), slave_address_);

    void* tag;  // uniquely identifies a request.
    bool ok;

  // wait for loading the checkpoint. The incoming messages are queued.
    while (is_terminated_) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      log_print("Wating for restarting");
    }

    while (!is_terminated_) {
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

  std::unique_ptr<ServerCompletionQueue> cq_;
  Kvs::AsyncService service_;
  std::unique_ptr<Server> server_;

  int version_ = 0;
  std::string slave_address_;
  Logger logger_;
};

ServerImpl *server = nullptr;

void recover(std::string master_address, std::string slave_address) {
  std::string alive_address;
  {
    std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials())));
    AliveRequest request;
    request.set_src_addr(slave_address);
    AliveReply reply;
    ClientContext context;
    Status status = stub->Alive(&context, request, &reply);
    if (status.ok()) {
      alive_address = reply.alive_addr();
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    }
  }
  log_print("alive address: " + alive_address);
  assert(!alive_address.empty());
  {
    std::unique_ptr<Kvs::Stub> stub(Kvs::NewStub(grpc::CreateChannel(alive_address, grpc::InsecureChannelCredentials())));
    EmptyRequest request;
    DataAndLog reply;
    ClientContext context;
    Status status = stub->DownloadCheckPoint(&context, request, &reply);
    log_print("Recovering starts");
    if (status.ok()) {
      const Data &data = reply.data();

      for (int i = 0; i < data.tables_size(); i++) {
        const Table &table = data.tables(i);
        for (int j = 0; j < table.rows_size(); j++) {
          const Row &row = table.rows(j);
          for (int k = 0; k < row.col_families_size(); k++) {
            const ColFamily &col_family = row.col_families(k);
            for (int l = 0; l < col_family.cols_size(); l++) {
              const Col &col = col_family.cols(l);

              std::pair<byte*, int> &p = server->tables_[table.table_name()][row.row_key()].row[col_family.col_family()][col.col()];
              if (p.first != NULL) {
                free(p.first);
                p.second = 0;
              }
              assert(!col.content().empty() && col.content().length() != 0);
              p.first = (byte*)malloc(sizeof(byte) * col.content().length());
              memcpy(p.first, col.content().c_str(), col.content().length());
              p.second = col.content().length();
              std::cout << table.table_name() << "\t" << row.row_key() << "\t" << col_family.col_family() << "\t" << col.col() << "\t" << col.content() << std::endl;
              log_print(std::string((char*)p.first, p.second));
            }
          }
        }
      }
      log_print("Recovering ends");

      log_print("Streaming log starts");
      const Log &log = reply.log();

      for (int i = 0; i < log.operations_size(); i++) {
        const Operation &operation = log.operations(i);

        if (operation.type() == Type::PUT) {

          for (int j = 0; j < operation.col_families_size(); j++) {
            const ColFamily &col_family = operation.col_families(j);
            for (int k = 0; k < col_family.cols_size(); k++) {
              const Col &col = col_family.cols(k);
              std::pair<byte*, int> &p = server->tables_[operation.table_name()][operation.row_key()].row[col_family.col_family()][col.col()];
              if (p.first != NULL) {
                free(p.first);
                p.second = 0;
              }
              assert(!col.content().empty() && col.content().length() != 0);
              p.first = (byte*)malloc(sizeof(byte) * col.content().length());
              memcpy(p.first, col.content().c_str(), col.content().length());
              p.second = col.content().length();
              std::cout << "[PUT]\t" << operation.table_name() << "\t" << operation.row_key() << "\t" << col_family.col_family() << "\t" << col.col() << "\t" << col.content() << std::endl;
            }
          }

        } else if (operation.type() == Type::DELETE) {

          RowMonitor &row_monitor = server->tables_[operation.table_name()][operation.row_key()];
          {
            for (int j = 0; j < operation.col_families_size(); j++) {
              const kvs::ColFamily &col_family = operation.col_families(j);
              for (int k = 0; k < col_family.cols_size(); k++) {
                const kvs::Col &col = col_family.cols(k);

                auto iter0 = row_monitor.row.find(col_family.col_family());
                if (iter0 != row_monitor.row.end()) {
                  auto iter1 = iter0->second.find(col.col());
                  if (iter1 != iter0->second.end()) {
                    if (iter1->second.first) {
                      free(iter1->second.first);
                      iter1->second.second = 0;
                    }
                    std::cout << "[DEL]\t" << operation.table_name() << "\t" << operation.row_key() << "\t" << col_family.col_family() << "\t" << col.col() << "\t" << std::endl;
                    iter0->second.erase(iter1);
                    if (iter0->second.empty()) row_monitor.row.erase(iter0); // remove column family if it's empty
                  }
                }
              }
            }

          }
        } else {
          std::cerr << "Can not recognize the type of operations" << std::endl;
        }
      }
      log_print("Streaming log ends");
    } else {
      std::cout << status.error_code() << ": " << status.error_message() << std::endl;
    }
  }

  server->is_terminated_ = false;
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: ./slave master_address slave_address replicate_slave_address\n");
    return -1;
  }
  std::string master_address(argv[1]);
  std::string slave_address(argv[2]);
  std::string replicate_slave_address(argv[3]);

  server = new ServerImpl(slave_address);
  
  // while (1) {
    SlaveClient client(grpc::CreateChannel(master_address, grpc::InsecureChannelCredentials()));
    bool is_new = client.Greet(slave_address, replicate_slave_address);

    if (!is_new) {
      log_print("Recovering");
      std::thread thread(recover, master_address, slave_address);
      thread.detach();
    } else {
      server->is_terminated_ = false;
    }

    server->Run();
  // }
  
  free(server);
  return 0;
}
