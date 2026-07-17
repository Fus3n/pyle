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
    using DocMap = std::map<std::string, Document>;
    const DocMap& all() const { return docs_; }

    void open(const std::string& file_path, const std::string& source) {
        Document doc;
        doc.file_path = file_path;
        doc.source = source;
        doc.reporter = pyle::ErrorReporter(source, file_path);
        FuzzyParser::parse(doc);
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

    // Resolves an import accurately using relative heuristics and upward scanning.
    std::string resolve_import(const std::string& current_file, const std::string& import_path, const std::vector<std::string>& extra_paths) {
        std::string cache_key = current_file + "|" + import_path;
        for (const auto& ep : extra_paths) cache_key += "|" + ep;
        
        auto it = import_cache_.find(cache_key);
        if (it != import_cache_.end()) return it->second;

        fs::path base_dir = fs::path(current_file).parent_path();
        std::vector<std::string> candidates;
        
        // 1. Explicit import relative to the current file's folder
        candidates.push_back((base_dir / (import_path + ".pyl")).string());
        candidates.push_back((base_dir / import_path).string());
        candidates.push_back((base_dir / (import_path + ".pyle")).string());

        // 2. Extra explicit include paths (like add_import_path("vm_verify"))
        for (const auto& ep : extra_paths) {
            fs::path ep_path(ep);
            if (!ep_path.is_absolute()) {
                ep_path = base_dir / ep_path;
            }
            candidates.push_back((ep_path / (import_path + ".pyl")).string());
            candidates.push_back((ep_path / import_path).string());
        }

        // 3. Heuristic: Automatically walk up the folder tree to resolve imports (e.g., vn/ui -> root/vn/ui.pyl)
        fs::path current_walk = base_dir;
        while (current_walk.has_parent_path() && current_walk != current_walk.parent_path()) {
            current_walk = current_walk.parent_path();
            candidates.push_back((current_walk / (import_path + ".pyl")).string());
            candidates.push_back((current_walk / import_path).string());
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

    // Lazily evaluate a type chain recursively (vn.get_theme(). -> returns 'VNTheme')
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
                            current_type = ""; // Trigger fallback module check
                        } else if (sym.type_name.find('.') != std::string::npos || sym.type_name.find('(') != std::string::npos) {
                            current_type = resolve_type_of_chain(file, sym.type_name, current_func, current_struct, depth + 1);
                        } else {
                            current_type = sym.type_name;
                        }
                    }
                    break;
                }
            }

            // Check if parts[0] is an imported module name (ex: 'engine')
            if (current_type.empty()) {
                for (const auto& [var_name, mod_path] : d->imports) {
                    if (var_name == parts[0]) {
                        if (parts.size() > 1) {
                            std::string resolved = resolve_import(file, mod_path, d->import_paths);
                            if (!resolved.empty()) {
                                Document* mod_doc = get(resolved);
                                if (mod_doc) {
                                    // Strip trailing call parenthesis to fetch structural types (e.g. Engine() -> Engine)
                                    std::string p1_clean = parts[1];
                                    if (p1_clean.size() > 2 && p1_clean.substr(p1_clean.size() - 2) == "()") {
                                        p1_clean = p1_clean.substr(0, p1_clean.size() - 2);
                                    }
                                    
                                    for (const auto& s : mod_doc->symbols) {
                                        if (s.name == p1_clean && s.kind == SymbolKind::Struct) {
                                            current_type = s.name;
                                            start_idx = 2;
                                            break;
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

        // Trace the nested properties down the chain
        for (size_t i = start_idx; i < parts.size(); ++i) {
            std::string member = parts[i];
            bool is_call = false;
            if (member.size() > 2 && member.substr(member.size() - 2) == "()") {
                member = member.substr(0, member.size() - 2);
                is_call = true;
            }

            if (current_type.empty()) return "";

            std::string member_type_raw = "";
            std::string member_file = file;

            auto find_member = [&](const Document& doc) -> bool {
                for (const auto& sym : doc.symbols) {
                    if (sym.parent_struct == current_type && sym.name == member) {
                        member_type_raw = sym.type_name;
                        member_file = doc.file_path;
                        return true;
                    }
                }
                return false;
            };

            if (!find_member(*d)) {
                for (const auto& [var_name, mod_path] : d->imports) {
                    std::string resolved = resolve_import(file, mod_path, d->import_paths);
                    if (!resolved.empty()) {
                        Document* mod_doc = get(resolved);
                        if (mod_doc && find_member(*mod_doc)) break;
                    }
                }
            }

            if (member_type_raw.empty()) return "";

            // Evaluate the raw member type expression relative to the file and struct where it is defined!
            current_type = resolve_type_of_chain(member_file, member_type_raw, "", current_type, depth + 1);
        }

        // Always return sanitized structural names
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