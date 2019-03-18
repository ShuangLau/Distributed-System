#include "storage-service-user.h"

StorageServiceUser::StorageServiceUser(std::string user_name, Connection &conn) : user_name_(user_name), table_(conn.get_table(user_name + "-drive")) {
  std::string name("/");
  root_ = std::to_string(std::hash<std::string>{}(user_name_));
  byte is_dir = 1;
  Put p(root_);
  p.add_column(cf, "name", (byte*)name.c_str(), name.length());
  p.add_column(cf, "is_dir", (byte*)(&is_dir), 1);
  table_.put(p);
}

bool StorageServiceUser::is_dir(std::string key) {
  Result result = table_.get(Get(key));
  std::pair<byte*, int> pair = result.get_value(cf, "is_dir");
  // printf("is_dir = %d\n", *(pair.first));
  assert(pair.first != nullptr && pair.second == 1);
  return *pair.first;
}

std::string StorageServiceUser::generate_new_key(std::string path) {
  std::string new_key = std::to_string(std::hash<std::string>{}(user_name_ + path + std::to_string(std::time(nullptr))));
  Put p_path_map(path);
  p_path_map.add_column(cf, "key", (byte*)new_key.c_str(), new_key.length());
  table_.put(p_path_map);
  return std::move(new_key);
}

void StorageServiceUser::create_file(std::string path, std::string content) {
  if (is_exist(path)) {
    fprintf(stderr, "create_file: %s already exists\n", path.c_str());
    return;
  }
  int pos = path.find_last_of('/');
  std::string parent_path = path.substr(0, pos);
  std::string file_name = path.substr(pos + 1);
  std::string parent_key = get_file_key(parent_path);
  if (parent_key.empty()) {
    fprintf(stderr, "create_file: %s doesn't exist\n", parent_path.c_str());
    return;
  }
  if (!is_dir(parent_key)) {
    fprintf(stderr, "create_file: %s isn't a directory\n", parent_path.c_str());
    return;
  }
  
  
  // std::string new_key = std::to_string(std::hash<std::string>{}(user_name_ + path));
  std::string new_key = generate_new_key(path);

  byte is_dir = 0;
  int size = content.length();
  Put p(new_key);
  p.add_column(cf, "name", (byte*)file_name.c_str(), file_name.length());
  p.add_column(cf, "is_dir", (byte*)(&is_dir), 1);
  p.add_column(cf, "content", (byte*)content.c_str(), content.length());
  p.add_column(cf, "size", (byte*)(&size), sizeof(int));
  table_.put(p);

  put_in_folder(parent_key, new_key);
}

void StorageServiceUser::put_in_folder(std::string parent_key, std::string new_key) {
  std::string child_key = get_child(parent_key);
  if (child_key.empty()) {
    Put p_parent(parent_key);
    p_parent.add_column(cf, "child", (byte*)new_key.c_str(), new_key.length());
    table_.put(p_parent);
  } else {
    std::string tail_key = get_tail(child_key);
    assert(!tail_key.empty());
    Put p_tail(tail_key);
    p_tail.add_column(cf, "next", (byte*)new_key.c_str(), new_key.length());
    table_.put(p_tail);
  }  
}

bool StorageServiceUser::is_exist(std::string path) {
  std::string key = get_file_key(path);
  Result result = table_.get(Get(key));
  std::pair<byte*, int> p0 = result.get_value(cf, "name");
  std::pair<byte*, int> p1 = result.get_value(cf, "is_dir");
  std::pair<byte*, int> p2 = result.get_value(cf, "content");
  std::pair<byte*, int> p3 = result.get_value(cf, "is_dir");
  std::pair<byte*, int> p4 = result.get_value(cf, "next");
  std::pair<byte*, int> p5 = result.get_value(cf, "child");
  return p0.first || p1.first || p2.first || p3.first || p4.first || p5.first;
}

void StorageServiceUser::create_folder(std::string path) {
  if (is_exist(path)) {
    fprintf(stderr, "create_folder: %s already exists\n", path.c_str());
    return;
  }
  int pos = path.find_last_of('/');
  std::string parent_path = path.substr(0, pos);
  std::string folder_name = path.substr(pos + 1);
  std::string parent_key = get_file_key(path.substr(0, pos));
  if (parent_key.empty()) {
    fprintf(stderr, "create_folder: %s doesn't exist\n", parent_path.c_str());
    return;
  }
  if (!is_dir(parent_key)) {
    fprintf(stderr, "create_folder: %s isn't a directory\n", parent_path.c_str());
    return;
  }  
  // std::string new_key = std::to_string(std::hash<std::string>{}(user_name_ + path));
  std::string new_key = generate_new_key(path);

  byte is_dir = 1;
  Put p(new_key);
  p.add_column(cf, "name", (byte*)folder_name.c_str(), folder_name.length());
  p.add_column(cf, "is_dir", (byte*)(&is_dir), 1);
  table_.put(p);

  // if (child_key.empty()) {
  //   Put p_parent(parent_key);
  //   p_parent.add_column(cf, "child", (byte*)new_key.c_str(), new_key.length());
  //   table_.put(p_parent);

  // } else {
    
  //   std::string tail_key = get_tail(child_key);
  //   assert(!tail_key.empty());
  //   Put p_tail(tail_key);
  //   p_tail.add_column(cf, "next", (byte*)new_key.c_str(), new_key.length());
  //   table_.put(p_tail);
  // }
  put_in_folder(parent_key, new_key);
}

void StorageServiceUser::rename(std::string path, std::string name) {
  std::string key = get_file_key(path);
  if (key.empty()) {
    fprintf(stderr, "No such file\n");
    return;
  }

  std::string new_path = path.substr(0, path.find_last_of('/')) + "/" + name;
  remap(path, new_path, key);
  Put p(key);
  p.add_column(cf, "name", (byte*)name.c_str(), name.length());
  table_.put(p);
}

void StorageServiceUser::unlink_node(std::string path) {
  std::string key = get_file_key(path);
  int pos = path.find_last_of("/");
  std::string parent_path = path.substr(0, pos);
  std::string parent_key = get_file_key(parent_path);
  assert(!parent_key.empty());
  std::string child_key = get_child(parent_key);
  assert(!child_key.empty());
  if (child_key == key) {
    std::string next_key = get_next(child_key);
    update_child(parent_key, next_key);
  } else {
    std::string pre = child_key, cur = get_next(child_key);
    assert(!cur.empty());
    while (cur != key) {
      pre = cur;
      cur = get_next(cur);
    }
    std::string next = get_next(cur);
    update_next(pre, next);    
  }  
  update_next(key, "");
}

void StorageServiceUser::erase(std::string path) {
  std::string key = get_file_key(path);
  if (key.empty()) {
    fprintf(stderr, "erase: %s doesn't exist\n", path.c_str());
    return;
  }
  unlink_node(path);
  clear(key);
}

void StorageServiceUser::move(std::string path, std::string new_path) {
  if (new_path.back() == '/') new_path.pop_back();
  std::string key = get_file_key(path);
  if (key.empty()) {
    fprintf(stderr, "move: %s doesn't exist\n", path.c_str());
    return;
  }
  unlink_node(path);

  std::string new_parent_key = get_file_key(new_path);
  if (new_parent_key.empty()) {
    fprintf(stderr, "move: %s doesn't exist\n", new_path.c_str());
    return;
  }
  if (!is_dir(new_parent_key)) {
    fprintf(stderr, "move: %s is not a directory\n", new_path.c_str());
    return;
  }
  put_in_folder(new_parent_key, key);

  std::string new_full_path = new_path + "/" + get_name(key);

  remap(path, new_full_path, key);
  // Result result = table_.get(Get(key));
  // std::pair<byte*, int> pair = result.get_value(cf, "content");
  // create_file(new_path, std::string((char*)pair.first, pair.second));
  // erase(path);
}

void StorageServiceUser::remap(std::string path, std::string new_path, std::string key) {
  assert(!key.empty());
  // std::cout << path << std::endl;
  r_remap(path, new_path, get_child(key));
  remap_single(path, new_path, key);
}

void StorageServiceUser::remap_single(std::string path, std::string new_path, std::string key) {
  Delete d(path);
  d.add_column(cf, "key");
  table_.del(d);

  Put p_path_map(new_path);
  p_path_map.add_column(cf, "key", (byte*)key.c_str(), key.length());
  table_.put(p_path_map);
}

void StorageServiceUser::r_remap(std::string parent_path, std::string parent_new_path, std::string key) {
  std::string cur = key;
  while (!cur.empty()) {
    std::string name = get_name(cur);
    std::string path = parent_path + "/" + name;
    std::string new_path = parent_new_path + "/" + name;
    // std::cout << "parent path: " << parent_new_path << std::endl;
    // std::cout << "remapping " << path << " to " << new_path << std::endl;
    // std::cout << "child key: " << get_child(key).empty() << std::endl;
    r_remap(path, new_path, get_child(key));
    remap_single(path, new_path, key);
    cur = get_next(cur);
    // std::cout << "cur: " << cur << std::endl;
  }
}

void StorageServiceUser::clear(std::string key) {
  std::string child = get_child(key);
  r_clear(child);
  clear_single(key);
}

void StorageServiceUser::clear_single(std::string key) {
  Delete d(key);
  d.add_column(cf, "name");
  d.add_column(cf, "is_dir");
  d.add_column(cf, "content");
  d.add_column(cf, "size");
  d.add_column(cf, "next");
  d.add_column(cf, "child");
  bool ret = table_.del(d);
  assert(ret);
}

void StorageServiceUser::r_clear(std::string key) {
  if (key.empty()) return;
  std::string cur = key, pre;
  while (!cur.empty()) {
    std::string child = get_child(cur);
    r_clear(child);
    pre = cur;
    cur = get_next(cur);
    clear_single(pre);
  }
}

void StorageServiceUser::update_next(std::string cur_key, std::string new_next_key) {
  if (new_next_key.empty()) {
    Delete d(cur_key);
    d.add_column(cf, "next");
    table_.del(d);
  } else {
    Put p(cur_key);
    p.add_column(cf, "next", (byte*)new_next_key.c_str(), new_next_key.length());
    table_.put(p);
  }
}

void StorageServiceUser::update_child(std::string parent_key, std::string new_child_key) {
  if (new_child_key.empty()) {
    Delete d(parent_key);
    d.add_column(cf, "child");
    table_.del(d);
  } else {
    Put p(parent_key);
    p.add_column(cf, "child", (byte*)new_child_key.c_str(), new_child_key.length());
    table_.put(p);
  }
}

std::string StorageServiceUser::get_file_key(std::string path) {
  if (path.empty()) return root_;
  // std::string key = std::to_string(std::hash<std::string>{}(user_name_ + path));
  Result key_result = table_.get(Get(path));
  std::pair<byte*, int> pair = key_result.get_value(cf, "key");
  if (pair.first == nullptr && pair.second == 0) return std::string();
  // // check if exists
  // Result result = table_.get(Get(key));
  // std::pair<byte*, int> pair = result.get_value(cf, "name");
  // if (pair.first == nullptr && pair.second == 0) return std::string();
  // std::cout << std::string((char*)pair.first, pair.second) << std::endl;
  return std::string((char*)pair.first, pair.second);
}
// std::string StorageServiceUser::find_file(std::string path) {
//   if (path.empty()) return root_;
//   int pos = path.find_last_of('/');
//   std::string file_name(path.substr(pos + 1));
//   std::string parent_key = find_file(path.substr(0, pos));

//   std::string child_key = get_child(parent_key);
//   std::string key = find_file_cur_dir(child_key, file_name);
//   return key;
// }

// std::string StorageServiceUser::find_file_cur_dir(std::string head_key, std::string file_name) {
//   assert(!head_key.empty());
//   Result result = table_.get(Get(head_key));
//   std::pair<byte*, int> name = result.get_value(cf, "name");
//   if (std::string(name.first, name.second) == file_name) return head_key;
//   std::pair<byte*, int> next = result.get_value(cf, "next");
//   if (next.first == nullptr && next.second == 0) return std::string();
//   return find_file_cur_dir(std::string(next.first, next.second), file_name);
// }

std::string StorageServiceUser::get_tail(std::string head_key) {
  assert(!head_key.empty());
  std::string next_key = get_next(head_key);
  if (next_key.empty()) return head_key;
  return get_tail(next_key);
}

std::string StorageServiceUser::get_child(std::string parent_key) {
  assert(!parent_key.empty());
  Result result = table_.get(Get(parent_key));
  std::pair<byte*, int> next = result.get_value(cf, "child");
  if (next.first == nullptr && next.second == 0) return std::string();
  return std::string((char*)next.first, next.second);
}

std::string StorageServiceUser::get_next(std::string key) {
  assert(!key.empty());
  Result result = table_.get(Get(key));
  std::pair<byte*, int> next = result.get_value(cf, "next");
  if (next.first == nullptr && next.second == 0) return std::string();
  return std::string((char*)next.first, next.second);
}

std::string StorageServiceUser::download_file(std::string path) {
  std::string key = get_file_key(path);
  if (key.empty()) {
    fprintf(stderr, "download_file: %s doesn't exist\n", path.c_str());
    return std::string();
  }
  if (is_dir(key)) {
    fprintf(stderr, "download_file: %s isn't a file\n", path.c_str());
    return std::string();
  }
  Result result = table_.get(Get(key));
  std::pair<byte*, int> pair = result.get_value(cf, "content");
  return std::string((char*)pair.first, pair.second);
}

std::string StorageServiceUser::get_name(std::string key) {
  Result result = table_.get(Get(key));
  std::pair<byte*, int> pair = result.get_value(cf, "name");
  if (pair.first == nullptr && pair.second == 0) {
    fprintf(stderr, "get_name: file doesn't exist\n");
    return std::string();
  }
  return std::string((char*)pair.first, pair.second);
}

std::vector<FileInfo> StorageServiceUser::list_dir(std::string path) {
  if (path.back() == '/') path.pop_back();
  if (!is_exist(path)) {
    fprintf(stderr, "list_dir: %s doesn't exist\n", path.c_str());
    return std::vector<FileInfo>();
  }
  std::string key = get_file_key(path);
  if (!is_dir(key)) {
    fprintf(stderr, "list_dir: %s is not a directory\n", path.c_str());
    return std::vector<FileInfo>();
  }
  std::vector<FileInfo> infos;
  std::string cur = get_child(key);
  while (!cur.empty()) {
    FileInfo info;
    Result result = table_.get(Get(cur));
    std::pair<byte*, int> p_name = result.get_value(cf, "name");
    std::pair<byte*, int> p_is_dir = result.get_value(cf, "is_dir");
    std::pair<byte*, int> p_size = result.get_value(cf, "size");

    info.name = std::string((char*)p_name.first, p_name.second);
    info.is_dir = *(bool*)p_is_dir.first;
    info.size = info.is_dir ? 0 : *(int*)p_size.first;

    infos.push_back(info);

    cur = get_next(cur);
  }
  return std::move(infos);
}

bool StorageServiceUser::is_folder(std::string path) {
  return is_dir(get_file_key(path));
}