#ifndef _DELETE_H
#define _DELETE_H

#include "operation.h"
#include <unordered_map>

typedef std::unordered_map<std::string, std::set<std::string>> DeleteRowType;

class Delete : public Operation {
  DeleteRowType row_;
public:
  Delete(std::string row_key);
  void add_column(std::string column_family_name, std::string column_name);
  const DeleteRowType & get_row() const;
};

#endif