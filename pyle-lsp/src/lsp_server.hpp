#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_transport.hpp"
#include "lsp_document.hpp"
#include <algorithm>

namespace pyle::lsp {

class LspServer {
    DocumentManager docs;

public:
    void run() {
        while (true) {
            std::string msg = JsonRpcTransport::read_message();
            if (msg.empty()) break;

            json req;
            try { req = json::parse(msg); }
            catch (...) { continue; }

            if (!req.contains("method")) continue;
            std::string method = req["method"];

            try {
                if (method == "initialize") {
                    handle_init(req);
                } else if (method == "shutdown") {
                    JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
                    break;
                } else if (method == "textDocument/didOpen") {
                    handle_open(req);
                } else if (method == "textDocument/didChange") {
                    handle_change(req);
                } else if (method == "textDocument/didClose") {
                    handle_close(req);
                } else if (method == "textDocument/completion") {
                    handle_completion(req);
                } else if (method == "textDocument/hover") {
                    handle_hover(req);
                } else if (method == "textDocument/definition") {
                    handle_definition(req);
                } else if (method == "textDocument/documentSymbol") {
                    handle_doc_symbol(req);
                } else if (method == "textDocument/semanticTokens/full") {
                    handle_semantic(req);
                } else if (req.contains("id")) {
                    JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
                }
            } catch (...) {}
        }
    }

private:
    void handle_init(const json& req) {
        json caps = pyle::lsp::utils::make_obj({
            {"textDocumentSync", pyle::lsp::utils::make_obj({{"openClose", true}, {"change", 1}})},
            {"completionProvider", pyle::lsp::utils::make_obj({{"triggerCharacters", json::array({".", ":"})} })},
            {"hoverProvider", true},
            {"definitionProvider", true},
            {"documentSymbolProvider", true},
            {"semanticTokensProvider", pyle::lsp::utils::make_obj({
                {"full", pyle::lsp::utils::make_obj({{"delta", false}})},
                {"legend", pyle::lsp::utils::make_obj({
                    {"tokenTypes", json::array({"function", "struct", "variable", "parameter", "keyword", "string", "number", "comment", "property", "module"})},
                    {"tokenModifiers", json::array()}
                })}
            })}
        });

        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, {"id", req["id"]},
            {"result", pyle::lsp::utils::make_obj({{"capabilities", caps}})}
        }).dump());
    }

    void handle_open(const json& req) {
        auto& p = req["params"];
        docs.open(pyle::lsp::utils::file_uri_to_path(p["textDocument"]["uri"]), p["textDocument"]["text"]);
    }

    void handle_change(const json& req) {
        auto& p = req["params"];
        docs.change(pyle::lsp::utils::file_uri_to_path(p["textDocument"]["uri"]), p["contentChanges"][0]["text"]);
    }

    void handle_close(const json& req) {
        docs.close(pyle::lsp::utils::file_uri_to_path(req["params"]["textDocument"]["uri"]));
    }

    void handle_completion(const json& req) {
        auto& p = req["params"];
        std::string path = pyle::lsp::utils::file_uri_to_path(p["textDocument"]["uri"]);
        size_t line = p["position"]["line"];
        size_t ch = p["position"]["character"];

        json items = json::array();
        Document* doc = docs.get(path);
        if (!doc) { send_comp(req["id"], items); return; }

        auto lines = pyle::lsp::utils::get_lines(doc->source);
        std::string text_before = (line < lines.size()) ? lines[line].substr(0, ch) : "";

        std::string obj_name, after_dot;
        auto dot = text_before.rfind('.');
        if (dot != std::string::npos && dot > 0) {
            obj_name = text_before.substr(0, dot);
            size_t ws = obj_name.find_last_of(" \t\n({[,=+-*/%");
            if (ws != std::string::npos) obj_name = obj_name.substr(ws + 1);
            after_dot = text_before.substr(dot + 1);
        }

        size_t last_brk = text_before.find_last_of(" \t\n({[,=+-*/%");
        std::string prefix = (last_brk != std::string::npos) ? text_before.substr(last_brk + 1) : text_before;
        if (!obj_name.empty()) prefix = after_dot;

        std::string current_func_name = "";
        std::string current_struct_name = "";
        for (const auto& sym : doc->symbols) {
            if ((sym.kind == SymbolKind::Function || sym.is_method) && line >= sym.range.start.line && line <= sym.range.end.line) {
                current_func_name = sym.name;
                if (sym.is_method) current_struct_name = sym.parent_struct;
            }
        }

        if (!obj_name.empty()) {
            if (obj_name == "self") {
                if (!current_struct_name.empty()) {
                    auto ff = docs.fields_of(path, current_struct_name, true);
                    for (const auto& f : ff) {
                        if (prefix.empty() || f.name.find(prefix) == 0) {
                            int k = (f.is_method) ? 2 : 5;
                            std::string sort_prio = (f.name.front() == '_') ? "1_" : "0_";
                            
                            items.push_back(pyle::lsp::utils::make_obj({
                                {"label", f.name}, {"kind", k}, {"detail", f.detail},
                                {"insertText", f.name}, {"sortText", sort_prio + f.name}
                            }));
                        }
                    }
                }
            } 
            else if (doc->imports.find(obj_name) != doc->imports.end()) {
                std::string resolved = docs.resolve_import(path, doc->imports.at(obj_name), doc->import_paths);
                if (!resolved.empty()) {
                    Document* mod_doc = docs.get(resolved);
                    if (mod_doc) {
                        for (const auto& sym : mod_doc->symbols) {
                            if (!sym.parent_struct.empty() || sym.is_local) continue; 
                            if (!sym.name.empty() && sym.name.front() == '_') continue; 
                            
                            if (prefix.empty() || sym.name.find(prefix) == 0) {
                                int k = (sym.kind == SymbolKind::Struct) ? 22 : 3;
                                items.push_back(pyle::lsp::utils::make_obj({
                                    {"label", sym.name}, {"kind", k}, {"detail", sym.detail},
                                    {"sortText", "0_" + sym.name}
                                }));
                            }
                        }
                    }
                }
            } 
            else {
                std::string struct_name = docs.resolve_type_of_chain(path, obj_name, current_func_name, current_struct_name);
                
                if (!struct_name.empty()) {
                    bool include_private_for_self = (obj_name.rfind("self", 0) == 0); // Allow private members ONLY if accessing self.* chain
                    auto ff = docs.fields_of(path, struct_name, include_private_for_self);
                    for (const auto& f : ff) {
                        if (prefix.empty() || f.name.find(prefix) == 0) {
                            int k = (f.is_method) ? 2 : 5;
                            std::string sort_prio = (f.name.front() == '_') ? "1_" : "0_";
                            items.push_back(pyle::lsp::utils::make_obj({
                                {"label", f.name}, {"kind", k}, {"detail", f.detail},
                                {"insertText", f.name}, {"sortText", sort_prio + f.name}
                            }));
                        }
                    }
                }
            }
            send_comp(req["id"], items);
            return;
        }

        // 2. Local & Global Context autocomplete
        if (doc) {
            for (const auto& sym : doc->symbols) {
                if (sym.is_method) continue; 
                if (sym.is_local && sym.scope_func != current_func_name) continue; 
                if (!sym.name.empty() && sym.name[0] == '_') continue;
                if (!prefix.empty() && sym.name.find(prefix) != 0) continue;
                
                int k = 6;
                switch (sym.kind) {
                    case SymbolKind::Function:  k = 3; break;
                    case SymbolKind::Struct:    k = 22; break;
                    case SymbolKind::Module:    k = 9; break;
                    case SymbolKind::Variable:  k = 6; break;
                    case SymbolKind::Parameter: k = 6; break; 
                    default: k = 6;
                }
                
                // Prioritize locals heavily, fallback to 2_ for globals
                std::string sort_prio = sym.is_local ? "0_" : "2_";
                items.push_back(pyle::lsp::utils::make_obj({
                    {"label", sym.name}, {"kind", k}, {"detail", sym.detail},
                    {"sortText", sort_prio + sym.name}
                }));
            }
        }

        for (const auto& [kw, det] : LSP_KEYWORDS) {
            if (prefix.empty() || kw.find(prefix) == 0) {
                items.push_back(pyle::lsp::utils::make_obj({
                    {"label", kw}, {"kind", 14}, {"detail", det},
                    {"sortText", "3_" + kw}
                }));
            }
        }

        send_comp(req["id"], items);
    }

    void send_comp(json id, json& items) {
        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, {"id", id},
            {"result", pyle::lsp::utils::make_obj({{"isIncomplete", false}, {"items", items}})}
        }).dump());
    }

    void handle_hover(const json& req) {
        auto& p = req["params"];
        std::string path = pyle::lsp::utils::file_uri_to_path(p["textDocument"]["uri"]);
        Position pos{p["position"]["line"], p["position"]["character"]};

        Document* doc = docs.get(path);
        if (!doc) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        std::string word = docs.get_word_at(doc->source, pos);
        SymbolInfo* sym = docs.sym_by_name(path, word, pos);
        
        if (!sym) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        std::string text = "```pyle\n" + sym->detail + "\n```";
        if (!sym->file_path.empty()) {
            text += "\n\n" + sym->file_path + ":" + std::to_string(sym->selection_range.start.line + 1);
        }

        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, {"id", req["id"]},
            {"result", pyle::lsp::utils::make_obj({
                {"contents", pyle::lsp::utils::make_obj({{"kind", "markdown"}, {"value", text}})},
                {"range", pyle::lsp::utils::make_range(sym->selection_range.start.line, sym->selection_range.start.character, sym->selection_range.end.line, sym->selection_range.end.character)}
            })}
        }).dump());
    }

    void handle_definition(const json& req) {
        auto& p = req["params"];
        std::string path = pyle::lsp::utils::file_uri_to_path(p["textDocument"]["uri"]);
        Position pos{p["position"]["line"], p["position"]["character"]};

        Document* doc = docs.get(path);
        if (!doc) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        std::string word = docs.get_word_at(doc->source, pos);
        SymbolInfo* sym = docs.sym_by_name(path, word, pos);
        
        if (!sym) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        if (sym->file_path == path && sym->selection_range.start.line == pos.line) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, {"id", req["id"]},
            {"result", pyle::lsp::utils::make_obj({
                {"uri", pyle::lsp::utils::path_to_file_uri(sym->file_path)},
                {"range", pyle::lsp::utils::make_range(sym->selection_range.start.line, sym->selection_range.start.character, sym->selection_range.end.line, sym->selection_range.end.character)}
            })}
        }).dump());
    }

    void handle_doc_symbol(const json& req) {
        std::string path = pyle::lsp::utils::file_uri_to_path(req["params"]["textDocument"]["uri"]);
        Document* doc = docs.get(path);
        if (!doc) {
            JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", nullptr}}).dump());
            return;
        }

        json arr = json::array();
        for (const auto& sym : doc->symbols) {
            if (sym.is_method || sym.is_local) continue;
            int k = 13;
            switch (sym.kind) {
                case SymbolKind::Function: k = 12; break;
                case SymbolKind::Struct:   k = 23; break;
                case SymbolKind::Module:   k = 2; break;
                default: k = 13;
            }
            arr.push_back(pyle::lsp::utils::make_obj({
                {"name", sym.name}, {"kind", k}, {"detail", sym.detail},
                {"range", pyle::lsp::utils::make_range(sym.range.start.line, sym.range.start.character, sym.range.end.line, sym.range.end.character)},
                {"selectionRange", pyle::lsp::utils::make_range(sym.selection_range.start.line, sym.selection_range.start.character, sym.selection_range.end.line, sym.selection_range.end.character)}
            }));
        }

        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({{"jsonrpc", "2.0"}, {"id", req["id"]}, {"result", arr}}).dump());
    }

    void handle_semantic(const json& req) {
        std::string path = pyle::lsp::utils::file_uri_to_path(req["params"]["textDocument"]["uri"]);
        Document* doc = docs.get(path);
        json data = json::array();

        if (doc) {
            auto sorted = doc->symbols;
            std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
                if (a.selection_range.start.line != b.selection_range.start.line)
                    return a.selection_range.start.line < b.selection_range.start.line;
                return a.selection_range.start.character < b.selection_range.start.character;
            });

            auto tok_type = [](SymbolKind k) -> int {
                switch (k) {
                    case SymbolKind::Function:  return 0;
                    case SymbolKind::Struct:    return 1;
                    case SymbolKind::Parameter: return 3;
                    case SymbolKind::Field:     return 8;
                    case SymbolKind::Module:    return 9;
                    default: return 2;
                }
            };

            int prev_line = 0, prev_col = 0;
            for (const auto& sym : sorted) {
                int l = static_cast<int>(sym.selection_range.start.line);
                int c = static_cast<int>(sym.selection_range.start.character);
                int len = static_cast<int>(sym.name.size());
                
                if (l < prev_line) continue;
                
                data.push_back(l - prev_line);
                data.push_back((l == prev_line) ? (c - prev_col) : c);
                data.push_back(len);
                data.push_back(tok_type(sym.kind));
                data.push_back(0);
                prev_line = l;
                prev_col = c + len;
            }
        }

        JsonRpcTransport::write_message(pyle::lsp::utils::make_obj({
            {"jsonrpc", "2.0"}, {"id", req["id"]},
            {"result", pyle::lsp::utils::make_obj({{"data", data}})}
        }).dump());
    }
};

} // namespace pyle::lsp