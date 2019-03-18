#include "configuration.h"

Connection Configuration::connect(std::string addr) {
  return Connection(addr);
}