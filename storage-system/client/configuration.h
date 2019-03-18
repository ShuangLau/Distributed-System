#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

#include <string>
#include "connection.h"

class Configuration {
public:
  static Connection connect(std::string addr);
};

#endif