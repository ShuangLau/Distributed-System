#ifndef _COLUMN_FAMILY_DESCRIPTOR_H
#define _COLUMN_FAMILY_DESCRIPTOR_H

#include <string>

class ColumnFamilyDescriptor {
  std::string name;
public:
  ColumnFamilyDescriptor(std::string column_family_name);
};
#endif