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
PROTOS_PATH = ../storage-system/protos

vpath %.proto $(PROTOS_PATH)

TARGET = sample
KVS_CLIENT_PATH = ../storage-system/client
FILTER = $(KVS_CLIENT_PATH)/sample.cc $(KVS_CLIENT_PATH)/sample2.cc $(KVS_CLIENT_PATH)/sample3.cc
KVS_CLIENT_SOURCES = $(filter-out $(FILTER), $(wildcard $(KVS_CLIENT_PATH)/*.cc))
KVS_CLIENT_SOURCES += $(KVS_CLIENT_PATH)/kvs.pb.cc $(KVS_CLIENT_PATH)/kvs.grpc.pb.cc
KVS_CLIENT_OBJECTS = $(KVS_CLIENT_SOURCES:$(KVS_CLIENT_PATH)/%.cc=lib/%.o)

all: $(KVS_CLIENT_PATH)/kvs.pb.cc $(KVS_CLIENT_PATH)/kvs.grpc.pb.cc $(TARGET)

lib/%.o: $(KVS_CLIENT_PATH)/%.cc
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

%.o: %.cc
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

$(TARGET): sample.o storage-service-user.o $(KVS_CLIENT_OBJECTS)
	$(CXX) $^ $(LDFLAGS) -g -o $@

.PRECIOUS: $(KVS_CLIENT_PATH)/%.grpc.pb.cc
$(KVS_CLIENT_PATH)/%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=$(KVS_CLIENT_PATH) --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

.PRECIOUS: $(KVS_CLIENT_PATH)/%.pb.cc
$(KVS_CLIENT_PATH)/%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=$(KVS_CLIENT_PATH) $<

clean:
	rm -rf *.o *.pb.cc *.pb.h $(TARGET) $(KVS_CLIENT_OBJECTS)
