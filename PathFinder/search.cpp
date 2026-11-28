#include "search.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
using namespace std;

int search_index(const Paths& paths)
{
    const auto& index_path = paths.index_path;
    const auto& search_path = paths.search_path;
    // PART 4: Search existing files directly on disk.
    ifstream data(index_path, ios::binary);
    ifstream search(search_path, ios::binary);
    if (!data || !search) { cerr << "Cannot open search index\n"; return 1; }
    const uint64_t count = fs::file_size(search_path) / sizeof(IndexEntry);
    auto read_entry = [&](uint64_t i, IndexEntry& entry)
    {
        search.clear();
        search.seekg(static_cast<streamoff>(i * sizeof(IndexEntry)), ios::beg);
        return bool(search.read(reinterpret_cast<char*>(&entry), sizeof(entry)));
    };

    string query, key, path;
    while (true)
    {
        cout << "Search (exit to quit): " << flush;
        if (!getline(cin, query) || query == "exit") break;
        if (query.empty()) continue;
        const uint64_t hash = hash_key(query);
        uint64_t left = 0, right = count;
        IndexEntry entry{};
        while (left < right)
        {
            uint64_t mid = left + (right - left) / 2;
            if (!read_entry(mid, entry)) { cerr << "Index read error\n"; return 1; }
            if (entry.hash < hash) left = mid + 1;
            else right = mid;
        }
        size_t found = 0;
        // Read matching records sequentially after the lower-bound search.
        if (left < count)
        {
            search.clear();
            search.seekg(static_cast<streamoff>(left * sizeof(IndexEntry)), ios::beg);
            for (uint64_t i = left; i < count; ++i)
            {
                if (!search.read(reinterpret_cast<char*>(&entry), sizeof(entry))) return 1;
                if (entry.hash != hash) break;
                data.clear();
                data.seekg(static_cast<streamoff>(entry.offset), ios::beg);
                if (!getline(data, key) || !getline(data, path))
                { cerr << "Invalid index offset\n"; return 1; }
                // Verify original key to handle hash collisions.
                if (key == query) { cout << path << '\n'; ++found; }
            }
        }
        cout << "Found: " << found << '\n';
    }
    return 0;
}
