#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct IndexEntry
{
    uint64_t hash;
    uint64_t offset;
};
static_assert(sizeof(IndexEntry) == 16);

uint64_t hash_key(string_view key)
{
    uint64_t hash = 14695981039346656037ULL;
    for (unsigned char ch : key)
    {
        hash ^= ch;
        hash *= 1099511628211ULL;
    }
    return hash;
}

string utf8_path(const fs::path& path)
{
    auto s = path.u8string();
    return string(reinterpret_cast<const char*>(s.data()), s.size());
}

void remove_cr(string& s)
{
    if (!s.empty() && s.back() == '\r')
        s.pop_back();
}

string_view filename_key(const string& path)
{
    auto pos = path.find_last_of("/\\");
    return string_view(path).substr(pos == string::npos ? 0 : pos + 1);
}

bool publish(const fs::path& temp, const fs::path& dest)
{
    error_code ec;
    fs::rename(temp, dest, ec);
    if (ec)
        cerr << "Cannot publish " << utf8_path(dest) << ": " << ec.message() << '\n';
    return !ec;
}

int main()
{
    const fs::path root = "C:/";
    const char* local = getenv("LOCALAPPDATA");
    const fs::path index_dir = local ? fs::path(local) / "PathFinder"
                                    : fs::path("C:/Users/200sa/AppData/Local/PathFinder");
    const fs::path paths_file = index_dir / "paths.txt";
    const fs::path marker = index_dir / "index_ready.flag";
    const fs::path index_path = index_dir / "index.idx";
    const fs::path search_path = index_dir / "search.bin";
    const fs::path search_marker = index_dir / "search_ready_v1.flag";
    error_code ec;
    fs::create_directories(index_dir, ec);
    if (ec) { cerr << ec.message() << '\n'; return 1; }

    // PART 1: Scan only if the saved snapshot is missing or incomplete.
    bool scanned = false;
    if (!fs::exists(marker) || !fs::exists(paths_file))
    {
        fs::remove(marker, ec);
        if (ec) return 1;
        fs::remove(search_marker, ec);
        if (ec) return 1;
        const fs::path temp = index_dir / "paths.tmp";
        ofstream out(temp, ios::binary | ios::trunc);
        if (!out) { cerr << "Cannot create paths.tmp\n"; return 1; }

        vector<fs::path> pending{root};
        size_t count = 0, errors = 0;
        while (!pending.empty())
        {
            fs::path current_dir = move(pending.back());
            pending.pop_back();
            fs::directory_iterator it(current_dir, ec), end;
            if (ec) { ++errors; continue; }
            while (it != end)
            {
                const fs::path path = it->path();
                // Do not index the application's own generated files.
                if (path.lexically_normal() != index_dir.lexically_normal())
                {
                    out << utf8_path(path) << '\n';
                    if (!out) return 1;
                    ++count;
                    error_code type_ec;
                    auto status = it->symlink_status(type_ec);
                    if (type_ec) ++errors;
                    else if (fs::is_directory(status)) pending.push_back(path);
                }
                it.increment(ec);
                if (ec) { ++errors; break; }
            }
        }
        out.close();
        if (!out || !publish(temp, paths_file)) return 1;
        ofstream flag(marker);
        flag.close();
        if (!flag) return 1;
        scanned = true;
        cout << "Indexed paths: " << count << " | Scan errors: " << errors << '\n';
    }

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
