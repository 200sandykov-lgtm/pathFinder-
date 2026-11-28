#include "common.hpp"
#include <iostream>
using namespace std;

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

