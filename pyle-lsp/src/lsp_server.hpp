#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_transport.hpp"
#include "lsp_resolver.hpp"
#include "lsp_analyzer.hpp"

#include <nlohmann/json.hpp>
#include <algorithm>
#include <set>
#include <sstream>

namespace pyle::lsp {

using json = nlohmann::json;


class LspServer {
public:
    LspServer(ModuleResolver& resolver, std::vector<std::string> std_paths)
        : resolver(resolver), types(resolver), std_paths(std::move(std_paths)) {}

    void run() {
        while (true) {
            std::string raw = JsonRpcTransport::read_message();
            if (raw.empty()) break;
            json msg = json::parse(raw, nullptr, false);
            if (msg.is_discarded()) continue;
            try {
                handle_message(msg);
            } catch (const std::exception& e) {
                log(std::string("handler exception: ") + e.what());
            } catch (...) {
                log("handler exception: unknown");
            }
            if (should_exit) break;
        }
    }

private:
    static void log(const std::string& s) {
        std::cerr << "[pyle-lsp] " << s << std::endl;
    }

    void send(const json& msg) {
        JsonRpcTransport::write_message(msg.dump());
    }

    void handle_message(const json& msg) {
        if (!msg.is_object()) return;
        if (msg.contains("method")) {
            std::string method = msg["method"].get<std::string>();
            fprintf(stderr, "[pyle-lsp] >> %s\n", method.c_str());
            auto id = msg.contains("id") ? msg["id"] : json();
            json params = msg.contains("params") ? msg["params"] : json();
            if (is_notification(method)) {
                handle_notification(method, params);
                return;
            }
            json result = dispatch_request(method, params);
            if (!id.is_null()) {
                send(json{{"jsonrpc", "2.0"}, {"id", id}, {"result", result}});
            }
        } else if (msg.contains("id") && msg.contains("error")) {
        }
    }

    static bool is_notification(const std::string& m) {
        static const std::set<std::string> notif = {
            "initialized", "exit", "textDocument/didOpen", "textDocument/didChange",
            "textDocument/didClose", "textDocument/didSave", "workspace/didChangeWorkspaceFolders",
            "workspace/didChangeConfiguration", "$/setTrace", "$/cancelRequest"
        };
        return notif.count(m) > 0;
    }

    void handle_notification(const std::string& method, const json& params) {
        if (method == "initialized") {
            on_initialized();
        } else if (method == "exit") {
            should_exit = true;
        } else if (method == "textDocument/didOpen") {
            on_did_open(params);
        } else if (method == "textDocument/didChange") {
            on_did_change(params);
        } else if (method == "textDocument/didClose") {
            on_did_close(params);
        } else if (method == "textDocument/didSave") {
            on_did_open(params);
        }
    }

    json dispatch_request(const std::string& method, const json& params) {
        if (method == "initialize") return on_initialize(params);
        if (method == "shutdown") return nullptr;
        if (method == "textDocument/completion") return on_completion(params);
        if (method == "textDocument/hover") return on_hover(params);
        if (method == "textDocument/definition") return on_definition(params);
        if (method == "textDocument/documentSymbol") return on_document_symbol(params);
        if (method == "textDocument/signatureHelp") return on_signature_help(params);
        if (method == "textDocument/semanticTokens/full") return on_semantic_tokens(params);
        if (method == "completionItem/resolve") return params;
        log("unhandled request: " + method);
        return nullptr;
    }


    static std::string uri_of(const json& params) {
        return params["textDocument"]["uri"].get<std::string>();
    }

    static std::string path_of(const json& params) {
        return utils::file_uri_to_path(uri_of(params));
    }

    static Position pos_of(const json& p) {
        Position pos;
        pos.line = p["line"].get<size_t>();
        pos.character = p["character"].get<size_t>();
        return pos;
    }

    static bool pos_in_range(const Position& p, const Range& r) {
        if (p.line != r.start.line) return false;
        return p.character >= r.start.character && p.character <= r.end.character;
    }

    const SymbolInfo* symbol_at(const DocumentModel& doc, const Position& pos) const {
        const SymbolInfo* best = nullptr;
        for (const auto& s : doc.symbols) {
            if (pos_in_range(pos, s.selection_range)) {
                if (!best || (s.selection_range.end.character - s.selection_range.start.character) <=
                             (best->selection_range.end.character - best->selection_range.start.character)) {
                    best = &s;
                }
            }
        }
        return best;
    }

    DocumentModel* get_document(const std::string& uri) {
        return resolver.load_file(utils::file_uri_to_path(uri), false);
    }

    std::vector<std::string> lines_of(const DocumentModel& doc) const {
        return utils::get_lines(doc.source);
    }


    json on_initialize(const json& params) {
        std::string root;
        if (params.contains("workspaceFolders") && params["workspaceFolders"].is_array() &&
            !params["workspaceFolders"].empty()) {
            resolver.clear_user_docs();
            for (const auto& wf : params["workspaceFolders"]) {
                if (wf.contains("uri") && !wf["uri"].is_null()) {
                    root = utils::file_uri_to_path(wf["uri"].get<std::string>());
                    if (!root.empty()) break;
                }
            }
        }
        if (root.empty() && params.contains("rootUri") && !params["rootUri"].is_null()) {
            root = utils::file_uri_to_path(params["rootUri"].get<std::string>());
        }
        resolver.set_workspace_root(root);
        if (!root.empty()) {
            std::string candidate = root + "/std";
            if (resolver.add_std_path(candidate)) {
                resolver.preload_std();
                log("added workspace std path: " + candidate);
            }
        }
        return json{
            {"capabilities", json{
                {"textDocumentSync", json{{"openClose", true}, {"change", 1}}},
                {"completionProvider", json{{"triggerCharacters", json::array({".", ":", "\""})}}},
                {"hoverProvider", true},
                {"definitionProvider", true},
                {"documentSymbolProvider", true},
                {"signatureHelpProvider", json{{"triggerCharacters", json::array({"(", ","})}}},
                {"semanticTokensProvider", json{
                    {"legend", json{
                        {"tokenTypes", json::array({"keyword", "type", "function", "method", "variable",
                                                    "parameter", "property", "string", "number"})},
                        {"tokenModifiers", json::array()}
                    }},
                    {"full", true}
                }}
            }},
            {"serverInfo", json{{"name", "pyle-lsp"}, {"version", "1.0.0"}}}
        };
    }

    void on_initialized() {
        if (!resolver.std_available()) {
            send(json{
                {"jsonrpc", "2.0"},
                {"method", "window/showMessage"},
                {"params", json{
                    {"type", 2},
                    {"message", "pyle-lsp: std/core not found. Builtin types (string, array, ...) "
                                "autocomplete and hover will be limited. Run pyle-lsp with --std-path "
                                "pointing at the std folder of your pyle interpreter."}
                }}
            });
        }
    }


    void publish_diagnostics(const std::string& uri, const DocumentModel& doc) {
        json items = json::array();
        std::set<std::string> seen;
        int count = 0;
        for (const auto& e : doc.reporter.get_errors()) {
            if (count >= 200) break;
            if (e.type == pyle::ErrorType::Compile) {
                if (e.message.rfind("Undefined global", 0) == 0) continue;
            }
            std::string key = std::to_string(e.loc.line) + ":" + std::to_string(e.loc.column) + ":" + e.message;
            if (!seen.insert(key).second) continue;
            Position p = utils::span_to_position(e.loc, doc.source);
            items.push_back(json{
                {"range", json{
                    {"start", json{{"line", p.line}, {"character", p.character}}},
                    {"end", json{{"line", p.line}, {"character", p.character + e.length}}}
                }},
                {"severity", e.type == pyle::ErrorType::Compile ? 1 : 1},
                {"message", e.message}
            });
            ++count;
        }
        send(json{
            {"jsonrpc", "2.0"},
            {"method", "textDocument/publishDiagnostics"},
            {"params", json{{"uri", uri}, {"diagnostics", items}}}
        });
    }


    void on_did_open(const json& params) {
        std::string uri = uri_of(params);
        std::string path = utils::file_uri_to_path(uri);
        std::string text = params["textDocument"].contains("text") && !params["textDocument"]["text"].is_null()
                               ? params["textDocument"]["text"].get<std::string>()
                               : "";
        DocumentModel* doc = resolver.load_file(path, false, text);
        if (doc) publish_diagnostics(uri, *doc);
    }

    void on_did_change(const json& params) {
        std::string uri = uri_of(params);
        std::string path = utils::file_uri_to_path(uri);
        std::string text;
        if (params.contains("contentChanges") && params["contentChanges"].is_array() &&
            !params["contentChanges"].empty()) {
            const auto& last = params["contentChanges"].back();
            if (last.contains("text") && !last["text"].is_null()) {
                text = last["text"].get<std::string>();
            }
        }
        if (text.empty()) return;
        DocumentModel* doc = resolver.load_file(path, false, text);
        if (doc) publish_diagnostics(uri, *doc);
    }

    void on_did_close(const json& params) {
        std::string uri = uri_of(params);
        std::string path = utils::file_uri_to_path(uri);
        resolver.close_file(path);
        send(json{
            {"jsonrpc", "2.0"},
            {"method", "textDocument/publishDiagnostics"},
            {"params", json{{"uri", uri}, {"diagnostics", json::array()}}}
        });
    }


    static bool ends_with(const std::string& s, const std::string& suffix) {
        return utils::has_suffix(s, suffix);
    }

    static bool import_context(const std::string& before, std::string& prefix) {
        size_t call = before.rfind("import(");
        if (call == std::string::npos) return false;
        std::string rest = before.substr(call + 7);
        size_t open = rest.find_first_of("\"'");
        if (open == std::string::npos) return false;
        if (rest.find_first_of("\"'", open + 1) != std::string::npos) return false;
        prefix = rest.substr(open + 1);
        return true;
    }

    static bool type_position(const std::string& before) {
        std::string trimmed = before;
        while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t')) trimmed.pop_back();
        return !trimmed.empty() && trimmed.back() == ':';
    }

    static std::string enclosing_function_name(const DocumentModel& doc, size_t line) {
        std::string best;
        size_t best_line = SIZE_MAX;
        for (const auto& s : doc.symbols) {
            if (s.kind != SymbolKind::Function && s.kind != SymbolKind::Method &&
                s.kind != SymbolKind::StaticFunction) continue;
            if (s.range.start.line > line) continue;
            if (s.range.end.line != 0 && line > s.range.end.line) continue;
            if (best_line == SIZE_MAX || s.range.start.line > best_line) {
                best_line = s.range.start.line;
                best = s.name;
            }
        }
        return best;
    }

    std::vector<std::string> module_candidates_for(const DocumentModel& doc) const {
        std::set<std::string> stems;
        std::vector<std::string> dirs;
        dirs.push_back(fs::path(doc.file_path).parent_path().string());
        for (const auto& ip : doc.import_paths) {
            dirs.push_back(fs::path(ip).is_absolute() ? ip : fs::path(doc.file_path).parent_path().string() + "/" + ip);
        }
        dirs.push_back(".");
        for (const auto& sp : std_paths) dirs.push_back(sp);

        std::error_code ec;
        for (const auto& dir : dirs) {
            if (!fs::exists(dir, ec) || ec) continue;
            for (const auto& entry : fs::directory_iterator(dir, ec)) {
                if (ec) break;
                std::string name = entry.path().filename().string();
                if (utils::has_suffix(name, ".pyl") && !utils::has_suffix(name, ".pyl.d")) {
                    stems.insert(name.substr(0, name.size() - 4));
                } else if (utils::has_suffix(name, ".pyl.d")) {
                    stems.insert(name.substr(0, name.size() - 6));
                }
            }
        }
        return std::vector<std::string>(stems.begin(), stems.end());
    }

    json on_completion(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;
        Position pos = pos_of(params["position"]);

        auto lines = lines_of(*doc);
        if (pos.line >= lines.size()) return nullptr;
        const std::string& line = lines[pos.line];
        std::string before = (pos.character <= line.size()) ? line.substr(0, pos.character) : line;
        std::string prefix = utils::prefix_at(doc->source, pos);

        json items = json::array();

        std::string import_prefix;
        if (import_context(before, import_prefix)) {
            std::string pfx = utils::strip_quotes(import_prefix);
            for (const auto& stem : module_candidates_for(*doc)) {
                if (!pfx.empty() && !utils::has_prefix(stem, pfx)) continue;
                items.push_back(json{
                    {"label", stem}, {"kind", 9}, {"detail", "module"}, {"insertText", stem}
                });
            }
            return json{{"isIncomplete", false}, {"items", items}};
        }

        if (type_position(before)) {
            std::set<std::string> added;
            for (const auto& b : BUILTIN_TYPES) {
                if (!utils::has_prefix(b, prefix)) continue;
                items.push_back(json{{"label", b}, {"kind", 22}, {"detail", "builtin type"}});
                added.insert(b);
            }
            for (auto* d : resolver.all_docs()) {
                for (const auto& s : d->symbols) {
                    if (s.kind != SymbolKind::Struct || !utils::has_prefix(s.name, prefix)) continue;
                    if (added.count(s.name)) continue;
                    added.insert(s.name);
                    items.push_back(json{
                        {"label", s.name}, {"kind", 22}, {"detail", s.detail},
                        {"documentation", s.documentation.empty() ? s.detail : s.documentation}
                    });
                }
            }
            for (const auto& b : LSP_KEYWORDS) {
                if (b.first != "fn" && b.first != "struct") continue;
                if (!utils::has_prefix(b.first, prefix)) continue;
                items.push_back(json{{"label", b.first}, {"kind", 14}});
            }
            return json{{"isIncomplete", false}, {"items", items}};
        }

        std::string base_text = utils::base_expression_before_dot(before);
        if (getenv("PYLE_LSP_DEBUG")) {
            log("completion line=" + std::to_string(pos.line) + " char=" + std::to_string(pos.character) +
                " prefix=[" + prefix + "] base=[" + base_text + "] text=[" + line.substr(0, std::min<size_t>(80, line.size())) + "]");
        }
        if (!base_text.empty()) {
            bool is_self = (base_text == "self");
            std::string base_type = types.resolve_chain(base_text, *doc, pos.line);
            if (base_type == ANY_TYPE) {
                for (const auto& b : BUILTIN_TYPES) {
                    if (b == base_text) { base_type = b; break; }
                }
            }
            bool is_static = false;
            if (!is_self && base_type != ANY_TYPE) {
                for (auto* d : resolver.all_docs()) {
                    for (const auto& s : d->symbols) {
                        if (s.kind == SymbolKind::Struct && s.name == base_text) {
                            is_static = true;
                            break;
                        }
                    }
                    if (is_static) break;
                }
            }
            auto members = types.members_of(base_type, is_self, is_static, doc);
            if (getenv("PYLE_LSP_DEBUG")) {
                log("member branch: base_type=[" + base_type + "] is_self=" + std::to_string(is_self) + " count=" + std::to_string(members.size()));
            }
            std::set<std::string> seen;
            for (const auto& m : members) {
                if (m.name == "[]") continue;
                if (!utils::has_prefix(m.name, prefix)) continue;
                if (!seen.insert(m.name + "\n" + m.detail).second) continue;
                items.push_back(completion_item(m));
            }
            return json{{"isIncomplete", false}, {"items", items}};
        }

        std::string enclosing_func = enclosing_function_name(*doc, pos.line);
        std::set<std::string> added;
        for (const auto& s : doc->symbols) {
            if (!utils::has_prefix(s.name, prefix)) continue;
            if (added.count(s.name)) continue;
            if (s.kind == SymbolKind::Field || s.kind == SymbolKind::Method ||
                s.kind == SymbolKind::StaticFunction) continue;
            if (s.kind == SymbolKind::Struct || s.kind == SymbolKind::Function) {
                if (!s.parent_struct.empty()) continue;
            }
            if (s.is_local) {
                if (s.scope_func != enclosing_func) continue;
                if (s.range.start.line > pos.line) continue;
                if (s.scope_end_line >= 0 && pos.line > s.scope_end_line) continue;
            } else if (s.kind == SymbolKind::Variable || s.kind == SymbolKind::Module) {
                if (s.range.start.line > pos.line) continue;
            }
            added.insert(s.name);
            items.push_back(completion_item(s));
        }
        for (const auto& b : BUILTIN_TYPES) {
            if (added.count(b)) continue;
            if (!utils::has_prefix(b, prefix)) continue;
            added.insert(b);
            items.push_back(json{{"label", b}, {"kind", 22}, {"detail", "builtin type"}});
        }
        for (const auto& [word, type] : LSP_KEYWORDS) {
            if (added.count(word)) continue;
            if (!utils::has_prefix(word, prefix)) continue;
            added.insert(word);
            items.push_back(json{{"label", word}, {"kind", 14}, {"detail", type}});
        }
        return json{{"isIncomplete", false}, {"items", items}};
    }

    json completion_item(const SymbolInfo& s) const {
        int kind = 6;
        switch (s.kind) {
            case SymbolKind::Function: kind = 3; break;
            case SymbolKind::Method:
            case SymbolKind::StaticFunction: kind = 2; break;
            case SymbolKind::Struct: kind = 22; break;
            case SymbolKind::Field: kind = 5; break;
            case SymbolKind::Parameter: kind = 6; break;
            case SymbolKind::Module: kind = 9; break;
            case SymbolKind::Keyword: kind = 14; break;
            default: break;
        }
        return json{
            {"label", s.name}, {"kind", kind},
            {"detail", s.detail},
            {"documentation", s.documentation.empty() ? "" : s.documentation},
            {"insertText", s.name}
        };
    }


    json on_hover(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;
        Position pos = pos_of(params["position"]);
        std::string word = utils::word_at(doc->source, pos);
        if (word.empty()) return nullptr;

        const SymbolInfo* sym = symbol_at(*doc, pos);

        auto lines = lines_of(*doc);
        if (pos.line < lines.size()) {
            std::string line = lines[pos.line];
            std::string before = (pos.character <= line.size()) ? line.substr(0, pos.character) : line;
            std::string base_text = utils::base_expression_before_dot(before);
            if (!base_text.empty() && !sym) {
                std::string base_type = types.resolve_chain(base_text, *doc, pos.line);
                if (getenv("PYLE_LSP_DEBUG")) {
                    log("hover: word=[" + word + "] base=[" + base_text + "] base_type=[" + base_type + "]");
                }
                const SymbolInfo* partial = nullptr;
                for (const auto& m : types.members_of(base_type, true, false, doc)) {
                    if (m.name == word) { sym = &m; break; }
                    if (!partial && utils::has_prefix(m.name, word)) partial = &m;
                }
                if (!sym && partial) sym = partial;
            }
        }

        if (!sym) {
            std::string t = types.resolve_base(word, *doc);
            if (t != ANY_TYPE && t != "") {
                return json{{"contents", json{
                    {"kind", "markdown"}, {"value", "```pyle\n" + t + "\n```\n\n*" + t + "*"}
                }}};
            }
            return nullptr;
        }

        std::string md = "```pyle\n" + sym->detail + "\n```";
        if (!sym->type_name.empty() && sym->type_name != sym->name) {
            std::string shown = sym->type_name;
            if (shown.find("self") != std::string::npos || shown.find('.') != std::string::npos ||
                shown.find('(') != std::string::npos) {
                const DocumentModel& target = sym->owner_doc ? *sym->owner_doc : *doc;
                std::string resolved =
                    types.resolve_chain(shown, const_cast<DocumentModel&>(target), sym->range.start.line);
                if (resolved != ANY_TYPE && resolved != "none" && !resolved.empty()) shown = resolved;
            }
            md += "\n\nType: `" + shown + "`";
        }
        if (sym->kind == SymbolKind::Field && !sym->parent_struct.empty()) {
            md += "\n\nField of `" + sym->parent_struct + "`";
        }
        if (sym->is_static) md += "\n\nStatic";
        md += "\n\n" + sym->file_path;
        if (sym->selection_range.start.line != 0 || sym->selection_range.start.character != 0) {
            md += ":" + std::to_string(sym->selection_range.start.line + 1);
        }
        return json{{"contents", json{{"kind", "markdown"}, {"value", md}}}};
    }


    json on_definition(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;
        Position pos = pos_of(params["position"]);
        std::string word = utils::word_at(doc->source, pos);
        if (word.empty()) return nullptr;

        const SymbolInfo* sym = symbol_at(*doc, pos);

        auto lines = lines_of(*doc);
        if (!sym && pos.line < lines.size()) {
            std::string line = lines[pos.line];
            std::string before = (pos.character <= line.size()) ? line.substr(0, pos.character) : line;
            std::string base_text = utils::base_expression_before_dot(before);
            if (!base_text.empty()) {
                std::string base_type = types.resolve_chain(base_text, *doc, pos.line);
                const SymbolInfo* partial = nullptr;
                for (const auto& m : types.members_of(base_type, true, false, doc)) {
                    if (m.name == word) { sym = &m; break; }
                    if (!partial && utils::has_prefix(m.name, word)) partial = &m;
                }
                if (!sym && partial) sym = partial;
            }
        }
        if (!sym) {
            std::string t = types.resolve_base(word, *doc);
            if (t != ANY_TYPE && !t.empty()) {
                for (auto* d : resolver.all_docs()) {
                    for (const auto& s : d->symbols) {
                        if (s.kind == SymbolKind::Struct && s.name == t) { sym = &s; break; }
                    }
                    if (sym) break;
                }
            }
        }
        if (!sym) return nullptr;

        return json::array({
            json{
                {"uri", utils::path_to_file_uri(sym->file_path)},
                {"range", json{
                    {"start", json{{"line", sym->selection_range.start.line}, {"character", sym->selection_range.start.character}}},
                    {"end", json{{"line", sym->selection_range.end.line}, {"character", sym->selection_range.end.character}}}
                }}
            }
        });
    }


    json on_document_symbol(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;

        json result = json::array();
        for (const auto& s : doc->symbols) {
            if (s.kind != SymbolKind::Struct) continue;
            json children = json::array();
            for (const auto& c : doc->symbols) {
                if (c.parent_struct != s.name) continue;
                children.push_back(symbol_to_doc_symbol(c));
            }
            json item = symbol_to_doc_symbol(s);
            item["children"] = children;
            result.push_back(item);
        }
        for (const auto& s : doc->symbols) {
            if (s.kind == SymbolKind::Struct || !s.parent_struct.empty() || s.is_local) continue;
            result.push_back(symbol_to_doc_symbol(s));
        }
        return result;
    }

    json symbol_to_doc_symbol(const SymbolInfo& s) const {
        int kind = 13;
        switch (s.kind) {
            case SymbolKind::Function:
            case SymbolKind::Method:
            case SymbolKind::StaticFunction: kind = 12; break;
            case SymbolKind::Struct: kind = 5; break;
            case SymbolKind::Variable:
            case SymbolKind::Parameter: kind = 13; break;
            case SymbolKind::Field: kind = 8; break;
            case SymbolKind::Module: kind = 2; break;
            default: break;
        }
        return json{
            {"name", s.name},
            {"kind", kind},
            {"detail", s.detail},
            {"range", json{
                {"start", json{{"line", s.range.start.line}, {"character", s.range.start.character}}},
                {"end", json{{"line", s.range.end.line}, {"character", s.range.end.character}}}
            }},
            {"selectionRange", json{
                {"start", json{{"line", s.selection_range.start.line}, {"character", s.selection_range.start.character}}},
                {"end", json{{"line", s.selection_range.end.line}, {"character", s.selection_range.end.character}}}
            }}
        };
    }


    json on_signature_help(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;
        Position pos = pos_of(params["position"]);

        auto lines = lines_of(*doc);
        if (pos.line >= lines.size()) return nullptr;
        std::string line = lines[pos.line];
        std::string before = (pos.character <= line.size()) ? line.substr(0, pos.character) : line;

        int depth = 0;
        size_t open = std::string::npos;
        for (size_t i = before.size(); i > 0; --i) {
            char c = before[i - 1];
            if (c == ')') depth++;
            else if (c == '(') {
                if (depth > 0) depth--;
                else { open = i - 1; break; }
            }
        }
        if (open == std::string::npos) return nullptr;

        size_t comma_count = 0;
        for (size_t i = open + 1; i < before.size(); ++i) {
            if (before[i] == ',') comma_count++;
        }

        size_t start = open;
        while (start > 0) {
            char c = before[start - 1];
            if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '[' || c == ']') start--;
            else break;
        }
        std::string callee = before.substr(start, open - start);

        const SymbolInfo* fn = nullptr;
        auto parts = tokenize_chain(callee);
        if (parts.empty()) return nullptr;
        if (parts.size() == 1) {
            for (const auto& s : doc->symbols) {
                if (s.name == callee && (s.kind == SymbolKind::Function ||
                    s.kind == SymbolKind::Method || s.kind == SymbolKind::StaticFunction)) {
                    fn = &s;
                    break;
                }
            }
            if (!fn) {
                for (const auto& s : doc->symbols) {
                    if (s.kind != SymbolKind::Struct || s.name != callee) continue;
                    for (const auto& c : doc->symbols) {
                        if (c.kind == SymbolKind::Method && c.name == "_init" && c.parent_struct == callee) {
                            fn = &c;
                            break;
                        }
                    }
                    if (fn) break;
                }
            }
            if (!fn) {
                for (const auto& b : LSP_KEYWORDS) {
                    if (b.first == callee) return nullptr;
                }
            }
        } else {
            std::string owner = "";
            for (size_t i = 0; i + 1 < parts.size(); ++i) {
                owner += (i == 0) ? parts[i].text : "." + parts[i].text;
            }
            std::string base_type = types.resolve_chain(owner, *doc);
            const SymbolInfo* member = nullptr;
            for (const auto& m : types.members_of(base_type, true, false, doc)) {
                if (m.name == parts.back().text) { member = &m; break; }
            }
            if (member) {
                if (member->kind == SymbolKind::Struct) {
                    const DocumentModel* target = member->owner_doc ? member->owner_doc : doc;
                    for (const auto& c : target->symbols) {
                        if (c.kind == SymbolKind::Method && c.name == "_init" &&
                            c.parent_struct == member->name) {
                            fn = &c;
                            break;
                        }
                    }
                } else {
                    fn = member;
                }
            }
        }
        if (!fn) return nullptr;

        json params_list = json::array();
        std::string detail = fn->detail;
        size_t lp = detail.find('(');
        size_t rp = detail.rfind(')');
        if (lp != std::string::npos && rp != std::string::npos && rp > lp) {
            std::string inner = detail.substr(lp + 1, rp - lp - 1);
            std::stringstream ss(inner);
            std::string part;
            while (std::getline(ss, part, ',')) {
                std::string trimmed = part;
                trimmed.erase(0, trimmed.find_first_not_of(" \t"));
                trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
                std::string label = trimmed;
                size_t colon = trimmed.find(':');
                if (colon != std::string::npos) label = trimmed.substr(0, colon);
                label.erase(label.find_last_not_of(" \t") + 1);
                if (!label.empty()) params_list.push_back(json{{"label", label}});
            }
        }
        if (params_list.empty()) {
            params_list.push_back(json{{"label", ""}});
        }

        return json{
            {"signatures", json::array({
                json{{"label", detail}, {"parameters", params_list}}
            })},
            {"activeSignature", 0},
            {"activeParameter", std::min(comma_count, params_list.size() - 1)}
        };
    }


    json on_semantic_tokens(const json& params) {
        std::string uri = uri_of(params);
        DocumentModel* doc = get_document(uri);
        if (!doc) return nullptr;

        auto symbol_kind_at = [&](size_t line, size_t character) -> int {
            Position p{line, character};
            const SymbolInfo* s = symbol_at(*doc, p);
            if (!s) return -1;
            switch (s->kind) {
                case SymbolKind::Struct: return 1;
                case SymbolKind::Function: return 2;
                case SymbolKind::Method:
                case SymbolKind::StaticFunction: return 3;
                case SymbolKind::Parameter: return 5;
                case SymbolKind::Field: return 6;
                default: return 4;
            }
        };

        std::vector<int> data;
        int last_line = 0;
        int last_start = 0;
        for (const auto& t : doc->tokens) {
            if (t.type == pyle::TokenType::ERROR || t.type == pyle::TokenType::EOF_TOKEN) continue;
            int tok_type = -1;
            size_t len = t.lexeme.size();
            switch (t.type) {
                case pyle::TokenType::AND: case pyle::TokenType::OR: case pyle::TokenType::NOT:
                case pyle::TokenType::IF: case pyle::TokenType::ELSE: case pyle::TokenType::ELIF:
                case pyle::TokenType::FOR: case pyle::TokenType::WHILE: case pyle::TokenType::IN:
                case pyle::TokenType::LOOP: case pyle::TokenType::BREAK: case pyle::TokenType::CONTINUE:
                case pyle::TokenType::FN: case pyle::TokenType::RETURN: case pyle::TokenType::LET:
                case pyle::TokenType::STRUCT: case pyle::TokenType::NONE: case pyle::TokenType::TRUE:
                case pyle::TokenType::FALSE: case pyle::TokenType::YIELD: case pyle::TokenType::ENUM:
                case pyle::TokenType::STATIC:
                    tok_type = 0;
                    break;
                case pyle::TokenType::IDENTIFIER:
                    tok_type = symbol_kind_at(t.selection.line, t.selection.column - 1);
                    if (tok_type < 0) tok_type = 4;
                    break;
                case pyle::TokenType::STRING:
                    tok_type = 7;
                    break;
                case pyle::TokenType::INT: case pyle::TokenType::FLOAT:
                    tok_type = 8;
                    break;
                default:
                    continue;
            }
            int line = (int)t.selection.line;
            int start = (int)t.selection.column - 1;
            data.push_back(line - last_line);
            if (line == last_line) {
                data.push_back(start - last_start);
            } else {
                data.push_back(start);
            }
            last_line = line;
            last_start = start;
            data.push_back((int)len);
            data.push_back(tok_type);
            data.push_back(0);
        }
        return json{{"data", data}};
    }


    ModuleResolver& resolver;
    TypeResolver types;
    std::vector<std::string> std_paths;
    bool should_exit = false;
};

}
