#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include "lsp_document.hpp"
#include <LibLsp/LspCpp.h>
#include <LibLsp/lsp/general/initialize.h>
#include <LibLsp/lsp/general/exit.h>
#include <LibLsp/lsp/textDocument/did_open.h>
#include <LibLsp/lsp/textDocument/did_change.h>
#include <LibLsp/lsp/textDocument/did_close.h>
#include <LibLsp/lsp/textDocument/completion.h>
#include <LibLsp/lsp/textDocument/hover.h>
#include <LibLsp/lsp/textDocument/declaration_definition.h>
#include <LibLsp/lsp/textDocument/document_symbol.h>
#include <LibLsp/lsp/textDocument/SemanticTokens.h>
#include <LibLsp/lsp/textDocument/publishDiagnostics.h>
#include <LibLsp/lsp/textDocument/document_symbol.h>

#include <mutex>
#include <condition_variable>
#include <algorithm>

namespace pyle::lsp {

class LspServer {
    DocumentManager docs;
    ::lsp::LanguageSession session;
    std::mutex mtx;
    std::condition_variable cv;
    bool exit_flag = false;

public:
    DocumentManager& document_manager() { return docs; }

    void run() {
        docs.on_diagnostics = [this](const std::string& uri) {
            Notify_TextDocumentPublishDiagnostics::notify notify;
            notify.params.uri = lsDocumentUri::FromUri(pyle::lsp::utils::path_to_file_uri(uri));
            session.send(notify);
        };

        session.on([this](td_initialize::request const& req) {
            return handle_init(req);
        });
        
        session.on([this](Notify_Exit::notify const&) {
            std::lock_guard<std::mutex> lock(mtx);
            exit_flag = true;
            cv.notify_all();
        });

        session.on([this](Notify_TextDocumentDidOpen::notify const& notify) {
            docs.open(notify.params.textDocument.uri.GetAbsolutePath(), notify.params.textDocument.text);
        });

        session.on([this](Notify_TextDocumentDidChange::notify const& notify) {
            if (!notify.params.contentChanges.empty()) {
                docs.change(notify.params.textDocument.uri.GetAbsolutePath(), notify.params.contentChanges[0].text);
            }
        });

        session.on([this](Notify_TextDocumentDidClose::notify const& notify) {
            docs.close(notify.params.textDocument.uri.GetAbsolutePath());
        });

        session.on([this](td_completion::request const& req) {
            return handle_completion(req);
        });

        session.on([this](td_hover::request const& req) {
            return handle_hover(req);
        });

        session.on([this](td_definition::request const& req) {
            return handle_definition(req);
        });

        session.on([this](td_symbol::request const& req) {
            return handle_doc_symbol(req);
        });;

        session.on([this](td_semanticTokens_full::request const& req) {
            return handle_semantic(req);
        });

        session.startStdio();

        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return exit_flag; });

        session.stop();
    }

private:
    td_initialize::response handle_init(const td_initialize::request& req) {
        if (req.params.rootUri) {
            docs.root_path = req.params.rootUri->GetAbsolutePath();
        }

        td_initialize::response rsp;
        rsp.id = req.id;
        
        rsp.result.capabilities.textDocumentSync = lsTextDocumentSyncOptions();
        rsp.result.capabilities.textDocumentSync->openClose = true;
        rsp.result.capabilities.textDocumentSync->change = lsTextDocumentSyncKind::Full;
        
        rsp.result.capabilities.completionProvider = lsCompletionOptions();
        rsp.result.capabilities.completionProvider->triggerCharacters = {".", ":"};
        
        rsp.result.capabilities.hoverProvider = true;
        rsp.result.capabilities.definitionProvider = true;
        rsp.result.capabilities.documentSymbolProvider = true;
        
        lsSemanticTokensOptions semOpts;
        semOpts.full = true;
        semOpts.legend.tokenTypes = {"function", "struct", "variable", "parameter", "keyword", "string", "number", "comment", "property", "module"};
        rsp.result.capabilities.semanticTokensProvider = semOpts;

        return rsp;
    }

    td_completion::response handle_completion(const td_completion::request& req) {
        td_completion::response rsp;
        rsp.id = req.id;
        rsp.result.isIncomplete = false;

        std::string path = req.params.textDocument.uri.GetAbsolutePath();
        size_t line = req.params.position.line;
        size_t ch = req.params.position.character;

        Document* doc = docs.get(path);
        if (!doc) return rsp;

        auto lines = pyle::lsp::utils::get_lines(doc->source);
        std::string text_before = (line < lines.size()) ? lines[line].substr(0, ch) : "";

        std::string obj_name, prefix;
        
        pyle::ErrorReporter rep("", "");
        pyle::Lexer lexer(text_before, rep);
        auto tokens = lexer.tokenize();
        if (!tokens.empty() && tokens.back().type == pyle::TokenType::EOF_TOKEN) {
            tokens.pop_back();
        }

        if (!tokens.empty()) {
            auto last = tokens.back();
            if (last.type == pyle::TokenType::DOT) {
                prefix = "";
                obj_name = docs.extract_chain_backwards(tokens, tokens.size() - 2);
            } else {
                prefix = std::string(last.lexeme);
                if (tokens.size() > 1 && tokens[tokens.size() - 2].type == pyle::TokenType::DOT) {
                    obj_name = docs.extract_chain_backwards(tokens, tokens.size() - 3);
                } else {
                    obj_name = "";
                }
            }
        }

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
                            lsCompletionItem item;
                            item.label = f.name;
                            item.kind = f.is_method ? lsCompletionItemKind::Method : lsCompletionItemKind::Field;
                            item.detail = f.detail;
                            item.insertText = f.name;
                            item.sortText = (f.name.front() == '_') ? "1_" + f.name : "0_" + f.name;
                            rsp.result.items.push_back(item);
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
                                lsCompletionItem item;
                                item.label = sym.name;
                                item.kind = (sym.kind == SymbolKind::Struct) ? lsCompletionItemKind::Struct : lsCompletionItemKind::Function;
                                item.detail = sym.detail;
                                item.sortText = "0_" + sym.name;
                                rsp.result.items.push_back(item);
                            }
                        }
                    }
                }
            } 
            else {
                std::string literal_type = "";
                char fc = obj_name.empty() ? 0 : obj_name[0];
                char lc = obj_name.empty() ? 0 : obj_name.back();
                if ((fc == '"' || fc == '\'') && fc == lc) {
                    literal_type = "string";
                } else if (fc == '[') {
                    literal_type = "array";
                } else if (fc == '{') {
                    literal_type = "map";
                } else if (obj_name == "true" || obj_name == "false") {
                    literal_type = "bool";
                } else if (!obj_name.empty()) {
                    bool is_num = true, is_float = false;
                    for (size_t ci = 0; ci < obj_name.size() && is_num; ++ci) {
                        char c = obj_name[ci];
                        if (ci == 0 && c == '-') continue;
                        if (c == '.') { is_float = true; continue; }
                        if (!isdigit(static_cast<unsigned char>(c))) is_num = false;
                    }
                    if (is_num) literal_type = is_float ? "float" : "int";
                }
                
                std::string struct_name = literal_type.empty()
                    ? docs.resolve_type_of_chain(path, obj_name, current_func_name, current_struct_name)
                    : literal_type;
                
                if (!struct_name.empty()) {
                    bool include_private_for_self = (obj_name.rfind("self", 0) == 0);
                    auto ff = docs.fields_of(path, struct_name, include_private_for_self);
                    for (const auto& f : ff) {
                        if (prefix.empty() || f.name.find(prefix) == 0) {
                            lsCompletionItem item;
                            item.label = f.name;
                            item.kind = f.is_method ? lsCompletionItemKind::Method : lsCompletionItemKind::Field;
                            item.detail = f.detail;
                            item.insertText = f.name;
                            item.sortText = (f.name.front() == '_') ? "1_" + f.name : "0_" + f.name;
                            rsp.result.items.push_back(item);
                        }
                    }
                }
            }
            return rsp;
        }

        if (doc) {
            for (const auto& sym : doc->symbols) {
                if (sym.is_method) continue; 
                if (sym.is_local && sym.scope_func != current_func_name) continue; 
                if (!sym.name.empty() && sym.name[0] == '_') continue;
                if (!prefix.empty() && sym.name.find(prefix) != 0) continue;
                
                lsCompletionItem item;
                item.label = sym.name;
                switch (sym.kind) {
                    case SymbolKind::Function:  item.kind = lsCompletionItemKind::Function; break;
                    case SymbolKind::Struct:    item.kind = lsCompletionItemKind::Struct; break;
                    case SymbolKind::Module:    item.kind = lsCompletionItemKind::Module; break;
                    case SymbolKind::Variable:  
                    case SymbolKind::Parameter: item.kind = lsCompletionItemKind::Variable; break; 
                    default: item.kind = lsCompletionItemKind::Variable;
                }
                item.detail = sym.detail;
                item.sortText = sym.is_local ? "0_" + sym.name : "2_" + sym.name;
                rsp.result.items.push_back(item);
            }
        }

        for (const auto& [kw, det] : LSP_KEYWORDS) {
            if (prefix.empty() || kw.find(prefix) == 0) {
                lsCompletionItem item;
                item.label = kw;
                item.kind = lsCompletionItemKind::Keyword;
                item.detail = det;
                item.sortText = "3_" + kw;
                rsp.result.items.push_back(item);
            }
        }

        return rsp;
    }

    td_hover::response handle_hover(const td_hover::request& req) {
        td_hover::response rsp;
        rsp.id = req.id;
        
        std::string path = req.params.textDocument.uri.GetAbsolutePath();
        Position pos{req.params.position.line, req.params.position.character};

        Document* doc = docs.get(path);
        if (!doc) return rsp;

        std::string word = docs.get_word_at(doc->source, pos);
        SymbolInfo* sym = docs.sym_by_name(path, word, pos);
        if (!sym) return rsp;

        std::string text = "```pyle\n" + sym->detail + "\n```";
        if (!sym->file_path.empty()) {
            text += "\n\n*" + sym->file_path + ":" + std::to_string(sym->selection_range.start.line + 1) + "*";
        }

        lsHover hover;
        lsMarkupContent mc;
        mc.kind = "markdown";
        mc.value = text;
        hover.contents = mc;
        
        lsRange range;
        range.start.line = sym->selection_range.start.line;
        range.start.character = sym->selection_range.start.character;
        range.end.line = sym->selection_range.end.line;
        range.end.character = sym->selection_range.end.character;
        hover.range = range;
        
        rsp.result = hover;
        return rsp;
    }

    td_definition::response handle_definition(const td_definition::request& req) {
        td_definition::response rsp;
        rsp.id = req.id;
        
        std::string path = req.params.textDocument.uri.GetAbsolutePath();
        Position pos{req.params.position.line, req.params.position.character};

        Document* doc = docs.get(path);
        if (!doc) return rsp;

        std::string word = docs.get_word_at(doc->source, pos);
        SymbolInfo* sym = docs.sym_by_name(path, word, pos);
        
        if (!sym || (sym->file_path == path && sym->selection_range.start.line == pos.line)) {
            return rsp;
        }

        std::vector<lsLocation> locs;
        lsLocation loc;
        loc.uri = lsDocumentUri::FromUri(pyle::lsp::utils::path_to_file_uri(sym->file_path));
        loc.range.start.line = sym->selection_range.start.line;
        loc.range.start.character = sym->selection_range.start.character;
        loc.range.end.line = sym->selection_range.end.line;
        loc.range.end.character = sym->selection_range.end.character;
        locs.push_back(loc);
        
        rsp.result = locs;
        return rsp;
    }

    td_symbol::response handle_doc_symbol(const td_symbol::request& req) {
        td_documentSymbol::response rsp;
        rsp.id = req.id;

        std::string path = req.params.textDocument.uri.GetAbsolutePath();
        Document* doc = docs.get(path);
        if (!doc) return rsp;

        std::vector<lsDocumentSymbol> symbols;
        for (const auto& sym : doc->symbols) {
            if (sym.is_method || sym.is_local) continue;
            
            lsDocumentSymbol docSym;
            docSym.name = sym.name;
            docSym.detail = sym.detail;
            
            switch (sym.kind) {
                case SymbolKind::Function: docSym.kind = lsSymbolKind::Function; break;
                case SymbolKind::Struct:   docSym.kind = lsSymbolKind::Struct; break;
                case SymbolKind::Module:   docSym.kind = lsSymbolKind::Module; break;
                default: docSym.kind = lsSymbolKind::Variable;
            }

            docSym.range.start.line = sym.range.start.line;
            docSym.range.start.character = sym.range.start.character;
            docSym.range.end.line = sym.range.end.line;
            docSym.range.end.character = sym.range.end.character;

            docSym.selectionRange.start.line = sym.selection_range.start.line;
            docSym.selectionRange.start.character = sym.selection_range.start.character;
            docSym.selectionRange.end.line = sym.selection_range.end.line;
            docSym.selectionRange.end.character = sym.selection_range.end.character;

            symbols.push_back(docSym);
        }
        
        rsp.result = symbols;
        return rsp;
    }

    td_semanticTokens_full::response handle_semantic(const td_semanticTokens_full::request& req) {
        td_semanticTokens_full::response rsp;
        rsp.id = req.id;
        
        std::string path = req.params.textDocument.uri.GetAbsolutePath();
        Document* doc = docs.get(path);
        if (!doc) return rsp;

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
        lsSemanticTokens tokens;
        
        for (const auto& sym : sorted) {
            int l = static_cast<int>(sym.selection_range.start.line);
            int c = static_cast<int>(sym.selection_range.start.character);
            int len = static_cast<int>(sym.name.size());
            
            if (l < prev_line) continue;
            
            tokens.data.push_back(l - prev_line);
            tokens.data.push_back((l == prev_line) ? (c - prev_col) : c);
            tokens.data.push_back(len);
            tokens.data.push_back(tok_type(sym.kind));
            tokens.data.push_back(0); 
            
            prev_line = l;
            
            prev_col = c;
        }

        rsp.result = tokens;
        return rsp;
    }
};

} 