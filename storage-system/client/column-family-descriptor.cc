#include "column-family-descriptor.h"

ColumnFamilyDescriptor::ColumnFamilyDescriptor(std::string column_family_name) : name(std::move(column_family_name)) {}