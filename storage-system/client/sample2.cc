#include <string>
#include <iostream>
#include <vector>
#include "configuration.h"
#include "table.h"
#include "column-family-descriptor.h"
#include "result.h"

std::vector<std::string> greetings = { "Hello World!", "Hello Bigtable!", "Hello HBase!" };
std::string table_name("table-1");
std::string column_family_name("column-family-1");
std::string column_name("column-1");

struct CustomStruct {
  std::string content = "Hey!";
  int id = 5;
  bool flag = false;
  std::vector<std::string> v = { "v[0]", "v[1]", "v[2]" };
} custom_struct;

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: ./sample2 master_address");
    return -1;
  }
  Connection conn = Configuration::connect(std::string(argv[1]));
  
  TableDescriptor table_descriptor(table_name);
  table_descriptor.add_family(ColumnFamilyDescriptor(column_family_name));
  conn.create_table(table_descriptor);

  // fetch the information of a table
  Table table = conn.get_table(table_name);

  /* 1. Basic put/get */
  int i = 0;
  for (auto &greeting : greetings) {
    std::string row_key("greeting" + std::to_string(i++));

    // configure a Put operation locally
    Put put(row_key);

    // convert a message to bytes first
    put.add_column(column_family_name, column_name, (byte*)greeting.c_str(), greeting.length() + 1);

    // this is the actual remote put
    table.put(put);
  }

  std::string row_key("greeting1");

  // Result is basically a row
  Result result = table.get(Get(row_key));

  // get_value returns bytes
  // convert bytes to char (or your custom structure/class)
  std::pair<byte*, int> p = result.get_value(column_family_name, column_name);
  assert(p.first != nullptr);
  std::cout << p.second << std::endl;
  std::string greeting((char*)(p.first), p.second);
  std::cout << "Get a single greeting by row key --- " << row_key << " = " << greeting << std::endl;
}