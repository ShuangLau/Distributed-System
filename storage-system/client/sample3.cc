#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <thread>
#include "configuration.h"
#include "table.h"
#include "column-family-descriptor.h"
#include "result.h"

std::vector<std::string> tables = { "table-1" };
// std::vector<std::string> rows = { "row-1" };
// std::vector<std::string> rows = { "row-1", "row-2", "row-3", "row-4"};
std::vector<std::string> col_families = { "f-1" };
// std::vector<std::string> col_families = { "f-1", "f-2", "f-3", "f-4", "f-5", "f-6" };
std::vector<std::string> cols = { "c-1" };
// std::vector<std::string> cols = { "c-1", "c-2", "c-3", "c-4", "c-5", "c-6", "c-7", "c-8", "c-9" };
// std::vector<std::string> contents = { "content-1", "content-2", "content-3", "content-4", "content-5", "content-6" };
std::vector<std::string> rows;
std::vector<std::string> contents;

typedef void (*func_ptr)(Table&, std::string, int);

func_ptr f[4];

void get(Table &table, std::string table_name, int thread_id) {
  int r = rand() % rows.size();
  int cf = rand() % col_families.size();
  int cl = rand() % cols.size();
  int c = rand() % contents.size();

  Get get(rows[r]);
  Result result = table.get(get);
  std::pair<byte*, int> p = result.get_value(col_families[cf], cols[cl]);
  if (p.first != nullptr) {
    std::string content((char*)(p.first), p.second);
    printf("thread %d reads table[%s][%s][%s][%s]= %s\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf].c_str(), cols[cl].c_str(), content.c_str());
  } else {
    printf("thread %d reads table[%s][%s][%s][%s]= no entry\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf].c_str(), cols[cl].c_str());
  }
}

void put(Table &table, std::string table_name, int thread_id) {
  printf("thread %d put...\n", thread_id);
  int r = rand() % rows.size();
  int cf = rand() % col_families.size();
  int cl = rand() % cols.size();
  int c = rand() % contents.size();
  
  // c = thread_id;

  Put put(rows[r]);
  put.add_column(col_families[cf], cols[cl], (byte*)contents[c].c_str(), contents[c].length());
  table.put(put);
  printf("thread %d writes table[%s][%s][%s][%s]= %s\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf].c_str(), cols[cl].c_str(), contents[c].c_str());
}

void del(Table &table, std::string table_name, int thread_id) {
  int r = rand() % rows.size();
  int cf = rand() % col_families.size();
  int cl = rand() % cols.size();
  int c = rand() % contents.size();

  Delete del(rows[r]);
  del.add_column(col_families[cf], cols[cl]);
  bool ret = table.del(del);
  if (ret) printf("thread %d deletes table[%s][%s][%s][%s] = true\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf].c_str(), cols[cl].c_str());
  else printf("thread %d deletes table[%s][%s][%s][%s] = false\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf].c_str(), cols[cl].c_str());
}

void cput(Table &table, std::string table_name, int thread_id) {
  printf("thread %d cput...\n", thread_id);
  int r = rand() % rows.size();

  int cf0 = rand() % col_families.size();
  int cl0 = rand() % cols.size();
  int c0 = rand() % contents.size();

  int cf1 = rand() % col_families.size();
  int cl1 = rand() % cols.size();
  int c1 = rand() % contents.size();

  // c0 = thread_id;

  Put put(rows[r]);
  put.add_column(col_families[cf1], cols[cl1], (byte*)contents[c1].c_str(), contents[c1].length());
  bool ret = table.check_and_put(rows[r], col_families[cf0], cols[cl0], (byte*)contents[c0].c_str(), contents[c0].length(), put);
  if (ret) printf("thread %d cput table[%s][%s][%s][%s] == %s, new value = %s\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf0].c_str(), cols[cl0].c_str(), contents[c0].c_str(), contents[c1].c_str());
  else printf("thread %d cput table[%s][%s][%s][%s] != %s\n", thread_id, table_name.c_str(), rows[r].c_str(), col_families[cf0].c_str(), cols[cl0].c_str(), contents[c0].c_str());
}

void test(std::string addr, int thread_id) {
  Connection conn = Configuration::connect(addr);

  int t = rand() % tables.size();
  Table table = conn.get_table(tables[t]);

  int iter = 400;
  while (iter--) f[rand() % 2](table, tables[t], thread_id);
}

void create_rows(int x) {
  for (int i = 1; i <= x; i++) rows.push_back("row-" + std::to_string(i));
}

void create_contents(int x) {
  for (int i = 1; i <= x; i++) contents.push_back("content-" + std::to_string(i));
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: ./sample2 master_address");
    return -1;
  }
  create_rows(100);
  create_contents(100);
  srand(time(NULL));
  f[0] = put;
  f[1] = del;
  f[2] = cput;
  f[3] = del;

  int N = 10;
  std::thread thread[N];
  for (int i = 0; i < N; i++) thread[i] = std::thread(test, std::string(argv[1]), i);
  for (int i = 0; i < N; i++) thread[i].join();

  Connection conn = Configuration::connect(std::string(argv[1]));
  Table table = conn.get_table(tables[0]);
  for (auto &r : rows) {
    // std::cout << "******** r = " << r << "*********" << std::endl;
    table.print_all_replicas(r, col_families[0], cols[0]);
  }
  // for (auto &r : rows) std::cout << std::hash<std::string>{}(r) << std::endl;
  // test(std::string(argv[1]));
  // Connection conn = Configuration::connect(std::string(argv[1]));

}