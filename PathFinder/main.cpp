#include "scanner.hpp"
#include "index.hpp"
#include "search.hpp"
#include <cstdlib>
#include <iostream>
using namespace std;

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
    const Paths paths{root, index_dir, paths_file, marker, index_path, search_path, search_marker};
    error_code ec;
    fs::create_directories(index_dir, ec);
    if (ec) { cerr << ec.message() << '\n'; return 1; }

    bool scanned = false;
    if (scan(paths, scanned) != 0) return 1;
    if (build_index(paths, scanned) != 0) return 1;
    return search_index(paths);
}
