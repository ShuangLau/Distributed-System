#include "put.h"

Put::Put(std::string row_key) : Operation(std::move(row_key)) {}

void Put::add_column(std::string column_family_name, std::string column_name, byte *content, int content_len) {
  if (content != nullptr && content_len > 0) {
    byte *data = (byte*)malloc(content_len);
    memcpy(data, content, content_len);
    row_[column_family_name][column_name] = {data, content_len};
  }
}

const RowType & Put::get_row() const {
  return row_;
}