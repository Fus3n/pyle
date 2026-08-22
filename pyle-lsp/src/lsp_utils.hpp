#pragma once

#include "lsp_types.hpp"
#include <sstream>
#include <fstream>
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

inline bool needs_url_encoding(char c) {
    return !(isalnum(static_cast<unsigned char>(c)) || c == '/' || c == '.' || c == '-' || c == '_' || c == ':');
}

inline std::string encode_url(const std::string& str) {
    std::string res;
    for (unsigned char c : str) {
        if (needs_url_encoding(static_cast<char>(c))) {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", c);
            res += buf;
        } else {
            res += static_cast<char>(c);
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
    if (uri.size() > 7 && uri.substr(0, 7) == "file://") {
        return normalize_path(decode_url(uri.substr(7)));
    }
    return normalize_path(decode_url(uri));
}

inline std::string path_to_file_uri(const std::string& path) {
    std::string n;
    for (char c : path) n += (c == '\\') ? '/' : c;
    n = encode_url(n);
    if (!n.empty() && n[0] != '/') return "file:///" + n;
    return "file://" + n;
}

inline std::string read_file_contents(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

inline Position span_to_position(const pyle::Span& span, const std::string& source) {
    Position pos;
    pos.line = span.line;
    pos.character = (span.column > 0) ? span.column - 1 : 0;
    return pos;
}

inline bool has_prefix(const std::string& str, const std::string& prefix) {
    return str.size() >= prefix.size() && str.compare(0, prefix.size(), prefix) == 0;
}

inline bool has_suffix(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline std::string word_at(const std::string& source, const Position& pos) {
    auto lines = get_lines(source);
    if (pos.line >= lines.size()) return "";
    const std::string& line = lines[pos.line];
    if (pos.character > line.size()) return "";

    size_t start = pos.character;
    while (start > 0 && (isalnum(static_cast<unsigned char>(line[start - 1])) || line[start - 1] == '_')) start--;
    size_t end = pos.character;
    while (end < line.size() && (isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) end++;
    return line.substr(start, end - start);
}


inline std::string prefix_at(const std::string& source, const Position& pos) {
    auto lines = get_lines(source);
    if (pos.line >= lines.size()) return "";
    const std::string& line = lines[pos.line];
    if (pos.character > line.size()) return "";

    bool at_word_end = pos.character > 0 &&
        (isalnum(static_cast<unsigned char>(line[pos.character - 1])) || line[pos.character - 1] == '_');
    bool mid_word = !at_word_end && pos.character < line.size() &&
        (isalnum(static_cast<unsigned char>(line[pos.character])) || line[pos.character] == '_');
    if (!at_word_end && !mid_word) return "";

    size_t start = pos.character;
    while (start > 0 && (isalnum(static_cast<unsigned char>(line[start - 1])) || line[start - 1] == '_')) start--;
    size_t end = pos.character;
    while (end < line.size() && (isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) end++;
    return line.substr(start, end - start);
}


inline std::string base_expression_before_dot(const std::string& text) {
    size_t dot = text.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return "";

    std::string raw = text.substr(0, dot);
    int paren = 0, bracket = 0, brace = 0;
    bool in_string = false;
    char quote = 0;

    for (int i = (int)raw.size() - 1; i >= 0; --i) {
        char c = raw[i];
        if (in_string) {
            if (c == quote && (i == 0 || raw[i - 1] != '\\')) in_string = false;
            continue;
        }
        if ((c == '"' || c == '\'') && (i == 0 || raw[i - 1] != '\\')) {
            in_string = true;
            quote = c;
            continue;
        }
        if (c == '(') paren--;
        else if (c == ')') paren++;
        else if (c == '[') bracket--;
        else if (c == ']') bracket++;
        else if (c == '{') brace--;
        else if (c == '}') brace++;

        if (paren < 0 || bracket < 0 || brace < 0) {
            if (i == 0) return raw;
            return raw.substr(i + 1);
        }
        if (paren == 0 && bracket == 0 && brace == 0) {
            if (c == ' ' || c == '\t' || c == '\n' ||
                c == ',' || c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                c == ';' || c == ':' ) {
                return raw.substr(i + 1);
            }
        }
    }
    return raw;
}

}
