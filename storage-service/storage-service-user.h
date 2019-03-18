#ifndef _STORAGE_SERVICE_USER_H
#define _STORAGE_SERVICE_USER_H

#include "../storage-system/client/configuration.h"
#include "../storage-system/client/table.h"
#include "../storage-system/client/column-family-descriptor.h"
#include "../storage-system/client/result.h"
#include <vector>

#define cf "attribute"

struct FileInfo {
  std::string name;
  bool is_dir;
  int size;
};

class StorageServiceUser {
public:
  StorageServiceUser(std::string user_name, Connection &conn);

  /**
   * Create a file
   * @param path      For example, "/usr/file1.txt"
   * @param content   File content
   */
  void create_file(std::string path, std::string content);

  /**
   * Create a folder
   * @param path.     For example, "/usr/folder"
   */
  void create_folder(std::string path);

  /**
   * Rename a file or a folder
   * @param path      the path of the file to be renamed. For example, "/usr/file1.txt"
   * @param name      the new name
   */
  void rename(std::string path, std::string name);

  /**
   * Delete a file or a folder
   * @param path      the path of the file to be erased. For example, "/usr/file1.txt"
   */
  void erase(std::string path);

  /**
   * Move a file or a folder
   * @param path      For example, "/usr/file1.txt"
   * @param new_path  Destination folder. For example, "/usr/folder"
   */
  void move(std::string path, std::string new_path);

  /**
   * Download the content of a file
   * @param path      For example, "/usr/file1.txt"
   */
  std::string download_file(std::string path);

  /**
   * Get the information of all the files/folders in the current directory.
   * The size of a folder is always 0.
   * @param path      The path of a folder. For example, "/usr/folder".
   * @return          a vector of FileInfo. The vector is empty if no files/folders exist
   */
  std::vector<FileInfo> list_dir(std::string path);

  /**
   * Check if a file/folder exists
   * @param path      The path of a file/folder to check
   * @return          true if exists, otherwise false
   */
  bool is_exist(std::string path);

  bool is_folder(std::string path);
private:
  std::string root_;
  std::string user_name_;
  Table table_;

  bool is_dir(std::string key);
  void update_next(std::string cur_key, std::string new_next_key);
  void update_child(std::string parent_key, std::string new_child_key);
  std::string get_file_key(std::string path);
  std::string get_tail(std::string head_key);
  std::string get_child(std::string parent_key);
  std::string get_next(std::string key);
  void clear(std::string key);
  void r_clear(std::string key);
  void clear_single(std::string key);
  std::string generate_new_key(std::string path);
  void remap(std::string parent_path, std::string parent_new_path, std::string key);
  void r_remap(std::string parent_path, std::string parent_new_path, std::string key);
  void remap_single(std::string parent_path, std::string parent_new_path, std::string key);
  std::string get_name(std::string key);
  void unlink_node(std::string path);
  void put_in_folder(std::string parent_key, std::string new_key);
};

#endif