#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <pyle/error_reporter.hpp>

namespace pyle::lsp {

using json = nlohmann::json;

struct Position { 
    size_t line; 
    size_t character; 
};

struct Range { 
    Position start; 
    Position end; 
};

enum class SymbolKind {
    Function, 
    Struct, 
    Variable, 
    Parameter, 
    Field, 
    Module, 
    Keyword
};

struct SymbolInfo {
    std::string name;
    SymbolKind kind;
    std::string detail;
    std::string file_path;
    Range range;
    Range selection_range;
    std::string type_name;
    bool is_method = false;
    bool is_local = false;
    std::string parent_struct = "";
    std::string scope_func = "";
};

struct Document {
    std::string file_path;
    std::string source;
    pyle::ErrorReporter reporter;
    std::vector<SymbolInfo> symbols;
    std::map<std::string, std::string> imports;
    std::vector<std::string> import_paths;
};

inline const std::vector<std::pair<std::string, std::string>> LSP_KEYWORDS = {
    {"fn", "fn"}, {"let", "let"}, {"struct", "struct"},
    {"if", "if"}, {"else", "else"}, {"elif", "elif"},
    {"for", "for"}, {"while", "while"}, {"in", "in"},
    {"return", "return"}, {"break", "break"}, {"continue", "continue"},
    {"true", "boolean"}, {"false", "boolean"}, {"none", "none"},
    {"import", "import"}, {"and", "and"}, {"or", "or"}, {"not", "not"},
    {"yield", "yield"}, {"loop", "loop"}, {"static", "static"}
};

} // namespace pyle::lsp