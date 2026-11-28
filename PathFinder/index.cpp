#include "index.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
using namespace std;

int build_index(const Paths& paths, bool scanned)
{
    const auto& index_dir = paths.index_dir;
    const auto& paths_file = paths.paths_file;
    const auto& index_path = paths.index_path;
    const auto& search_path = paths.search_path;
    const auto& search_marker = paths.search_marker;
    error_code ec;
    // PARTS 2 AND 3: Build both indexes in one pass, only when needed.
    // Check timestamps as well, so a changed source invalidates the index.
    bool ready = !scanned && fs::exists(search_marker) &&
                 fs::exists(index_path) && fs::exists(search_path);
    if (ready)
    {
        auto source_time = fs::last_write_time(paths_file);
        auto index_time = fs::last_write_time(index_path);
        auto search_time = fs::last_write_time(search_path);
        ready = source_time <= index_time && index_time <= search_time &&
                fs::file_size(search_path) % sizeof(IndexEntry) == 0;
    }
    if (!ready)
    {
        fs::remove(search_marker, ec);
        if (ec) return 1;
        ifstream source(paths_file, ios::binary);
        const fs::path index_temp = index_dir / "index.tmp";
        const fs::path search_temp = index_dir / "search.tmp";
        ofstream index(index_temp, ios::binary | ios::trunc);
        if (!source || !index) { cerr << "Cannot open index build files\n"; return 1; }

        vector<IndexEntry> entries;
        string path;
        uint64_t offset = 0;
        while (getline(source, path))
        {
            remove_cr(path);
            if (path.empty()) continue;
            string_view key = filename_key(path);
            if (key.empty()) continue;
            entries.push_back({hash_key(key), offset});
            index << key << '\n' << path << '\n';
            if (!index) return 1;
            offset += key.size() + path.size() + 2;
        }
        if (!source.eof() || source.bad()) return 1;
        index.close();
        if (!index) return 1;

        sort(entries.begin(), entries.end(), [](const IndexEntry& a, const IndexEntry& b)
        {
            return a.hash != b.hash ? a.hash < b.hash : a.offset < b.offset;
        });
        ofstream binary(search_temp, ios::binary | ios::trunc);
        if (!binary) return 1;
        if (!entries.empty())
            binary.write(reinterpret_cast<const char*>(entries.data()),
                         static_cast<streamsize>(entries.size() * sizeof(IndexEntry)));
        binary.close();
        if (!binary || !publish(index_temp, index_path) || !publish(search_temp, search_path))
            return 1;
        ofstream flag(search_marker);
        flag.close();
        if (!flag) return 1;
        cout << "Search index created: " << entries.size() << " entries\n";
    }

    return 0;
}
