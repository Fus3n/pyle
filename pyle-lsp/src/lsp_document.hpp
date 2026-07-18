#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_parser.hpp"
#include "lsp_transport.hpp"
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>

namespace fs = std::filesystem;

namespace pyle::lsp {

class DocumentManager {
    std::map<std::string, Document> docs_;
    std::map<std::string, std::string> import_cache_;

public:
    std::string root_path;
    std::vector<std::string> default_import_paths = {"."};
    using DocMap = std::map<std::string, Document>;
    const DocMap& all() const { return docs_; }

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

        Document type_doc = docs_[type_file];
        if (type_doc.file_path.empty()) {
            std::string type_src = pyle::lsp::utils::read_file_contents(type_file);
            if (type_src.empty()) {
                doc.type_decls_loaded = true;
                return;
            }
            type_doc.file_path = type_file;
            type_doc.source = type_src;
            type_doc.reporter = pyle::ErrorReporter(type_src, type_file);
            FuzzyParser::parse(type_doc);
            docs_[type_file] = type_doc;
        }

        for (const auto& s : type_doc.symbols) {
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

    void open(const std::string& file_path, const std::string& source) {
        Document doc;
        doc.file_path = file_path;
        doc.source = source;
        doc.reporter = pyle::ErrorReporter(source, file_path);
        FuzzyParser::parse(doc);

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
            FuzzyParser::parse(it->second);
            send_diagnostics(it->second);
        } else {
            open(file_path, source);
        }
    }

    void close(const std::string& file_path) {
        docs_.erase(file_path);
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
                            // Simple constructor call like "Enin()" — strip parens
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
    void send_diagnostics(Document& doc) {
        json arr = json::array();
        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, 
            {"method", "textDocument/publishDiagnostics"},
            {"params", pyle::lsp::utils::make_obj({
                {"uri", pyle::lsp::utils::path_to_file_uri(doc.file_path)},
                {"diagnostics", arr}
            })}
        }).dump());
    }
};

} // namespace pyle::lsp