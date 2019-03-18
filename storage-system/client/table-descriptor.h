#ifndef _TABLE_DESCRIPTOR_H
#define _TABLE_DESCRIPTOR_H

#include <string>
#include <vector>
#include "column-family-descriptor.h"

class TableDescriptor {
  std::string table_name;
  std::vector<ColumnFamilyDescriptor> column_families;
public:
  TableDescriptor(std::string table_name);
  void add_family(const ColumnFamilyDescriptor &descriptor);
  const std::string & get_table_name() const;
};

#endif