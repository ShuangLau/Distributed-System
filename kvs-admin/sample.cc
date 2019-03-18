#include "kvsadmin.h"

int main() {
  KvsAdmin admin;
  admin.shutdown_slave("127.0.0.1:10000");
}