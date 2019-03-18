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

int main() {
  Connection conn = Configuration::connect("127.0.0.1:50051");
  
  // TableDescriptor table_descriptor(table_name);
  // table_descriptor.add_family(ColumnFamilyDescriptor(column_family_name));
  // conn.create_table(table_descriptor);

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

  // get_value returns <bytes, length>
  
  std::pair<byte*, int> pair = result.get_value(column_family_name, column_name);
  // convert bytes to char (or your custom structure/class)
  std::string greeting((char*)pair.first, pair.second);
  std::cout << "Get a single greeting by row key --- " << row_key << " = " << greeting << std::endl;

  /* 2. check_and_put (CPUT) returns true */
  Put put0("greeting1");
  std::string new_content("new content");
  put0.add_column(column_family_name, column_name, (byte*)new_content.c_str(), new_content.length() + 1);

  bool ret0 = table.check_and_put("greeting0", column_family_name, column_name, (byte*)greetings[0].c_str(), greetings[0].length() + 1, put0);
  std::cout << std::boolalpha << "check_and_put returns " << ret0 << std::endl;

  pair = table.get(Get("greeting1")).get_value(column_family_name, column_name); 
  std::cout << "table[greeting1][column-family-1][column-1] has been updated to \"" << std::string((char*)pair.first, pair.second) << "\"" << std::endl;
  
  /* 3. check_and_put (CPUT) returns false */
  Put put1("greeting0");
  put1.add_column(column_family_name, column_name, (byte*)new_content.c_str(), new_content.length() + 1);

  bool ret1 = table.check_and_put("greeting0", column_family_name, column_name, (byte*)greetings[1].c_str(), greetings[1].length() + 1, put1);
  std::cout << std::boolalpha << "check_and_put returns " << ret1 << std::endl;
  pair = table.get(Get("greeting0")).get_value(column_family_name, column_name);
  std::cout << "table[greeting0][column-family-1][column-1] is still \"" << std::string((char*)pair.first, pair.second) << "\"" << std::endl;

  /* 4. custom struct put/get */
  Put put("custom struct"); // row key is "custom struct"
  put.add_column("abc", "def", (byte*)&custom_struct, sizeof(custom_struct));
  table.put(put);

  Result result2 = table.get(Get("custom struct"));
  CustomStruct *p = reinterpret_cast<CustomStruct*>(result2.get_value("abc", "def").first);
  std::cout << "p->content = " << p->content << std::endl;
  std::cout << "p->id = " << p->id << std::endl;
  std::cout << std::boolalpha << "p->flag = " << p->flag << std::endl;
  std::cout << "p->v = { " << p->v[0] << " " <<  p->v[1] << " " << p->v[2] << " }" << std::endl;

  std::cout << "Delete the table" << std::endl;
  conn.delete_table(table_name);
}