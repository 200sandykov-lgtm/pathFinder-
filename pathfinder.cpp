#include "library.hpp"

using namespace std;
namespace fs = std::filesystem;

int main()
{
    const fs::path root = "C:/";
    const fs::path index_dir = "C:/Users/200sa/AppData/Local/PathFinder";
    error_code ec;
    fs::path marker = "C:/Users/200sa/AppData/Local/PathFinder/index_ready.flag";

    if (!fs::exists (marker))
    {
        fs::create_directories(index_dir, ec);
        if (ec)
        {
            cerr << "Cannot create index directory: " << ec.message() << '\n';
            return 1;
        }

        ofstream out_file(index_dir / "paths.txt");
        if (!out_file)
        {
            cerr << "Cannot open paths.txt for writing\n";
            return 1;
        }

        vector<fs::path> pending{root};
        size_t count = 0;
        size_t errors = 0;

        while (!pending.empty())
        {
            const fs::path current_dir = pending.back();
            pending.pop_back();
            fs::directory_iterator it(current_dir, ec), end;
            if (ec)
            {
                cerr << "Cannot open directory: " << current_dir.u8string()
                    << ": " << ec.message() << '\n';
                ++errors;
                continue;
            }
            while (it != end)
            {
                const fs::path current_path = it->path();
                out_file << current_path.u8string() << '\n';
                if (!out_file)
                {
                    cerr << "Cannot write to paths.txt\n";
                    return 1;
                }
                ++count;
                error_code type_ec;
                const auto status = it->symlink_status(type_ec);
                if (type_ec)
                {
                    cerr << "Cannot read attributes: " << current_path.u8string()
                        << ": " << type_ec.message() << '\n';
                    ++errors;
                }
                else if (fs::is_directory(status))
                {
                    pending.push_back(current_path);
                }
                it.increment(ec);
                if (ec)
                {
                    cerr << "Cannot continue directory: " << current_dir.u8string()
                        << " (after " << current_path.u8string() << "): "
                        << ec.message() << '\n';
                    ++errors;
                    break;
                }
            }
        }

        out_file.close();

        if (!out_file)
        {
            cerr << "Cannot finish writing paths.txt\n";
            return 1;
        }

        cout << "Indexed paths: " << count << '\n';
        cout << "Scan errors: " << errors << '\n';
        ofstream flag(marker);
    }
    return 0;
}
