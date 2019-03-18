HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX=g++
CXXFLAGS += -std=c++11 -pthread
OBJ=column-family-descriptor.o configuration.o connection.o delete.o get.o operation.o put.o result.o table-descriptor.o table.o
#OBJ_DIR=storge_client
KVS_CLIENT_PATH=../storage-system/client
STORAGE_SERVICE_PATH=../storage-service
KVS_ADMIN_PATH=../kvs-admin

TARGET=sample sample2 sample3

ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++ grpc`\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= /usr/local/bin/grpc_cpp_plugin


FILTER = $(KVS_CLIENT_PATH)/sample.cc $(KVS_CLIENT_PATH)/sample2.cc $(KVS_CLIENT_PATH)/sample3.cc
SRC=$(filter-out $(FILTER), $(wildcard $(KVS_CLIENT_PATH)/*.cc))
SRC+=$(STORAGE_SERVICE_PATH)/storage-service-user.cc
SRC+=$(KVS_ADMIN_PATH)/kvsadmin.cc

NEW_OBJ=$(patsubst %.cc, %.o, $(SRC))

#$(OBJ): $(SRC)
	#$(CXX) $(CXXFLAGS) -c $^    

# $(KVS_CLIENT_PATH)/%.o: $(KVS_CLIENT_PATH)/%.cc
# 	$(CXX) $(CXXFLAGS) -g -c $< -o $@

# $(STORAGE_SERVICE_PATH)/%.o: $(STORAGE_SERVICE_PATH)/%.cc
# 	$(CXX) $(CXXFLAGS) -g -c $< -o $@

Servant: Servant.o Socket.o EventLoopThreadPool.o EventLoopThread.o Thread.o EventLoop.o Parser.o Handler.o Epoll.o Buffer.o smtp.o DNSLookup.o database.o $(NEW_OBJ)
	$(CXX) $^ $(LDFLAGS) -g -o $@
	#g++ $^ -std=c++11 -pthread $(LDFLAGS) -o Servant Servant.o Socket.o EventLoopThreadPool.o EventLoopThread.o Thread.o EventLoop.o Parser.o Handler.o Epoll.o Buffer.o database.o smtp.o DNSLookup.o

Servant.o: Servant.cpp Socket.h EventLoopThreadPool.h EventLoopThread.h EventLoop.h smtp.h 
	g++ -std=c++11 -g -c -pthread Servant.cpp EventLoopThreadPool.cpp EventLoopThread.cpp EventLoop.cpp smtp.cpp

DNSLookup.o: DNStest/DNSLookup.cpp DNStest/DNSLookup.h
	g++ -std=c++11 -g -c -pthread DNStest/DNSLookup.cpp

Socket.o: Socket.cpp Socket.h 
	g++ -std=c++11 -g -c -pthread Socket.cpp

smtp.o: smtp.cpp smtp.cpp database.h DNStest/DNSLookup.h
	g++ -std=c++11 -g -c -pthread smtp.cpp database.cpp

EventLoopThreadPool.o: EventLoopThreadPool.cpp EventLoopThread.h 
	g++ -std=c++11 -g -c -pthread EventLoopThreadPool.cpp EventLoopThread.cpp

EventLoopThread.o: EventLoopThread.cpp EventLoopThread.h EventLoop.h Thread.h 
	g++ -std=c++11 -g -c -pthread EventLoopThread.cpp EventLoop.cpp Thread.cpp

Thread.o: Thread.cpp Thread.h 
	g++ -std=c++11 -g -c -pthread Thread.cpp

EventLoop.o: EventLoop.cpp EventLoop.h Epoll.h 
	g++ -std=c++11 -g -c EventLoop.cpp Epoll.cpp

Parser.o: Parser.cpp Parser.h 
	g++ -std=c++11 -g -c Parser.cpp


Handler.o: Handler.cpp Handler.h Parser.h database.h
	g++ -std=c++11 -g -c Handler.cpp Parser.cpp database.cpp

Epoll.o: Handler.cpp Handler.h Parser.h Epoll.h 
	g++ -std=c++11 -g -c Epoll.cpp Handler.cpp Parser.cpp

Buffer.o: Buffer.cpp Buffer.h 
	g++ -std=c++11 -g -c Buffer.cpp



database.o: database.cpp database.h 
	$(CXX) $(CXXFLAGS) -c $< -o $@ 
	#g++ -std=c++11 -c database.cpp

clean:
	rm -rf $(KVS_CLIENT_PATH)/*.o $(STORAGE_SERVICE_PATH)/*.o DNStest/*.o  *.o Servant
