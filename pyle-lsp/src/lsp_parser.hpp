#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include <pyle/lexer.hpp>
#include <pyle/token.hpp>
#include <string>
#include <vector>
#include <sstream>

namespace pyle::lsp {

class FuzzyParser {
public:
    static void parse(Document& doc) {
        doc.symbols.clear();
        doc.imports.clear();
        doc.import_paths.clear();
        
        pyle::Lexer lexer(doc.source, doc.reporter);
        std::vector<pyle::Token> tokens;
        try {
            tokens = lexer.tokenize();
        } catch (...) {
            return;
        }

        struct ActiveScope {
            int brace_depth;
            size_t symbol_index;
            std::string name;
        };

        std::vector<ActiveScope> active_funcs;
        std::vector<ActiveScope> active_structs;

        std::string current_func = "";
        std::string current_struct = "";
        int current_depth = 0;

        auto get_ident = [&](size_t idx) -> std::string {
            if (idx < tokens.size() && tokens[idx].type == pyle::TokenType::IDENTIFIER)
                return std::string(tokens[idx].lexeme);
            return "";
        };

        for (size_t i = 0; i < tokens.size(); i++) {
            const auto& t = tokens[i];
            
            if (t.type == pyle::TokenType::LEFT_BRACE) {
                current_depth++;
            }
            else if (t.type == pyle::TokenType::RIGHT_BRACE) {
                current_depth--;

                while (!active_funcs.empty() && active_funcs.back().brace_depth > current_depth) {
                    doc.symbols[active_funcs.back().symbol_index].range.end.line = t.selection.line;
                    doc.symbols[active_funcs.back().symbol_index].range.end.character = t.selection.column + 1;
                    active_funcs.pop_back();
                }
                current_func = active_funcs.empty() ? "" : active_funcs.back().name;

                while (!active_structs.empty() && active_structs.back().brace_depth > current_depth) {
                    doc.symbols[active_structs.back().symbol_index].range.end.line = t.selection.line;
                    doc.symbols[active_structs.back().symbol_index].range.end.character = t.selection.column + 1;
                    active_structs.pop_back();
                }
                current_struct = active_structs.empty() ? "" : active_structs.back().name;
            }

            if (t.type == pyle::TokenType::STRUCT) {
                std::string struct_name = get_ident(i + 1);
                if (!struct_name.empty()) {
                    current_struct = struct_name;
                    size_t search = i + 2;
                    std::string params_detail = "";
                    
                    while (search < tokens.size() && tokens[search].type != pyle::TokenType::LEFT_BRACE && tokens[search].type != pyle::TokenType::RIGHT_PAREN) {
                        if (tokens[search].type == pyle::TokenType::IDENTIFIER) {
                            std::string field_name = std::string(tokens[search].lexeme);
                            SymbolInfo field_info;
                            field_info.name = field_name;
                            field_info.kind = SymbolKind::Field;
                            field_info.detail = "self." + field_name;
                            field_info.file_path = doc.file_path;
                            field_info.range = {{tokens[search].selection.line, tokens[search].selection.column},
                                                {tokens[search].selection.line, tokens[search].selection.column + field_name.size()}};
                            field_info.selection_range = field_info.range;
                            field_info.parent_struct = current_struct;
                            
                            if (search + 1 < tokens.size() && tokens[search + 1].type == pyle::TokenType::COLON) {
                                std::string hint = get_ident(search + 2);
                                if (!hint.empty()) {
                                    field_info.type_name = hint;
                                    field_info.has_type_hint = true;
                                    search += 2;
                                }
                            }
                            
                            doc.symbols.push_back(field_info);
                            
                            if (!params_detail.empty()) params_detail += ", ";
                            params_detail += field_name;
                        }
                        search++;
                    }

                    SymbolInfo s_info;
                    s_info.name = struct_name;
                    s_info.kind = SymbolKind::Struct;
                    s_info.detail = "struct " + struct_name + "(" + params_detail + ")";
                    s_info.file_path = doc.file_path;
                    s_info.range = {{t.selection.line, t.selection.column}, {t.selection.line + 100, 0}};
                    s_info.selection_range = {{tokens[i + 1].selection.line, tokens[i + 1].selection.column},
                                              {tokens[i + 1].selection.line, tokens[i + 1].selection.column + struct_name.size()}};
                    
                    size_t s_idx = doc.symbols.size();
                    doc.symbols.push_back(s_info);
                    active_structs.push_back({current_depth + 1, s_idx, struct_name});
                }
            }
            else if (t.type == pyle::TokenType::FN) {
                std::string func_name = get_ident(i + 1);
                if (!func_name.empty()) {
                    current_func = func_name;
                    bool is_method = !current_struct.empty();
                    size_t search = i + 2;
                    std::string params_detail = "";
                    
                    while (search < tokens.size() && tokens[search].type != pyle::TokenType::LEFT_BRACE && tokens[search].type != pyle::TokenType::ARROW) {
                        if (tokens[search].type == pyle::TokenType::IDENTIFIER) {
                            std::string param_name = std::string(tokens[search].lexeme);
                            if (param_name != "self") {
                                SymbolInfo p_info;
                                p_info.name = param_name;
                                p_info.kind = SymbolKind::Parameter;
                                p_info.detail = "parameter " + param_name;
                                p_info.file_path = doc.file_path;
                                p_info.range = {{tokens[search].selection.line, tokens[search].selection.column},
                                                {tokens[search].selection.line, tokens[search].selection.column + param_name.size()}};
                                p_info.selection_range = p_info.range;
                                p_info.is_local = true;
                                p_info.scope_func = current_func;
                                
                                if (search + 1 < tokens.size() && tokens[search + 1].type == pyle::TokenType::COLON) {
                                    std::string hint = get_ident(search + 2);
                                    if (!hint.empty()) {
                                        p_info.type_name = hint;
                                        p_info.has_type_hint = true;
                                        search += 2;
                                    }
                                }
                                
                                doc.symbols.push_back(p_info);
                                
                                if (!params_detail.empty()) params_detail += ", ";
                                params_detail += param_name;
                            }
                        }
                        search++;
                    }

                    SymbolInfo f_info;
                    f_info.name = func_name;
                    f_info.kind = is_method ? SymbolKind::Field : SymbolKind::Function;
                    f_info.detail = "fn " + func_name + "(" + params_detail + ")";
                    f_info.file_path = doc.file_path;
                    f_info.range = {{t.selection.line, t.selection.column}, {t.selection.line + 50, 0}};
                    f_info.selection_range = {{tokens[i + 1].selection.line, tokens[i + 1].selection.column},
                                              {tokens[i + 1].selection.line, tokens[i + 1].selection.column + func_name.size()}};
                    f_info.is_method = is_method;
                    f_info.parent_struct = current_struct;
                    
                    size_t f_idx = doc.symbols.size();
                    doc.symbols.push_back(f_info);
                    active_funcs.push_back({current_depth + 1, f_idx, func_name});
                }
            }
            else if (t.type == pyle::TokenType::LET) {
                std::string var_name = get_ident(i + 1);
                if (!var_name.empty()) {
                    std::string type_hint = "";
                    size_t eq_pos = i + 2;
                    if (i + 2 < tokens.size() && tokens[i + 2].type == pyle::TokenType::COLON) {
                        type_hint = get_ident(i + 3);
                        eq_pos = i + 4;
                    }
                    
                    bool is_import = false;
                    std::string import_val = "";
                    if (eq_pos + 3 < tokens.size() && 
                        tokens[eq_pos].type == pyle::TokenType::EQUAL && 
                        tokens[eq_pos + 1].type == pyle::TokenType::IDENTIFIER && 
                        tokens[eq_pos + 1].lexeme == "import" &&
                        tokens[eq_pos + 2].type == pyle::TokenType::LEFT_PAREN && 
                        tokens[eq_pos + 3].type == pyle::TokenType::STRING) {
                        is_import = true;
                        import_val = pyle::lsp::utils::strip_quotes(std::string(tokens[eq_pos + 3].lexeme));
                    }

                    SymbolInfo v_info;
                    v_info.name = var_name;
                    v_info.file_path = doc.file_path;
                    v_info.range = {{t.selection.line, t.selection.column},
                                    {tokens[i + 1].selection.line, tokens[i + 1].selection.column + var_name.size()}};
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
                        
                        if (!type_hint.empty()) {
                            v_info.type_name = type_hint;
                            v_info.has_type_hint = true;
                        }

                        if (eq_pos < tokens.size() && tokens[eq_pos].type == pyle::TokenType::EQUAL) {
                            size_t init_start = eq_pos + 1;
                            std::string init_chain = "";
                            while (init_start < tokens.size() && 
                                   tokens[init_start].type != pyle::TokenType::SEMICOLON && 
                                   tokens[init_start].selection.line == t.selection.line) {
                                
                                if (tokens[init_start].type == pyle::TokenType::IDENTIFIER || 
                                    tokens[init_start].type == pyle::TokenType::DOT) {
                                    init_chain += std::string(tokens[init_start].lexeme);
                                } else if (tokens[init_start].type == pyle::TokenType::LEFT_PAREN) {
                                    init_chain += "()";
                                    break;
                                } else {
                                    break;
                                }
                                init_start++;
                            }
                            if (v_info.type_name.empty()) {
                                v_info.type_name = init_chain;
                            }
                        }
                    }
                    doc.symbols.push_back(v_info);
                }
            }
            else if (t.type == pyle::TokenType::IDENTIFIER && t.lexeme == "self" && !current_struct.empty()) {
                std::string field_name = get_ident(i + 2);
                if (!field_name.empty()) {
                    bool has_self_type_hint = false;
                    std::string self_type_hint = "";
                    size_t eq_index = i + 3;
                    if (i + 3 < tokens.size() && tokens[i + 3].type == pyle::TokenType::COLON) {
                        self_type_hint = get_ident(i + 4);
                        if (!self_type_hint.empty()) {
                            has_self_type_hint = true;
                            eq_index = i + 5;
                        }
                    }

                    if (eq_index < tokens.size() && tokens[eq_index].type == pyle::TokenType::EQUAL) {
                        size_t val_start = eq_index + 1;
                        std::string val_chain = "";
                        while (val_start < tokens.size() && 
                               tokens[val_start].type != pyle::TokenType::SEMICOLON && 
                               tokens[val_start].selection.line == t.selection.line) {
                            
                            if (tokens[val_start].type == pyle::TokenType::IDENTIFIER || 
                                tokens[val_start].type == pyle::TokenType::DOT) {
                                val_chain += std::string(tokens[val_start].lexeme);
                            } else if (tokens[val_start].type == pyle::TokenType::LEFT_PAREN) {
                                val_chain += "()";
                                break;
                            } else {
                                break;
                            }
                            val_start++;
                        }

                        bool should_set = has_self_type_hint || !val_chain.empty();
                        if (should_set) {
                            bool found = false;
                            for (auto& sym : doc.symbols) {
                                if (sym.kind == SymbolKind::Field && sym.parent_struct == current_struct && sym.name == field_name) {
                                    if (has_self_type_hint) {
                                        sym.type_name = self_type_hint;
                                        sym.has_type_hint = true;
                                    } else if (!sym.has_type_hint) {
                                        bool is_bare_var = val_chain.find('.') == std::string::npos && val_chain.find('(') == std::string::npos;
                                        if (is_bare_var && !sym.type_name.empty()) {
                                            // preserve existing type_name, don't overwrite with bare var name
                                        } else {
                                            sym.type_name = val_chain;
                                        }
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
                                f_info.range = {{tokens[i+2].selection.line, tokens[i+2].selection.column},
                                                {tokens[i+2].selection.line, tokens[i+2].selection.column + field_name.size()}};
                                f_info.selection_range = f_info.range;
                                f_info.parent_struct = current_struct;
                                if (has_self_type_hint) {
                                    f_info.type_name = self_type_hint;
                                    f_info.has_type_hint = true;
                                } else {
                                    f_info.type_name = val_chain;
                                }
                                doc.symbols.push_back(f_info);
                            }
                        }
                    }
                }
            }
            else if (t.type == pyle::TokenType::RETURN && !active_funcs.empty()) {
                size_t ret_start = i + 1;
                std::string ret_chain = "";
                while (ret_start < tokens.size() && 
                       tokens[ret_start].type != pyle::TokenType::SEMICOLON && 
                       tokens[ret_start].type != pyle::TokenType::RIGHT_BRACE && 
                       tokens[ret_start].selection.line == t.selection.line) {
                    
                    if (tokens[ret_start].type == pyle::TokenType::IDENTIFIER || 
                        tokens[ret_start].type == pyle::TokenType::DOT) {
                        ret_chain += std::string(tokens[ret_start].lexeme);
                    } else if (tokens[ret_start].type == pyle::TokenType::LEFT_PAREN) {
                        ret_chain += "()";
                        break;
                    } else {
                        break;
                    }
                    ret_start++;
                }
                if (!ret_chain.empty()) {
                    doc.symbols[active_funcs.back().symbol_index].type_name = ret_chain;
                }
            }
            else if (t.type == pyle::TokenType::IDENTIFIER && t.lexeme == "add_import_path") {
                if (i + 2 < tokens.size() && tokens[i+1].type == pyle::TokenType::LEFT_PAREN && tokens[i+2].type == pyle::TokenType::STRING) {
                    doc.import_paths.push_back(pyle::lsp::utils::strip_quotes(std::string(tokens[i+2].lexeme)));
                }
            }
        }
    }
};

} // namespace pyle::lsp