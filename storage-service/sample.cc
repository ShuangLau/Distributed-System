#include "storage-service-user.h"
#include <string>
#include <iostream>
#include <vector>
#include "../storage-system/client/connection.h"

void print_dir(std::string path, StorageServiceUser &user) {
  std::vector<FileInfo> infos = user.list_dir(path);
  std::cout << std::endl << "*************listing dir***************" << std::endl;
  std::cout << "dir: " << path << std::endl;
  for (auto &info : infos) {
    std::cout << "\t" << info.name << " " << std::boolalpha << info.is_dir << " " << info.size << std::endl;
  }
  std::cout << "***************************************" << std::endl << std::endl;
}

int main(int argc, char **argv) {
  std::string user_name = "immiao";
  if (argc >= 2) user_name = std::string(argv[1]);
  Connection conn = Configuration::connect("127.0.0.1:50051");
  StorageServiceUser user("immiao", conn);

  if (user.is_exist("/rename-usr")) {
    printf("erase /rename-usr directory\n");
    user.erase("/rename-usr");
  }
  if (user.is_exist("/usr")) {
    printf("erase /usr directory\n");
    user.erase("/usr");
  }
  if (user.is_exist("/folder")) {
    printf("erase /folder directory\n");
    user.erase("/folder");
  }
  user.create_folder("/usr");

  user.create_file("/usr/file1.txt", "testing testing");
  std::string content = user.download_file("/usr/file1.txt");
  std::cout << "/usr/file1.txt content: " << content << std::endl;

  user.create_file("/usr/file2.txt", "testing testing");
  print_dir("/usr", user);

  if (user.is_exist("/file3.txt")) {
    printf("erase /file3.txt\n");
    user.erase("/file3.txt");
  }
  user.create_file("/file3.txt", "testing testing");

  if (user.is_exist("/file4.txt")) {
    printf("erase /file4.txt\n");
    user.erase("/file4.txt");
  }
  user.create_file("/file4.txt", "testing testing");

  if (user.is_exist("/file5.txt")) {
    printf("erase /file5.txt\n");
    user.erase("/file5.txt");
  }
  user.create_file("/file5.txt", "testing testing");
  print_dir("/", user);

  user.erase("/file3.txt");
  print_dir("/", user);

  user.erase("/usr/file1.txt");
  user.create_folder("/usr/folder");
  user.create_file("/usr/folder/file6.txt", "abcdefg");
  content = user.download_file("/usr/folder/file6.txt");
  print_dir("/usr", user);

  std::cout << "/usr/folder/file6.txt content: " << content << std::endl;
  user.erase("/usr");
  user.create_folder("/usr");
  user.create_folder("/usr/folder");
  user.create_file("/usr/folder/file6.txt", "abcdefg");
  print_dir("/usr/folder", user);

  user.move("/usr/folder/file6.txt", "/");
  user.move("/file6.txt", "/usr/");
  user.rename("/usr/file6.txt", "file7.txt");
  user.move("/usr/file7.txt", "/usr/folder/");
  user.rename("/usr", "rename-usr");
  user.create_file("/rename-usr/folder/file8.txt", "abcdefg");

  content = user.download_file("/rename-usr/folder/file7.txt");
  std::cout << "/rename-usr/folder/file7.txt content: " << content << std::endl;

  print_dir("/", user);
  print_dir("/rename-usr", user);
  print_dir("/rename-usr/folder", user);
  user.move("/rename-usr/folder", "/");

  print_dir("/", user);
  print_dir("/rename-usr", user);
  print_dir("/folder", user);
}