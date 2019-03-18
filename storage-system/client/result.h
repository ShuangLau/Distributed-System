#ifndef _RESULT_H
#define _RESULT_H

#include <string>
#include <unordered_map>
#include <utility>
#include "../common.h"

class Result {
  RowType row_;
public:
  Result(RowType row);
  std::pair<byte*, int> get_value(std::string column_family_name, std::string column_name);
};

#endif