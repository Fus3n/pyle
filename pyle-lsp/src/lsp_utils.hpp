#pragma once

#include "lsp_types.hpp"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cctype>

namespace pyle::lsp::utils {

inline std::vector<std::string> get_lines(const std::string& str) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

inline std::string strip_quotes(const std::string& str) {
    if (str.size() >= 2 && (str.front() == '"' || str.front() == '\'') && str.back() == str.front()) {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

inline std::string decode_url(const std::string& str) {
    std::string res;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            int hex;
            std::istringstream iss(str.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                res += static_cast<char>(hex);
                i += 2;
            } else {
                res += '%';
            }
        } else {
            res += str[i];
        }
    }
    return res;
}

inline std::string normalize_path(const std::string& p) {
#ifdef _WIN32
    std::string n = p;
    for (auto& c : n) if (c == '/') c = '\\';
    for (auto& c : n) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return n;
#else
    return p;
#endif
}

inline std::string file_uri_to_path(const std::string& uri) {
    if (uri.size() > 8 && uri.substr(0, 8) == "file:///") {
        return normalize_path(decode_url(uri.substr(8)));
    }
    return normalize_path(decode_url(uri));
}

inline std::string path_to_file_uri(const std::string& path) {
    std::string n;
    for (char c : path) n += (c == '\\') ? '/' : c;
    if (!n.empty() && n[0] != '/') return "file:///" + n;
    return "file://" + n;
}

inline std::string read_file_contents(const std::string& path) {
    std::ifstream f(path);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

} 