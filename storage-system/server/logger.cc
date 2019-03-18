#include "logger.h"
#include <fstream>

Logger::Logger(std::string slave_address) : slave_address_(slave_address) {}

void Logger::log_put(PutRequest &request) {
  std::fstream fin("log-" + slave_address_, std::ios::in | std::ios::binary);
  Log log;
  if (!log.ParseFromIstream(&fin)) std::cerr << "Parsing log file failed" << std::endl;
  fin.close();

  std::fstream fout("log-" + slave_address_, std::ios::out | std::ios::trunc | std::ios::binary);

  Operation *operation = log.add_operations();
  operation->set_table_name(request.table_name());
  operation->set_row_key(request.row_key());
  operation->set_type(Type::PUT);

  for (int i = 0; i < request.col_families_size(); i++) {
    const kvs::ColFamily &col_family = request.col_families(i);
    ColFamily *log_col_family = operation->add_col_families();
    log_col_family->set_col_family(col_family.col_family());

    for (int j = 0; j < col_family.cols_size(); j++) {
      const kvs::Col &col = col_family.cols(j);
      Col *log_col = log_col_family->add_cols();
      log_col->set_col(col.col());
      log_col->set_content(col.content());
    }
  }
  if (!log.SerializeToOstream(&fout)) std::cerr << "Output log file failed" << std::endl;
  fout.close();
}

void Logger::log_delete(DeleteRequest &request) {
  std::fstream fin("log-" + slave_address_, std::ios::in | std::ios::binary);
  Log log;
  if (!log.ParseFromIstream(&fin)) std::cerr << "Parsing log file failed" << std::endl;
  fin.close();

  std::fstream fout("log-" + slave_address_, std::ios::out | std::ios::trunc | std::ios::binary);

  Operation *operation = log.add_operations();
  operation->set_table_name(request.table_name());
  operation->set_row_key(request.row_key());
  operation->set_type(Type::DELETE);

  for (int i = 0; i < request.col_families_size(); i++) {
    const kvs::ColFamily &col_family = request.col_families(i);
    ColFamily *log_col_family = operation->add_col_families();
    log_col_family->set_col_family(col_family.col_family());

    for (int j = 0; j < col_family.cols_size(); j++) {
      const kvs::Col &col = col_family.cols(j);
      Col *log_col = log_col_family->add_cols();
      log_col->set_col(col.col());
    }
  }
  if (!log.SerializeToOstream(&fout)) std::cerr << "Output log file failed" << std::endl;
  fout.close();
}

void Logger::clear() {
  std::fstream fout("log-" + slave_address_, std::ios::out | std::ios::trunc | std::ios::binary);
  Log log;
  if (!log.SerializeToOstream(&fout)) std::cerr << "Clear log file failed" << std::endl;
  fout.close();
}

void Logger::print_log() {
  std::fstream fin("log-" + slave_address_, std::ios::in | std::ios::binary);
  Log log;
  if (!log.ParseFromIstream(&fin)) std::cerr << "Parsing log file failed" << std::endl;

  for (int i = 0; i < log.operations_size(); i++) {
    const Operation &operation = log.operations(i);
    std::cout << "Type: " << operation.type() << std::endl;

    for (int j = 0; j < operation.col_families_size(); j++) {
      const ColFamily &col_family = operation.col_families(j);
      for (int k = 0; k < col_family.cols_size(); k++) {
        const Col &col = col_family.cols(k);

        std::cout << operation.table_name() << "\t" << operation.row_key() << "\t" << col_family.col_family() << "\t" << col.col() << "\t" << col.content() << std::endl;
      }
    }
  }
  fin.close();
}