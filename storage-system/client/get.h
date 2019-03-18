#ifndef _GET_H
#define _GET_H

#include "operation.h"

class Get: public Operation {
public:
  Get(std::string row_key);
};
#endif