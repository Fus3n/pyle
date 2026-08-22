#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <pyle/error_reporter.hpp>
#include <pyle/ast.hpp>

namespace pyle::lsp {

struct DocumentModel;

struct Position {
    size_t line = 0;
    size_t character = 0;
};

struct Range {
    Position start;
    Position end;
};

enum class SymbolKind {
    Function,
    Method,
    StaticFunction,
    Struct,
    Variable,
    Parameter,
    Field,
    Module,
    Keyword
};

struct SymbolInfo {
    std::string name;
    SymbolKind kind = SymbolKind::Variable;
    std::string detail;
    std::string documentation;
    std::string file_path;
    Range range;
    Range selection_range;
    std::string type_name;
    bool has_type_hint = false;
    bool is_local = false;
    int scope_end_line = -1;
    std::string parent_struct = "";
    std::string scope_func = "";
    bool is_static = false;
    const DocumentModel* owner_doc = nullptr;
};

struct DocumentModel {
    std::string file_path;
    std::string source;
    pyle::ErrorReporter reporter;
    std::vector<SymbolInfo> symbols;
    std::map<std::string, std::string> imports;
    std::vector<std::string> import_paths;
    std::vector<std::unique_ptr<pyle::Stmt>> ast;
    std::vector<pyle::Token> tokens;
    bool is_definition_file = false;
    std::map<std::string, std::vector<SymbolInfo*>> struct_members;
    std::map<std::string, SymbolInfo*> struct_symbols;
};

inline const std::vector<std::string> BUILTIN_TYPES = {
    "string", "int", "float", "bool", "array", "map",
    "bytes", "coro", "function", "range", "iterator", "none"
};

inline const std::string ANY_TYPE = "any";

struct Diagnostic {
    size_t line = 0;
    size_t character = 0;
    size_t length = 1;
    int severity = 1;
    std::string message;
};

inline const std::vector<std::pair<std::string, std::string>> LSP_KEYWORDS = {
    {"fn", "fn"}, {"let", "let"}, {"struct", "struct"},
    {"if", "if"}, {"else", "else"}, {"elif", "elif"},
    {"for", "for"}, {"while", "while"}, {"in", "in"},
    {"return", "return"}, {"break", "break"}, {"continue", "continue"},
    {"true", "boolean"}, {"false", "boolean"}, {"none", "none"},
    {"import", "import"}, {"and", "and"}, {"or", "or"}, {"not", "not"},
    {"yield", "yield"}, {"loop", "loop"}, {"static", "static"},
    {"enum", "enum"}
};

}
