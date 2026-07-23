#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_transport.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <pyle/lexer.hpp>
#include <pyle/parser.hpp>
#include <pyle/ast.hpp>

namespace fs = std::filesystem;

namespace pyle::lsp {

class DocumentManager {
    std::map<std::string, Document> docs_;
    std::map<std::string, std::string> import_cache_;

public:
    std::string root_path;
    std::vector<std::string> default_import_paths;
    std::function<void(const std::string&)> on_diagnostics;

    using DocMap = std::map<std::string, Document>;
    const DocMap& all() const { return docs_; }

    DocumentManager() = default;

    std::string find_type_decl_file(const std::string& file_path) {
        fs::path p(file_path);
        std::string stem = p.stem().string();
        fs::path parent = p.parent_path();

        std::vector<fs::path> candidates;
        candidates.push_back(parent / (stem + ".pyl.d"));
        candidates.push_back(parent / "types" / (stem + ".pyl.d"));

        for (const auto& dp : default_import_paths) {
            fs::path dp_path(dp);
            if (dp_path.is_absolute()) {
                candidates.push_back(dp_path / (stem + ".pyl.d"));
                candidates.push_back(dp_path / "types" / (stem + ".pyl.d"));
            }
        }

        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                return pyle::lsp::utils::normalize_path(c.string());
            }
        }
        return "";
    }

    void load_type_declarations(Document& doc) {
        if (doc.type_decls_loaded) return;

        std::string type_file = find_type_decl_file(doc.file_path);
        if (type_file.empty()) {
            doc.type_decls_loaded = true;
            return;
        }

        Document* type_doc = &docs_[type_file];
        if (type_doc->file_path.empty()) {
            std::string type_src = pyle::lsp::utils::read_file_contents(type_file);
            if (type_src.empty()) {
                doc.type_decls_loaded = true;
                return;
            }
            type_doc->file_path = type_file;
            type_doc->source = type_src;
            type_doc->reporter = pyle::ErrorReporter(type_src, type_file);
            parse_document(*type_doc);
        }

        for (const auto& s : type_doc->symbols) {
            bool exists = false;
            for (auto& existing : doc.symbols) {
                if (existing.name == s.name && existing.parent_struct == s.parent_struct && existing.kind == s.kind) {
                    existing = s;
                    existing.has_type_hint = true;
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                auto copy = s;
                copy.has_type_hint = true;
                doc.symbols.push_back(copy);
            }
        }

        doc.type_decls_loaded = true;
    }

    bool has_suffix(const std::string& str, const std::string& suffix) const {
        return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    static std::string expr_to_chain_string(const pyle::Expr* expr) {
        if (!expr) return "";
        if (auto* lit = dynamic_cast<const pyle::LiteralExpr*>(expr)) {
            return std::string(lit->token.lexeme);
        }
        if (auto* var = dynamic_cast<const pyle::VariableExpr*>(expr)) {
            return std::string(var->name.lexeme);
        }
        if (auto* mc = dynamic_cast<const pyle::MethodCallExpr*>(expr)) {
            return expr_to_chain_string(mc->callee.get()) + "." + std::string(mc->method_name.lexeme) + "()";
        }
        if (auto* gf = dynamic_cast<const pyle::GetFieldExpr*>(expr)) {
            return expr_to_chain_string(gf->obj.get()) + "." + std::string(gf->name.lexeme);
        }
        if (auto* call = dynamic_cast<const pyle::CallExpr*>(expr)) {
            return expr_to_chain_string(call->callee.get()) + "()";
        }
        if (dynamic_cast<const pyle::ArrayExpr*>(expr)) return "[]";
        if (dynamic_cast<const pyle::MapExpr*>(expr)) return "{}";
        if (auto* bin = dynamic_cast<const pyle::BinaryExpr*>(expr)) {
            return expr_to_chain_string(bin->left.get()) + "." + expr_to_chain_string(bin->right.get());
        }
        return "";
    }

    void parse_document(Document& doc) {
        doc.symbols.clear();
        doc.imports.clear();
        doc.import_paths.clear();
        doc.ast.clear();

        pyle::Lexer lexer(doc.source, doc.reporter);
        auto tokens = lexer.tokenize();

        pyle::Parser parser(tokens, doc.reporter);
        doc.ast = parser.parse();

        std::string current_func = "";
        std::string current_struct = "";
        std::vector<std::pair<std::string, size_t>> func_stack; // {func_name, symbol_index}
        std::vector<std::pair<std::string, size_t>> struct_stack; // {struct_name, symbol_index}

        auto resolve_init_type = [&](const pyle::Expr* init, const std::string& type_hint) -> std::string {
            if (!type_hint.empty()) return type_hint;
            if (!init) return "";
            
            std::string chain = expr_to_chain_string(init);
            if (chain.empty()) {
                if (auto* lit = dynamic_cast<const pyle::LiteralExpr*>(init)) {
                    const auto& lex = lit->token.lexeme;
                    if (!lex.empty()) {
                        char fc = lex[0];
                        if ((fc == '"' || fc == '\'') && lex.back() == fc) return "string";
                        if (fc == '[') return "array";
                        if (fc == '{') return "map";
                        if (lex == "true" || lex == "false") return "bool";
                        bool is_num = true, is_float = false;
                        for (size_t i = 0; i < lex.size() && is_num; ++i) {
                            char c = lex[i];
                            if (i == 0 && c == '-') continue;
                            if (c == '.') { is_float = true; continue; }
                            if (!isdigit(static_cast<unsigned char>(c))) is_num = false;
                        }
                        if (is_num) return is_float ? "float" : "int";
                    }
                }
                if (auto* arr = dynamic_cast<const pyle::ArrayExpr*>(init)) return "array";
                if (auto* map = dynamic_cast<const pyle::MapExpr*>(init)) return "map";
                return "";
            }
            
            if (auto* call = dynamic_cast<const pyle::CallExpr*>(init)) {
                if (auto* callee_var = dynamic_cast<const pyle::VariableExpr*>(call->callee.get())) {
                    if (callee_var->name.lexeme == "import" && !call->args.empty()) {
                        return "import()";
                    }
                }
            }
            
            return chain;
        };

        for (const auto& stmt : doc.ast) {
            if (auto* block = dynamic_cast<pyle::BlockStmt*>(stmt.get())) {
                for (const auto& bstmt : block->statements) {
                }
            }

            if (auto* fd = dynamic_cast<pyle::FuncDeclStmt*>(stmt.get())) {
                std::string func_name(fd->name.lexeme);
                std::string detail = "fn " + func_name + "(";
                for (size_t pi = 0; pi < fd->params.size(); ++pi) {
                    if (pi > 0) detail += ", ";
                    detail += std::string(fd->params[pi].lexeme);
                    if (pi < fd->param_types.size() && !fd->param_types[pi].empty()) {
                        detail += ": " + fd->param_types[pi];
                    }
                }
                detail += ")";
                if (!fd->return_type.empty()) detail += ": " + fd->return_type;

                bool is_method = !current_struct.empty();
                
                SymbolInfo sym;
                sym.name = func_name;
                sym.kind = is_method ? SymbolKind::Field : SymbolKind::Function;
                sym.detail = detail;
                sym.file_path = doc.file_path;
                sym.range = {{fd->name.selection.line, fd->name.selection.column},
                            {fd->name.selection.line + 500, 0}};
                sym.selection_range = {{fd->name.selection.line, fd->name.selection.column},
                                      {fd->name.selection.line, fd->name.selection.column + func_name.size()}};
                sym.type_name = fd->return_type;
                sym.is_method = is_method;
                sym.parent_struct = current_struct;
                
                size_t sym_idx = doc.symbols.size();
                doc.symbols.push_back(sym);
                
                std::string prev_func = current_func;
                current_func = func_name;
                func_stack.push_back({func_name, sym_idx});
                
                if (fd->body) {
                    for (const auto& body_stmt : fd->body->statements) {
                        if (auto* ret = dynamic_cast<pyle::ReturnStmt*>(body_stmt.get())) {
                            if (ret->value) {
                                std::string ret_chain = expr_to_chain_string(ret->value.get());
                                if (!ret_chain.empty()) {
                                    doc.symbols[sym_idx].type_name = ret_chain;
                                }
                            }
                        }
                    }
                }
                
                current_func = prev_func;
                func_stack.pop_back();
            }
            else if (auto* sd = dynamic_cast<pyle::StructDeclStmt*>(stmt.get())) {
                std::string struct_name(sd->name.lexeme);
                
                SymbolInfo sym;
                sym.name = struct_name;
                sym.kind = SymbolKind::Struct;
                sym.detail = "struct " + struct_name;
                sym.file_path = doc.file_path;
                sym.range = {{sd->name.selection.line, sd->name.selection.column},
                            {sd->name.selection.line + 1000, 0}};
                sym.selection_range = {{sd->name.selection.line, sd->name.selection.column},
                                      {sd->name.selection.line, sd->name.selection.column + struct_name.size()}};
                
                size_t struct_sym_idx = doc.symbols.size();
                doc.symbols.push_back(sym);
                
                for (size_t fi = 0; fi < sd->fields.size(); ++fi) {
                    std::string field_name(sd->fields[fi].lexeme);
                    std::string field_type_name = "";
                    if (fi < sd->field_types.size()) {
                        field_type_name = sd->field_types[fi];
                    }
                    
                    SymbolInfo field_sym;
                    field_sym.name = field_name;
                    field_sym.kind = SymbolKind::Field;
                    field_sym.detail = "self." + field_name;
                    field_sym.file_path = doc.file_path;
                    field_sym.parent_struct = struct_name;
                    field_sym.type_name = field_type_name;
                    field_sym.has_type_hint = !field_type_name.empty();
                    doc.symbols.push_back(field_sym);
                }
                
                std::string prev_struct = current_struct;
                current_struct = struct_name;
                struct_stack.push_back({struct_name, struct_sym_idx});
                
                for (const auto& method : sd->methods) {
                    std::string m_name(method->name.lexeme);
                    std::string m_detail = "fn " + m_name + "(";
                    for (size_t pi = 0; pi < method->params.size(); ++pi) {
                        if (pi > 0) m_detail += ", ";
                        m_detail += std::string(method->params[pi].lexeme);
                        if (pi < method->param_types.size() && !method->param_types[pi].empty()) {
                            m_detail += ": " + method->param_types[pi];
                        }
                    }
                    m_detail += ")";
                    if (!method->return_type.empty()) m_detail += ": " + method->return_type;

                    SymbolInfo msym;
                    msym.name = m_name;
                    msym.kind = SymbolKind::Field;
                    msym.detail = m_detail;
                    msym.file_path = doc.file_path;
                    msym.is_method = true;
                    msym.parent_struct = struct_name;
                    msym.type_name = method->return_type;
                    doc.symbols.push_back(msym);
                }
                
                current_struct = prev_struct;
                struct_stack.pop_back();
            }
            else if (auto* vd = dynamic_cast<pyle::VarDeclStmt*>(stmt.get())) {
                std::string var_name(vd->name.lexeme);
                std::string init_type = resolve_init_type(vd->initializer.get(), vd->type_annotation);
                
                bool is_import = false;
                std::string import_val = "";
                if (vd->initializer) {
                    if (auto* call = dynamic_cast<pyle::CallExpr*>(vd->initializer.get())) {
                        if (auto* callee_var = dynamic_cast<pyle::VariableExpr*>(call->callee.get())) {
                            if (callee_var->name.lexeme == "import" && !call->args.empty()) {
                                if (auto* arg_lit = dynamic_cast<pyle::LiteralExpr*>(call->args[0].get())) {
                                    is_import = true;
                                    import_val = pyle::lsp::utils::strip_quotes(std::string(arg_lit->token.lexeme));
                                }
                            }
                        }
                    }
                }

                SymbolInfo v_info;
                v_info.name = var_name;
                v_info.file_path = doc.file_path;
                v_info.range = {{vd->name.selection.line, vd->name.selection.column},
                                {vd->name.selection.line, vd->name.selection.column + var_name.size()}};
                v_info.selection_range = v_info.range;
                v_info.is_local = !current_func.empty();
                v_info.scope_func = current_func;

                if (is_import) {
                    v_info.kind = SymbolKind::Module;
                    v_info.detail = "import(\"" + import_val + "\")";
                    v_info.type_name = "import()";
                    doc.imports[var_name] = import_val;
                } else {
                    v_info.kind = SymbolKind::Variable;
                    v_info.detail = "let " + var_name;
                    v_info.type_name = init_type;
                    v_info.has_type_hint = !vd->type_annotation.empty();
                }
                doc.symbols.push_back(v_info);
            }
            else if (auto* es = dynamic_cast<pyle::ExpressionStmt*>(stmt.get())) {
                if (auto* sf = dynamic_cast<pyle::SetFieldExpr*>(es->expression.get())) {
                    if (auto* obj_var = dynamic_cast<pyle::VariableExpr*>(sf->obj.get())) {
                        if (obj_var->name.lexeme == "self" && !current_struct.empty()) {
                            std::string field_name(sf->name.lexeme);
                            std::string val_chain = expr_to_chain_string(sf->value.get());
                            
                            bool found = false;
                            for (auto& sym : doc.symbols) {
                                if (sym.kind == SymbolKind::Field && sym.parent_struct == current_struct && sym.name == field_name) {
                                    if (!sym.has_type_hint && !val_chain.empty()) {
                                        sym.type_name = val_chain;
                                    }
                                    found = true;
                                    break;
                                }
                            }
                            if (!found) {
                                SymbolInfo f_info;
                                f_info.name = field_name;
                                f_info.kind = SymbolKind::Field;
                                f_info.detail = "self." + field_name;
                                f_info.file_path = doc.file_path;
                                f_info.parent_struct = current_struct;
                                f_info.type_name = val_chain;
                                doc.symbols.push_back(f_info);
                            }
                        }
                    }
                }
                else if (auto* assign = dynamic_cast<pyle::AssignExpr*>(es->expression.get())) {
                    if (auto* call = dynamic_cast<pyle::CallExpr*>(assign->value.get())) {
                        if (auto* callee_var = dynamic_cast<pyle::VariableExpr*>(call->callee.get())) {
                            if (callee_var->name.lexeme == "add_import_path" && !call->args.empty()) {
                                if (auto* arg_lit = dynamic_cast<pyle::LiteralExpr*>(call->args[0].get())) {
                                    doc.import_paths.push_back(pyle::lsp::utils::strip_quotes(std::string(arg_lit->token.lexeme)));
                                }
                            }
                        }
                    }
                }
                else if (auto* call = dynamic_cast<pyle::CallExpr*>(es->expression.get())) {
                    if (auto* callee_var = dynamic_cast<pyle::VariableExpr*>(call->callee.get())) {
                        if (callee_var->name.lexeme == "add_import_path" && !call->args.empty()) {
                            if (auto* arg_lit = dynamic_cast<pyle::LiteralExpr*>(call->args[0].get())) {
                                doc.import_paths.push_back(pyle::lsp::utils::strip_quotes(std::string(arg_lit->token.lexeme)));
                            }
                        }
                    }
                }
            }
        }
    }

     std::string extract_chain_backwards(const std::vector<pyle::Token>& tokens, int end_idx) const {
        if (end_idx < 0 || end_idx >= (int)tokens.size()) return "";
        std::string chain = "";
        int paren_depth = 0, bracket_depth = 0, brace_depth = 0;
        
        for (int i = end_idx; i >= 0; --i) {
            const auto& t = tokens[i];
            
            if (t.type == pyle::TokenType::RIGHT_PAREN) paren_depth++;
            else if (t.type == pyle::TokenType::LEFT_PAREN) paren_depth--;
            else if (t.type == pyle::TokenType::RIGHT_BRACKET) bracket_depth++;
            else if (t.type == pyle::TokenType::LEFT_BRACKET) bracket_depth--;
            else if (t.type == pyle::TokenType::RIGHT_BRACE) brace_depth++;
            else if (t.type == pyle::TokenType::LEFT_BRACE) brace_depth--;
            
            if (paren_depth < 0 || bracket_depth < 0 || brace_depth < 0) {
                break;
            }

            chain = std::string(t.lexeme) + chain;
            
            if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0) {
                if (i > 0) {
                    auto prev = tokens[i-1].type;
                    if (prev != pyle::TokenType::DOT && 
                        prev != pyle::TokenType::IDENTIFIER && 
                        prev != pyle::TokenType::RIGHT_PAREN && 
                        prev != pyle::TokenType::RIGHT_BRACKET) {
                        break;
                    }
                }
            }
        }
        return chain;
    }

    void open(const std::string& file_path, const std::string& source) {
        Document doc;
        doc.file_path = file_path;
        doc.source = source;
        doc.reporter = pyle::ErrorReporter(source, file_path);
        parse_document(doc);

        if (has_suffix(file_path, ".pyled")) {
            load_type_declarations(doc);
        }

        docs_[file_path] = std::move(doc);
        send_diagnostics(docs_[file_path]);
    }

     void change(const std::string& file_path, const std::string& source) {
        auto it = docs_.find(file_path);
        if (it != docs_.end()) {
            it->second.source = source;
            it->second.reporter = pyle::ErrorReporter(source, file_path);
            parse_document(it->second);
            send_diagnostics(it->second);
        } else {
            open(file_path, source);
        }
    }


    void close(const std::string& file_path) {
        docs_.erase(file_path);
    }

    void open_preloaded(const std::string& file_path, const std::string& source) {
        Document doc;
        doc.file_path = file_path;
        doc.source = source;
        doc.reporter = pyle::ErrorReporter(source, file_path);
        parse_document(doc);
        docs_[file_path] = std::move(doc);
    }

    void preload_core_types(const std::vector<std::string>& search_paths) {
        for (const auto& sp : search_paths) {
            fs::path core_dir = fs::path(sp) / "core";
            std::error_code ec;
            if (!fs::is_directory(core_dir, ec)) continue;
            for (const auto& entry : fs::directory_iterator(core_dir, ec)) {
                auto ext = entry.path().extension().string();
                if (ext != ".pyl.d") continue;
                std::string fp = pyle::lsp::utils::normalize_path(entry.path().string());
                if (docs_.find(fp) != docs_.end()) continue;
                std::string src = pyle::lsp::utils::read_file_contents(fp);
                if (src.empty()) continue;
                open_preloaded(fp, src);
            }
        }
    }

    Document* get(const std::string& file_path) {
        auto it = docs_.find(file_path);
        if (it != docs_.end()) return &it->second;

        if (has_suffix(file_path, ".pyled")) {
            Document doc;
            doc.file_path = file_path;
            doc.source = "";
            doc.reporter = pyle::ErrorReporter("", file_path);
            load_type_declarations(doc);
            if (doc.symbols.empty()) return nullptr;
            docs_[file_path] = std::move(doc);
            return &docs_[file_path];
        }

        std::string src = pyle::lsp::utils::read_file_contents(file_path);
        if (src.empty()) return nullptr;
        open(file_path, src);
        return &docs_[file_path];
    }

    std::string get_word_at(const std::string& source, Position pos) {
        auto lines = pyle::lsp::utils::get_lines(source);
        if (pos.line >= lines.size()) return "";
        std::string line = lines[pos.line];
        if (pos.character > line.size()) return "";
        
        size_t start = pos.character;
        while (start > 0 && (isalnum(static_cast<unsigned char>(line[start-1])) || line[start-1] == '_')) start--;
        
        size_t end = pos.character;
        while (end < line.size() && (isalnum(static_cast<unsigned char>(line[end])) || line[end] == '_')) end++;
        
        return line.substr(start, end - start);
    }
    
    std::string resolve_import(const std::string& current_file, const std::string& import_path, const std::vector<std::string>& extra_paths) {
        std::string cache_key = current_file + "|" + import_path;
        for (const auto& ep : extra_paths) cache_key += "|" + ep;
        for (const auto& dp : default_import_paths) cache_key += "|" + dp;
        cache_key += "|" + root_path;
        
        auto it = import_cache_.find(cache_key);
        if (it != import_cache_.end()) return it->second;

        auto try_extensions = [&](const fs::path& dir) -> std::vector<std::string> {
            static const std::vector<std::string> exts = {".pyl", ".pyle", ".pyled", ".pyl.d"};
            std::vector<std::string> result;
            for (const auto& ext : exts) {
                result.push_back((dir / (import_path + ext)).string());
            }
            result.push_back((dir / import_path).string());
            return result;
        };

        std::vector<std::string> candidates;
        fs::path base_dir = fs::path(current_file).parent_path();

        auto add = [&](const fs::path& dir) {
            auto c = try_extensions(dir);
            candidates.insert(candidates.end(), c.begin(), c.end());
        };

        add(base_dir);

        for (const auto& ep : extra_paths) {
            fs::path ep_path(ep);
            if (!ep_path.is_absolute()) {
                ep_path = base_dir / ep_path;
            }
            add(ep_path);
        }

        for (const auto& dp : default_import_paths) {
            fs::path dp_path(dp);
            if (!dp_path.is_absolute()) dp_path = base_dir / dp_path;
            add(dp_path);
        }

        if (!root_path.empty()) {
            fs::path rp(root_path);
            for (const auto& dp : default_import_paths) {
                add(rp / dp);
            }
        }

        fs::path current_walk = base_dir;
        while (current_walk.has_parent_path() && current_walk != current_walk.parent_path()) {
            current_walk = current_walk.parent_path();
            add(current_walk);
        }
        
        std::string resolved = "";
        for (const auto& c : candidates) {
            if (fs::exists(c)) {
                resolved = pyle::lsp::utils::normalize_path(c);
                break;
            }
        }
        import_cache_[cache_key] = resolved;
        return resolved;
    }

    std::string resolve_type_of_chain(const std::string& file, const std::string& chain, const std::string& current_func, const std::string& current_struct, int depth = 0) {
        if (depth > 5 || chain.empty()) return ""; 

        std::vector<std::string> parts;
        std::stringstream ss(chain);
        std::string part;
        while (std::getline(ss, part, '.')) {
            if (!part.empty()) parts.push_back(part);
        }
        if (parts.empty()) return "";

        Document* d = get(file);
        if (!d) return "";

        std::string current_type = "";
        size_t start_idx = 0;

        if (parts[0] == "self") {
            current_type = current_struct;
            start_idx = 1;
        } else {
            // Find base variable
            for (const auto& sym : d->symbols) {
                if (sym.name == parts[0]) {
                    if (sym.is_local && sym.scope_func != current_func) continue;
                    if (!sym.type_name.empty()) {
                        if (sym.type_name.rfind("import()", 0) == 0) {
                            current_type = ""; 
                        } else if (sym.type_name.size() > 2 && sym.type_name.substr(sym.type_name.size() - 2) == "()" && sym.type_name.find('.') == std::string::npos) {
                            current_type = sym.type_name.substr(0, sym.type_name.size() - 2);
                        } else if (sym.type_name.find('.') != std::string::npos || sym.type_name.find('(') != std::string::npos) {
                            current_type = resolve_type_of_chain(file, sym.type_name, current_func, current_struct, depth + 1);
                        } else {
                            current_type = sym.type_name;
                        }
                    }
                    break;
                }
            }

            // Infer type from literal expression if symbol not found
            if (current_type.empty() && !parts[0].empty()) {
                char fc = parts[0][0];
                if ((fc == '"' || fc == '\'') && parts[0].back() == fc) {
                    current_type = "string";
                } else if (fc == '[') {
                    current_type = "array";
                } else if (fc == '{') {
                    current_type = "map";
                } else if (parts[0] == "true" || parts[0] == "false") {
                    current_type = "bool";
                } else {
                    bool is_num = true, is_float = false;
                    for (size_t ci = 0; ci < parts[0].size() && is_num; ++ci) {
                        char c = parts[0][ci];
                        if (ci == 0 && c == '-') continue;
                        if (c == '.') { is_float = true; continue; }
                        if (!isdigit(static_cast<unsigned char>(c))) is_num = false;
                    }
                    if (is_num) current_type = is_float ? "float" : "int";
                }
            }

            if (current_type.empty()) {
                for (const auto& [var_name, mod_path] : d->imports) {
                    if (var_name == parts[0]) {
                        if (parts.size() > 1) {
                            std::string resolved = resolve_import(file, mod_path, d->import_paths);
                            if (!resolved.empty()) {
                                Document* mod_doc = get(resolved);
                                if (mod_doc) {
                                    std::string p1_clean = parts[1];
                                    if (p1_clean.size() > 2 && p1_clean.substr(p1_clean.size() - 2) == "()") {
                                        p1_clean = p1_clean.substr(0, p1_clean.size() - 2);
                                    }
                                    
                                    for (const auto& s : mod_doc->symbols) {
                                        if (s.name == p1_clean) {
                                            if (s.kind == SymbolKind::Struct) {
                                                current_type = s.name;
                                                start_idx = 2;
                                                break;
                                            } else if ((s.is_method || s.kind == SymbolKind::Function) && !s.type_name.empty()) {
                                                current_type = resolve_type_of_chain(resolved, s.type_name, "", s.parent_struct, depth + 1);
                                                start_idx = 2;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
            }

            if (current_type.empty()) {
                start_idx = 1;
            } else if (start_idx == 0) {
                start_idx = 1;
            }
        }

        for (size_t i = start_idx; i < parts.size(); ++i) {
            std::string member = parts[i];
            bool is_call = false;
            if (member.size() > 2 && member.substr(member.size() - 2) == "()") {
                member = member.substr(0, member.size() - 2);
                is_call = true;
            }
            auto paren_pos = member.find('(');
            if (paren_pos != std::string::npos) {
                member = member.substr(0, paren_pos);
                is_call = true;
            }

            if (current_type.empty()) {
                return "";
            }

            std::string member_type_raw = "";
            std::string member_file = file;

            auto find_member_doc = [&](const Document& doc) -> bool {
                for (const auto& sym : doc.symbols) {
                    if (sym.parent_struct == current_type && sym.name == member) {
                        member_type_raw = sym.type_name;
                        member_file = doc.file_path;
                        return true;
                    }
                }
                return false;
            };

            if (!find_member_doc(*d)) {
                bool found_in_import = false;
                for (const auto& [var_name, mod_path] : d->imports) {
                    std::string resolved = resolve_import(file, mod_path, d->import_paths);
                    if (!resolved.empty()) {
                        Document* mod_doc = get(resolved);
                        if (mod_doc && find_member_doc(*mod_doc)) {
                            found_in_import = true;
                            break;
                        }
                    }
                }
                if (!found_in_import) {
                    return "";
                }
            }

            if (member_type_raw.empty()) {
                return "";
            }

            if (member_type_raw.size() > 2 && member_type_raw.substr(member_type_raw.size() - 2) == "()" && member_type_raw.find('.') == std::string::npos) {
                current_type = member_type_raw.substr(0, member_type_raw.size() - 2);
            } else if (member_type_raw.find('.') == std::string::npos && member_type_raw.find('(') == std::string::npos) {
                SymbolInfo* sym = sym_by_name(member_file, member_type_raw, Position{0, 0});
                if (sym) {
                    if (sym->kind == SymbolKind::Struct) {
                        current_type = sym->name;
                    } else if (!sym->type_name.empty()) {
                        current_type = resolve_type_of_chain(member_file, sym->type_name, "", current_type, depth + 1);
                    } else {
                        return "";
                    }
                } else {
                    return "";
                }
            } else {
                current_type = resolve_type_of_chain(member_file, member_type_raw, "", current_type, depth + 1);
            }
        }

        std::string result = current_type;
        if (result.size() > 2 && result.substr(result.size() - 2) == "()") {
            result = result.substr(0, result.size() - 2);
        }
        return result;
    }

    SymbolInfo* sym_by_name(const std::string& file, const std::string& name, Position pos) {
        if (name.empty()) return nullptr;
        Document* d = get(file);
        if (!d) return nullptr;
        
        std::string current_func = "";
        for (const auto& s : d->symbols) {
            if ((s.kind == SymbolKind::Function || s.is_method) && pos.line >= s.range.start.line && pos.line <= s.range.end.line) {
                current_func = s.name;
            }
        }

        SymbolInfo* best_match = nullptr;
        for (auto& s : d->symbols) {
            if (s.name == name) {
                if (s.is_local && s.scope_func != current_func) continue;
                if (!best_match) {
                    best_match = &s;
                } else if (s.is_local && !best_match->is_local) {
                    best_match = &s;
                }
            }
        }
        if (best_match) return best_match;
        
        for (const auto& [var_name, mod_path] : d->imports) {
            std::string resolved = resolve_import(file, mod_path, d->import_paths);
            if (resolved.empty()) continue;
            Document* mod_doc = get(resolved);
            if (mod_doc) {
                for (auto& s : mod_doc->symbols) {
                    if (s.name == name && !s.is_local && s.name.front() != '_') return &s;
                }
            }
        }
        return nullptr;
    }

    std::vector<SymbolInfo> fields_of(const std::string& file, const std::string& struct_name, bool include_private) {
        Document* d = get(file);
        if (!d) return {};

        std::vector<SymbolInfo> out;
        auto search_doc = [&](const Document& doc) {
            for (const auto& s : doc.symbols) {
                if ((s.kind == SymbolKind::Field || s.is_method) && s.parent_struct == struct_name) {
                    if (s.name.empty() || (!include_private && s.name[0] == '_')) continue;
                    out.push_back(s);
                }
            }
        };

        search_doc(*d);
        if (!out.empty()) return out;

        for (const auto& [var_name, mod_path] : d->imports) {
            std::string resolved = resolve_import(file, mod_path, d->import_paths);
            if (resolved.empty()) continue;
            Document* mod_doc = get(resolved);
            if (mod_doc) search_doc(*mod_doc);
            if (!out.empty()) return out;
        }

        for (const auto& [fp, doc] : docs_) {
            if (&doc == d) continue;
            search_doc(doc);
            if (!out.empty()) return out;
        }
        return out;
    }

private:
    private:
        void send_diagnostics(Document& doc) {
            if (on_diagnostics) {
                on_diagnostics(doc.file_path);
            }
        }
    };
};
