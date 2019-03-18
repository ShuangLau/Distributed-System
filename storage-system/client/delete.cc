#include "delete.h"

Delete::Delete(std::string row_key) : Operation(std::move(row_key)) {}

const DeleteRowType & Delete::get_row() const {
  return row_;
}

void Delete::add_column(std::string column_family_name, std::string column_name) {
  row_[column_family_name].insert(column_name);
}