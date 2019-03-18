#include "table-descriptor.h"

TableDescriptor::TableDescriptor(std::string table_name) : table_name(std::move(table_name)) {}

void TableDescriptor::add_family(const ColumnFamilyDescriptor &descriptor) {
  column_families.push_back(descriptor);
}

const std::string & TableDescriptor::get_table_name() const {
  return table_name;
}