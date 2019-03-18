HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CXXFLAGS += -std=c++11
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
PROTOS_PATH = ../protos

vpath %.proto $(PROTOS_PATH)

TARGET = master slave
TEST = testclient

all: $(TARGET) $(TEST)

master: ../common.o kvs.pb.o kvs.grpc.pb.o master.o calldata.o tablecalldata.o greetcalldata.o conncalldata.o emptycalldata.o alivecalldata.o
	$(CXX) $^ $(LDFLAGS) -o $@

testclient: kvs.pb.o kvs.grpc.pb.o testclient.o
	$(CXX) $^ $(LDFLAGS) -o $@

slave: ../common.o kvs.pb.o kvs.grpc.pb.o calldata.o operationcalldata.o getcalldata.o putcalldata.o slave.o deletecalldata.o precputcalldata.o cputcheckcalldata.o cputputcalldata.o checkpointcalldata.o logger.o shutdowncalldata.o emptycalldata.o downloadcalldata.o
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h $(TARGET) $(TEST)
