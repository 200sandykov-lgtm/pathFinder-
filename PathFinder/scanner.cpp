#include "scanner.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
using namespace std;

int scan(const Paths& paths, bool& scanned)
{
    const auto& root = paths.root;
    const auto& index_dir = paths.index_dir;
    const auto& paths_file = paths.paths_file;
    const auto& marker = paths.marker;
    const auto& search_marker = paths.search_marker;
    error_code ec;
    // PART 1: Scan only if the saved snapshot is missing or incomplete.
    scanned = false;
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

    return 0;
}
