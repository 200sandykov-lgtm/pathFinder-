#include "library.hpp"

using namespace std;
namespace fs = std::filesystem;

int main()
{
//part 1 start

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
        //part 1 end 
        //part 2 start

    ifstream file1("C:/Users/200sa/AppData/Local/PathFinder/paths.txt");
    if (!file1.is_open())
    {
        cerr<< "cannot open the file";
        return 1;
    }

    char ch;
    int part = 0;
    class Part
    {
        public:
            int part_number;
            string part_name;
            Part (string part_name, int part_number): 
                part_name(part_name), part_number(part_number)
                {}
    };

    vector<Part> part_storage;
    vector<vector<Part>> path_storage;
    string line;
    int part_number = 0;

    while (getline(file1, line))
    {
        string variable;
        part_number = 0;

        for (char ch : line)
        {
            if (ch == '\\')
            {
                if (!variable.empty())
                {
                    part_storage.emplace_back(variable, part_number++);
                    variable.clear();
                }
            }
            else
            {
                variable += ch;
            }
        }

        if (!variable.empty())
        {
            part_storage.emplace_back(variable, part_number);
        }

        path_storage.push_back(part_storage);
        part_storage.clear();
    }
//part 2 end
//part 3 start
    ofstream index_file("index.idx");

    for (const auto& path : path_storage)
    {
        if (path.empty())
            continue;

    // Получаем название последнего Part
        string key = path.back().part_name;

    // Записываем ключ
        index_file << key << '\n';

    // Восстанавливаем полный путь
        for (size_t i = 0; i < path.size(); i++)
        {
            index_file << path[i].part_name;

            if (i != path.size() - 1)
            {
                index_file << '\\';
            }
        }
    }

        index_file << '\n';
        //part 3 end


    return 0;
}
