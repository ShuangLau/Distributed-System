#include "checkpointcalldata.h"

CheckPointCallData::CheckPointCallData(Kvs::AsyncService* service, ServerCompletionQueue* cq, TableType &tables, std::string slave_address, Logger &logger) : OperationCallData(service, cq, tables), responder_(&ctx_), slave_address_(std::move(slave_address)), logger_(logger) {
  Proceed();
}

void PrintCheckPoint(std::string slave_address) {
  std::fstream fin("checkpoint-" + slave_address, std::ios::in | std::ios::binary);
  Data data;
  if (!data.ParseFromIstream(&fin)) {
    std::cerr << "Failed to parse check point." << std::endl;
  }
  std::cout << "version: " << data.version() <<std::endl;
  for (int i = 0; i < data.tables_size(); i++) {
    const Table &table = data.tables(i);
    for (int j = 0; j < table.rows_size(); j++) {
      const Row &row = table.rows(j);
      for (int k = 0; k < row.col_families_size(); k++) {
        const ColFamily &col_family = row.col_families(k);
        for (int l = 0; l < col_family.cols_size(); l++) {
          const Col &col = col_family.cols(l);

          std::cout << table.table_name() << "\t" << row.row_key() << "\t" << col_family.col_family() << "\t" << col.col() << "\t" << col.content() << std::endl;
        }
      }
    }
  }
  fin.close();
}

// assume no fault during dumping
void Dump(int version, const TableType &tables, std::string slave_address, Logger &logger) {
  logger.clear();

  std::cout << "dump: " << version << std::endl;
  std::fstream fout("checkpoint-" + slave_address, std::ios::out | std::ios::trunc | std::ios::binary);
// std::cout << "here-1" << std::endl;

  Data data;
  data.set_version(version);
    // std::cout << "here-2" << std::endl;

  for (auto &table_pair : tables) {
    Table *table = data.add_tables();
    table->set_table_name(table_pair.first);
    log_print("table name: " + table_pair.first);
    // std::cout << "here0" << std::endl;

    for (auto &row_pair : table_pair.second) {
      Row *row = table->add_rows();
      row->set_row_key(row_pair.first);

// std::cout << "here1" << std::endl;
      for (auto &col_family_pair : row_pair.second.row) {
        ColFamily *col_family = row->add_col_families();
        col_family->set_col_family(col_family_pair.first);

// std::cout << "here2" << std::endl;
        for (auto &col_pair : col_family_pair.second) {
          Col *col = col_family->add_cols();
          col->set_col(col_pair.first);

// std::cout << "here3" << std::endl;
          const std::pair<byte*, int> &pair = col_pair.second;
          log_print("content: " + std::string((char*)pair.first, pair.second));
          col->set_content(std::string((char*)pair.first, pair.second));
        }
      }
    }
  }
  if (!data.SerializeToOstream(&fout)) {
    std::cerr << "Dump failed" << std::endl;
  }
  fout.close();
  PrintCheckPoint(slave_address);
}

void test() {
  std::cout << "test" << std::endl;
}

void CheckPointCallData::Proceed() {
  if (status_ == CREATE) {
    status_ = PROCESS;

    service_->RequestCheckPoint(&ctx_, &request_, &responder_, cq_, cq_, this);
  } else if (status_ == PROCESS) {
    new CheckPointCallData(service_, cq_, tables_, slave_address_, logger_);

    std::thread thread(Dump, request_.version(), std::ref(tables_), slave_address_, std::ref(logger_));
    // std::thread thread(test);
    thread.detach();

    status_ = FINISH;
    responder_.Finish(reply_, Status::OK, this);
  } else {
    GPR_ASSERT(status_ == FINISH);
    delete this;
  }
}