#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_analyzer.hpp"

#include <map>
#include <set>
#include <filesystem>
#include <algorithm>
#include <cctype>

namespace fs = std::filesystem;

namespace pyle::lsp {

inline void debug_log(const std::string& s) {
    if (getenv("PYLE_LSP_DEBUG")) fprintf(stderr, "[pyle-lsp] %s\n", s.c_str());
}

class ModuleResolver {
public:
    void set_std_paths(const std::vector<std::string>& paths) {
        std_paths = paths;
        for (auto& p : std_paths) {
            try { p = fs::weakly_canonical(p).string(); } catch (...) {}
        }
    }

    bool add_std_path(const std::string& path) {
        std::string canon;
        try { canon = fs::weakly_canonical(path).string(); } catch (...) { canon = path; }
        std::error_code ec;
        if (!fs::exists(canon, ec) || ec) return false;
        for (const auto& p : std_paths) {
            if (utils::has_prefix(p, canon) || utils::has_prefix(canon, p)) return false;
        }
        std_paths.push_back(canon);
        return true;
    }

    const std::vector<std::string>& get_std_paths() const { return std_paths; }

    void set_workspace_root(const std::string& root) {
        workspace_root = root;
        try {
            if (!workspace_root.empty()) workspace_root = fs::weakly_canonical(workspace_root).string();
        } catch (...) {}
    }

    const std::string& get_workspace_root() const { return workspace_root; }

    void preload_std() {
        for (const auto& dir : std_paths) {
            for (const auto& core : {fs::path(dir) / "core", fs::path(dir)}) {
        std::error_code ec;
        if (!fs::exists(core, ec) || ec) continue;
        for (const auto& entry : fs::directory_iterator(core, ec)) {
            if (ec) break;
            if (!entry.is_regular_file(ec)) continue;
            std::string fn = entry.path().filename().string();
            if (utils::has_suffix(fn, ".pyl.d") || utils::has_suffix(fn, ".pyl")) {
                load_file(entry.path().string(), true);
            }
        }
            }
        }
    }

    bool std_available() const {
        for (const auto& dir : std_paths) {
            std::error_code ec;
            if (fs::exists(fs::path(dir) / "core", ec) && !ec) return true;
        }
        return false;
    }

    std::string find_module_file(const std::string& module_name, const std::string& from_file) {
        std::string name = module_name;
        if (name.empty()) return "";
        std::replace(name.begin(), name.end(), '\\', '/');
        auto candidates = module_candidates(name, from_file);
        if (name.find('/') == std::string::npos) {
            auto extra = module_candidates("core/" + name, from_file);
            candidates.insert(candidates.end(), extra.begin(), extra.end());
        }
        std::set<std::string> seen;
        for (const auto& c : candidates) {
            std::string canon;
            try { canon = fs::weakly_canonical(c).string(); } catch (...) { continue; }
            if (!seen.insert(canon).second) continue;
            std::error_code ec;
            if (fs::is_regular_file(canon, ec) && !ec) return canon;
        }
        return "";
    }

    DocumentModel* resolve(const std::string& module_name, const std::string& from_file) {
        std::string file = find_module_file(module_name, from_file);
        if (file.empty()) return nullptr;
        return load_file(file, false);
    }

    DocumentModel* load_file(const std::string& path, bool is_definition_file, std::string content = "") {
        std::string canon;
        try { canon = fs::weakly_canonical(path).string(); } catch (...) { canon = path; }
        auto it = docs.find(canon);
        if (it != docs.end()) {
            if (!content.empty() && it->second->source != content) {
                it->second->source = content;
                Analyzer::parse(*it->second);
            }
            return it->second.get();
        }

        if (content.empty()) {
            content = utils::read_file_contents(canon);
            if (content.empty() && !fs::exists(canon)) return nullptr;
        }

        auto doc = std::make_unique<DocumentModel>();
        doc->file_path = canon;
        doc->source = content;
        doc->is_definition_file = is_definition_file || utils::has_suffix(canon, ".pyl.d");
        Analyzer::parse(*doc);
        auto* raw = doc.get();
        docs[canon] = std::move(doc);
        return raw;
    }

    void close_file(const std::string& path) {
        std::string canon;
        try { canon = fs::weakly_canonical(path).string(); } catch (...) { canon = path; }
        auto it = docs.find(canon);
        if (it != docs.end() && !it->second->is_definition_file) {
            docs.erase(it);
        }
    }

    std::vector<DocumentModel*> all_docs() {
        std::vector<DocumentModel*> out;
        out.reserve(docs.size());
        for (auto& [_, doc] : docs) out.push_back(doc.get());
        return out;
    }

    std::vector<DocumentModel*> std_docs() const {
        std::vector<DocumentModel*> out;
        for (const auto& [_, doc] : docs) {
            if (doc->is_definition_file) out.push_back(doc.get());
        }
        return out;
    }

    void clear_user_docs() {
        for (auto it = docs.begin(); it != docs.end();) {
            if (it->second->is_definition_file) {
                ++it;
            } else {
                it = docs.erase(it);
            }
        }
    }

private:
    std::vector<std::string> doc_import_paths(const std::string& from_file) const {
        std::string canon;
        try { canon = fs::weakly_canonical(from_file).string(); } catch (...) { return {}; }
        auto it = docs.find(canon);
        if (it == docs.end()) return {};
        return it->second->import_paths;
    }

    std::vector<std::string> module_candidates(const std::string& name, const std::string& from_file) {
        std::vector<std::string> out;
        std::string dir;
        try {
            dir = fs::path(from_file).parent_path().string();
        } catch (...) { dir = "."; }

        std::string plain = name.substr(name.find_last_of('/') + 1);

        std::string slash = "/";
        if (!dir.empty() && dir != ".") {
            out.push_back(dir + slash + name + ".pyl");
            out.push_back(dir + slash + name + ".pyl.d");
            out.push_back(dir + slash + name + slash + plain + ".pyl");
            out.push_back(dir + "/types/" + plain + ".pyl.d");
        }
        out.push_back(name + ".pyl");
        out.push_back(name + ".pyl.d");
        out.push_back(name + slash + plain + ".pyl");
        if (!workspace_root.empty()) {
            out.push_back(workspace_root + slash + name + ".pyl");
            out.push_back(workspace_root + slash + name + ".pyl.d");
            out.push_back(workspace_root + slash + name + slash + plain + ".pyl");
        }
        for (const auto& p : doc_import_paths(from_file)) {
            std::string norm = p;
            std::replace(norm.begin(), norm.end(), '\\', '/');
            std::vector<std::string> bases;
            bases.push_back(norm);
            if (!dir.empty() && dir != ".") bases.push_back(dir + slash + norm);
            if (!workspace_root.empty()) bases.push_back(workspace_root + slash + norm);
            for (const auto& base : bases) {
                out.push_back(base + slash + name + ".pyl");
                out.push_back(base + slash + name + ".pyl.d");
                out.push_back(base + slash + name + slash + plain + ".pyl");
            }
        }
        for (const auto& sp : std_paths) {
            out.push_back(sp + slash + name + ".pyl.d");
            out.push_back(sp + slash + "core" + slash + name + ".pyl.d");
        }
        fs::path walk = dir.empty() || dir == "." ? fs::current_path() : fs::path(dir);
        for (int depth = 0; depth < 4; ++depth) {
            if (walk.empty()) break;
            out.push_back(walk.string() + "/types/" + plain + ".pyl.d");
            out.push_back(walk.string() + slash + name + ".pyl");
            walk = walk.parent_path();
        }
        return out;
    }

    std::vector<std::string> std_paths;
    std::string workspace_root;
    std::map<std::string, std::unique_ptr<DocumentModel>> docs;
};

class TypeResolver {
public:
    explicit TypeResolver(ModuleResolver& modules) : modules(modules) {}

    std::vector<SymbolInfo> members_of(const std::string& type_name, bool include_private, bool static_only = false,
                                       const DocumentModel* context_doc = nullptr) {
        std::vector<SymbolInfo> result;
        if (type_name.empty() || type_name == ANY_TYPE) return result;

        if (utils::has_prefix(type_name, "module:")) {
            std::string module = type_name.substr(7);
            for (auto* doc : module_docs(module, context_doc)) {
                for (const auto& s : doc->symbols) {
                    if (s.parent_struct.empty() && s.scope_func.empty() &&
                        (s.kind == SymbolKind::Function || s.kind == SymbolKind::Struct ||
                         s.kind == SymbolKind::Variable || s.kind == SymbolKind::Module)) {
                        if (!include_private && !s.name.empty() && s.name[0] == '_') continue;
                        result.push_back(s);
                        result.back().owner_doc = doc;
                    }
                }
            }
            return result;
        }

        std::string base = type_name;
        if (utils::has_prefix(type_name, "array[")) base = "array";
        if (utils::has_prefix(type_name, "map[")) base = "map";

        for (auto* doc : modules.all_docs()) {
            auto it = doc->struct_members.find(base);
            if (it == doc->struct_members.end()) continue;
            for (const SymbolInfo* sp : it->second) {
                const SymbolInfo& s = *sp;
                bool is_private = !s.name.empty() && s.name[0] == '_';
                if (is_private && !include_private) continue;
                if (static_only && s.kind != SymbolKind::StaticFunction) continue;
                result.push_back(s);
                result.back().owner_doc = doc;
            }
        }

        if (base == "array") {
            SymbolInfo idx;
            idx.name = "[]";
            idx.kind = SymbolKind::Method;
            idx.detail = "operator []";
            idx.type_name = type_name == "array" ? ANY_TYPE : type_name.substr(6, type_name.size() - 7);
            result.push_back(idx);
        } else if (base == "map") {
            SymbolInfo idx;
            idx.name = "[]";
            idx.kind = SymbolKind::Method;
            idx.detail = "operator []";
            idx.type_name = ANY_TYPE;
            result.push_back(idx);
        }
        return result;
    }

    std::string resolve_base(const std::string& name, DocumentModel& doc, int depth = 8) {
        if (name.empty()) return ANY_TYPE;

        for (const auto& s : doc.symbols) {
            if (s.name != name) continue;
            switch (s.kind) {
                case SymbolKind::Variable:
                case SymbolKind::Parameter:
                case SymbolKind::Field: {
                    if (s.type_name.empty()) return ANY_TYPE;
                    if (s.type_name == ANY_TYPE) return ANY_TYPE;
                    if (s.type_name.find('(') != std::string::npos ||
                        s.type_name.find('.') != std::string::npos ||
                        s.type_name.find('[') != std::string::npos ||
                        s.type_name.find('{') != std::string::npos ||
                        s.type_name[0] == '"' || s.type_name[0] == '\'') {
                        return depth > 0 ? resolve_chain(s.type_name, doc, SIZE_MAX, depth - 1) : ANY_TYPE;
                    }
                    return s.type_name;
                }
                case SymbolKind::Function:
                case SymbolKind::Method:
                case SymbolKind::StaticFunction:
                    if (s.type_name.empty()) return ANY_TYPE;
                    if (s.type_name.find('(') != std::string::npos ||
                        s.type_name.find('.') != std::string::npos) {
                        return depth > 0 ? resolve_chain(s.type_name, doc, SIZE_MAX, depth - 1) : ANY_TYPE;
                    }
                    return s.type_name;
                case SymbolKind::Struct:
                    return s.type_name.empty() ? s.name : s.type_name;
                case SymbolKind::Module:
                    return s.type_name;
                default:
                    break;
            }
        }
        auto imp = doc.imports.find(name);
        if (imp != doc.imports.end()) return "module:" + imp->second;
        for (const auto& b : BUILTIN_TYPES) {
            if (b == name) return b;
        }
        for (auto* d : modules.all_docs()) {
            auto it = d->struct_symbols.find(name);
            if (it != d->struct_symbols.end()) {
                const SymbolInfo* s = it->second;
                return s->type_name.empty() ? s->name : s->type_name;
            }
        }
        for (const auto& [var, module] : doc.imports) {
            if (auto* m = modules.resolve(module, doc.file_path)) {
                auto it = m->struct_symbols.find(name);
                if (it != m->struct_symbols.end()) {
                    const SymbolInfo* s = it->second;
                    return s->type_name.empty() ? s->name : s->type_name;
                }
            }
        }
        return ANY_TYPE;
    }

    std::string resolve_chain(const std::string& chain, DocumentModel& doc, size_t cursor_line = SIZE_MAX, int depth = 8,
                              const std::string& self_hint = "") {
        if (chain.empty()) return ANY_TYPE;
        if (depth <= 0) return ANY_TYPE;

        if (chain[0] == '"' || chain[0] == '\'') return "string";
        if (chain == "true" || chain == "false") return "bool";
        if (chain == "none") return "none";
        if (chain == "[]") return "array[any]";
        if (chain == "{}") return "map[any,any]";
        if (is_number(chain)) return is_float(chain) ? "float" : "int";
        if (chain.find('[') != std::string::npos && utils::is_type_annotation(chain)) return chain;

        auto parts = tokenize_chain(chain);
        if (parts.empty()) return ANY_TYPE;
        if (parts[0].text == "fn" || parts[0].text == "fn(") return "function";

        std::string current;
        if (parts[0].text == "self") {
            const SymbolInfo* enclosing = nullptr;
            if (cursor_line != SIZE_MAX) {
                for (const auto& s : doc.symbols) {
                    if ((s.kind == SymbolKind::Method || s.kind == SymbolKind::StaticFunction) &&
                        s.range.start.line <= cursor_line && s.parent_struct.empty() == false &&
                        (s.range.end.line == 0 || cursor_line <= s.range.end.line)) {
                        if (!enclosing || s.range.start.line > enclosing->range.start.line) enclosing = &s;
                    }
                }
            }
            if (enclosing) {
                current = enclosing->parent_struct;
            } else if (!self_hint.empty()) {
                current = self_hint;
            } else if (cursor_line == SIZE_MAX) {
                for (const auto& s : doc.symbols) {
                    if (s.kind == SymbolKind::Field && !s.parent_struct.empty()) {
                        current = s.parent_struct;
                        break;
                    }
                }
                if (current.empty()) return ANY_TYPE;
            } else {
                return ANY_TYPE;
            }
        } else {
            current = resolve_base(parts[0].text, doc);
        }
        for (size_t i = 1; i < parts.size(); ++i) {
            const auto& part = parts[i];
            if (current == ANY_TYPE || current == "none") return ANY_TYPE;
            current = member_type(current, part.text, part.is_call, doc, depth, self_hint);
            if (part.is_index) {
                current = apply_index(current);
            }
        }
        return current;
    }


private:
    static std::string apply_index(const std::string& t) {
        if (utils::has_prefix(t, "array[") && utils::has_suffix(t, "]")) {
            std::string inner = t.substr(6, t.size() - 7);
            int depth = 0;
            for (size_t i = 0; i < inner.size(); ++i) {
                if (inner[i] == '[') depth++;
                else if (inner[i] == ']') depth--;
                else if (inner[i] == ',' && depth == 0) {
                    // array of pairs is unusual; treat first segment
                    break;
                }
            }
            return inner;
        }
        if (utils::has_prefix(t, "map[")) {
            size_t end = t.rfind(']');
            if (end != std::string::npos) {
                std::string inner = t.substr(4, end - 4);
                int depth = 0;
                for (size_t i = 0; i < inner.size(); ++i) {
                    if (inner[i] == '[') depth++;
                    else if (inner[i] == ']') depth--;
                    else if (inner[i] == ',' && depth == 0) {
                        return inner.substr(i + 1);
                    }
                }
            }
        }
        return ANY_TYPE;
    }

    std::string member_type(const std::string& owner_type, const std::string& member, bool is_call,
                            const DocumentModel& context_doc, int depth, const std::string& self_hint = "") {
        if (owner_type == ANY_TYPE || owner_type == "none") return ANY_TYPE;

        if (utils::has_prefix(owner_type, "array[")) {
            return member == "[]" ? owner_type.substr(6, owner_type.size() - 7) : ANY_TYPE;
        }

        auto members = members_of(owner_type, true, false, &context_doc);
        std::string result = ANY_TYPE;
        for (const auto& s : members) {
            if (s.name != member) continue;
            if (getenv("PYLE_LSP_DEBUG")) {
                debug_log("member_type hop: " + owner_type + "." + member + " kind=" + std::to_string((int)s.kind) +
                    " stored=[" + s.type_name + "] line=" + std::to_string(s.range.start.line) + " depth=" + std::to_string(depth));
            }
            if (s.type_name == "self") {
                result = owner_type;
                break;
            }
            if (s.kind == SymbolKind::Field || s.kind == SymbolKind::Variable ||
                s.kind == SymbolKind::Function || s.kind == SymbolKind::Method ||
                s.kind == SymbolKind::StaticFunction) {
                if (s.type_name.empty()) continue;
                const DocumentModel& target = s.owner_doc ? *s.owner_doc : context_doc;
                size_t scope_line = (s.kind == SymbolKind::Method || s.kind == SymbolKind::Function ||
                                     s.kind == SymbolKind::StaticFunction)
                                        ? s.range.start.line
                                        : SIZE_MAX;
                result = depth > 0 ? resolve_chain(s.type_name, const_cast<DocumentModel&>(target), scope_line, depth - 1,
                                                   utils::has_prefix(owner_type, "module:") ? "" : owner_type)
                                   : ANY_TYPE;
                if (result != ANY_TYPE) break;
            } else if (s.kind == SymbolKind::Struct) {
                result = s.name;
                break;
            }
        }
        return result;
    };

    std::vector<DocumentModel*> module_docs(const std::string& module_name, const DocumentModel* context_doc = nullptr) {
        std::vector<DocumentModel*> out;
        for (auto* d : modules.all_docs()) {
            if (d->is_definition_file) continue;
            std::string fn = d->file_path;
            std::string stem = fs::path(fn).stem().string();
            std::string dirname = fs::path(fn).parent_path().filename().string();
            if (stem == module_name || dirname == module_name) out.push_back(d);
        }
        if (out.empty()) {
            const std::string& anchor = context_doc ? context_doc->file_path : "";
            if (auto* doc = modules.resolve(module_name, anchor)) out.push_back(doc);
        }
        return out;
    }

    static bool is_number(const std::string& s) {
        if (s.empty()) return false;
        for (char c : s) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.' && c != '-') return false;
        }
        return true;
    }

    static bool is_float(const std::string& s) {
        return s.find('.') != std::string::npos;
    }

    ModuleResolver& modules;
};

}
