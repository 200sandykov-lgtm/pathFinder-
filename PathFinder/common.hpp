#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
struct IndexEntry { uint64_t hash; uint64_t offset; };
static_assert(sizeof(IndexEntry) == 16);
struct Paths {
    fs::path root, index_dir, paths_file, marker, index_path, search_path, search_marker;
};
uint64_t hash_key(std::string_view key);
std::string utf8_path(const fs::path& path);
void remove_cr(std::string& s);
std::string_view filename_key(const std::string& path);
bool publish(const fs::path& temp, const fs::path& dest);
