#include "result.h"
#include <iostream>
#include <cassert>

Result::Result(RowType row) : row_(row) {}

std::pair<byte*, int> Result::get_value(std::string column_family_name, std::string column_name) {
  // assert(row_[column_family_name][column_name].first != nullptr);
  // std::cout << row_[column_family_name][column_name].second << std::endl;
  auto iter0 = row_.find(column_family_name);
  if (iter0 == row_.end()) return { nullptr, 0 };
  auto iter1 = iter0->second.find(column_name);
  if (iter1 == iter0->second.end()) return { nullptr, 0 };
  return iter1->second;
  // return row_[column_family_name][column_name];
}