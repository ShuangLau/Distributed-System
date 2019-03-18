HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX=g++
CXXFLAGS += -std=c++11
OBJ=column-family-descriptor.o configuration.o connection.o delete.o get.o operation.o put.o result.o table-descriptor.o table.o ../common.o
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
PROTOS_PATH = ../protos

vpath %.proto $(PROTOS_PATH)

all: kvs.pb.cc kvs.grpc.pb.cc $(TARGET)

.PHONY: clean all

clean:
	rm -f *.pb.cc *.pb.h $(TARGET) $(OBJ) sample.o sample2.o sample3.o

sample: kvs.pb.o kvs.grpc.pb.o sample.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

sample2: kvs.pb.o kvs.grpc.pb.o sample2.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

sample3: kvs.pb.o kvs.grpc.pb.o sample3.o $(OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

.PRECIOUS: %.grpc.pb.cc
%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: %.pb.cc
%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<