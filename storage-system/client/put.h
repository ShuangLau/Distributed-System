#ifndef _PUT_H
#define _PUT_H

#include <unordered_map>
#include <utility>
#include "../common.h"
#include "operation.h"

class Put : public Operation {
  RowType row_;
public:
  Put(std::string row_key);
  void add_column(std::string column_family_name, std::string column_name, byte *content, int content_len);
  const RowType & get_row() const;
};

#endif
