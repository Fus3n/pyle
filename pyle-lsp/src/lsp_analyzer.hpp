#pragma once

#include "lsp_types.hpp"
#include "lsp_utils.hpp"
#include <pyle/lexer.hpp>
#include <pyle/parser.hpp>
#include <pyle/compiler.hpp>
#include <pyle/vm.hpp>
#include <memory>
#include <vector>
#include <algorithm>

namespace pyle::lsp {

inline std::string expr_to_chain_string(const pyle::Expr* expr) {
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
    return "";
}

struct ChainPart {
    std::string text;
    bool is_call = false;
};

inline std::vector<ChainPart> tokenize_chain(const std::string& chain) {
    std::vector<ChainPart> parts;
    size_t i = 0;
    while (i < chain.size()) {
        char c = chain[i];
        if (c == '"' || c == '\'') {
            size_t j = i + 1;
            while (j < chain.size()) {
                if (chain[j] == '\\') { j += 2; continue; }
                if (chain[j] == c) { j++; break; }
                j++;
            }
            parts.push_back({chain.substr(i, j - i), false});
            i = j;
        } else if (isalnum(static_cast<unsigned char>(c)) || c == '_') {
            size_t j = i;
            while (j < chain.size() && (isalnum(static_cast<unsigned char>(chain[j])) || chain[j] == '_')) j++;
            parts.push_back({chain.substr(i, j - i), false});
            i = j;
        } else if (c == '.' || c == ' ') {
            i++;
        } else if (c == '(' || c == '[' || c == '{') {
            char open = c, close = (c == '(') ? ')' : (c == '[') ? ']' : '}';
            int depth = 1;
            size_t j = i + 1;
            while (j < chain.size() && depth > 0) {
                if (chain[j] == open) depth++;
                else if (chain[j] == close) depth--;
                j++;
            }
            if (parts.empty()) {
                parts.push_back({chain.substr(i, j - i), c == '('});
            } else {
                parts.back().is_call = true;
            }
            i = j;
        } else {
            i++;
        }
    }
    return parts;
}

class Analyzer {
public:
    static void parse(DocumentModel& doc) {
        std::vector<SymbolInfo> previous;
        previous.swap(doc.symbols);
        doc.struct_members.clear();
        doc.struct_symbols.clear();
        doc.imports.clear();
        doc.import_paths.clear();
        doc.ast.clear();
        doc.reporter.clear();

        pyle::Lexer lexer(doc.source, doc.reporter);
        auto tokens = lexer.tokenize();
        doc.tokens = tokens;

        pyle::Parser parser(tokens, doc.reporter);
        doc.ast = parser.parse();

        Ctx c;
        walk_stmts(doc.ast, doc, c);

        if (doc.reporter.has_errors() && !previous.empty() &&
            doc.symbols.size() < previous.size()) {
            doc.symbols = std::move(previous);
        }

        for (auto& s : doc.symbols) {
            if (!s.parent_struct.empty()) {
                doc.struct_members[s.parent_struct].push_back(&s);
            }
            if (s.kind == SymbolKind::Struct) {
                doc.struct_symbols[s.name] = &s;
            }
        }

        if (!doc.reporter.has_errors()) {
            try {
                pyle::VM vm;
                pyle::Compiler compiler(vm, doc.reporter);
                compiler.compile(doc.ast);
            } catch (...) {}
        }
    }

private:
    struct Ctx {
        std::string current_struct;
        std::string current_func;
        size_t current_func_symbol = SIZE_MAX;
        std::vector<int> block_ends;
    };

    static Ctx ctx() { return Ctx{}; }

    static void walk_stmts(const std::vector<std::unique_ptr<pyle::Stmt>>& stmts, DocumentModel& doc, Ctx& c) {
        for (const auto& s : stmts) {
            if (s) walk_stmt(s.get(), doc, c);
        }
    }

    static void walk_stmt(pyle::Stmt* stmt, DocumentModel& doc, Ctx& c) {
        if (auto* vd = dynamic_cast<pyle::VarDeclStmt*>(stmt)) {
            walk_var_decl(vd, doc, c);
        } else if (auto* fd = dynamic_cast<pyle::FuncDeclStmt*>(stmt)) {
            walk_func_decl(fd, doc, c, nullptr);
        } else if (auto* sd = dynamic_cast<pyle::StructDeclStmt*>(stmt)) {
            walk_struct_decl(sd, doc, c);
        } else if (auto* blk = dynamic_cast<pyle::BlockStmt*>(stmt)) {
            c.block_ends.push_back(blk->close_line);
            walk_stmts(blk->statements, doc, c);
            c.block_ends.pop_back();
        } else if (auto* iff = dynamic_cast<pyle::IfStmt*>(stmt)) {
            if (iff->then_branch) walk_stmt(iff->then_branch.get(), doc, c);
            if (iff->else_branch) walk_stmt(iff->else_branch.get(), doc, c);
        } else if (auto* wh = dynamic_cast<pyle::WhileStmt*>(stmt)) {
            if (wh->body) walk_stmt(wh->body.get(), doc, c);
        } else if (auto* fr = dynamic_cast<pyle::ForStmt*>(stmt)) {
            SymbolInfo v;
            v.name = std::string(fr->var_name.lexeme);
            v.kind = SymbolKind::Variable;
            v.detail = "let " + v.name;
            v.type_name = ANY_TYPE;
            v.file_path = doc.file_path;
            v.range = {{fr->var_name.selection.line, fr->var_name.selection.column},
                       {fr->var_name.selection.line, fr->var_name.selection.column + v.name.size()}};
            v.selection_range = v.range;
            v.is_local = !c.current_func.empty();
            v.scope_func = c.current_func;
            v.scope_end_line = c.block_ends.empty() ? -1 : c.block_ends.back();
            doc.symbols.push_back(v);
            if (fr->body) walk_stmt(fr->body.get(), doc, c);
        } else if (auto* ret = dynamic_cast<pyle::ReturnStmt*>(stmt)) {
            if (ret->value && c.current_func_symbol != SIZE_MAX) {
                std::string t = expr_to_chain_string(ret->value.get());
                if (!t.empty() && doc.symbols[c.current_func_symbol].type_name.empty()) {
                    doc.symbols[c.current_func_symbol].type_name = t;
                }
            }
        } else if (auto* es = dynamic_cast<pyle::ExpressionStmt*>(stmt)) {
            walk_expression(es->expression.get(), doc, c);
        }
    }

    static void walk_var_decl(pyle::VarDeclStmt* vd, DocumentModel& doc, Ctx& c) {
        std::string name(vd->name.lexeme);
        std::string init_chain = vd->initializer ? expr_to_chain_string(vd->initializer.get()) : "";

        if (vd->initializer) {
            if (auto* call = dynamic_cast<pyle::CallExpr*>(vd->initializer.get())) {
                if (auto* callee = dynamic_cast<pyle::VariableExpr*>(call->callee.get())) {
                    if (callee->name.lexeme == "import" && !call->args.empty()) {
                        if (auto* lit = dynamic_cast<pyle::LiteralExpr*>(call->args[0].get())) {
                            std::string module = pyle::lsp::utils::strip_quotes(std::string(lit->token.lexeme));
                            SymbolInfo v;
                            v.name = name;
                            v.kind = SymbolKind::Module;
                            v.detail = "import(\"" + module + "\")";
                            v.type_name = "module:" + module;
                            v.file_path = doc.file_path;
                            v.range = {{vd->name.selection.line, vd->name.selection.column},
                                       {vd->name.selection.line, vd->name.selection.column + name.size()}};
                            v.selection_range = v.range;
                            doc.symbols.push_back(v);
                            doc.imports[name] = module;
                            return;
                        }
                    }
                }
            }
        }

        std::string type = vd->type_annotation.empty() ? init_chain : vd->type_annotation;

        SymbolInfo v;
        v.name = name;
        v.kind = SymbolKind::Variable;
        v.detail = "let " + name;
        v.type_name = type;
        v.has_type_hint = !vd->type_annotation.empty();
        v.file_path = doc.file_path;
        v.range = {{vd->name.selection.line, vd->name.selection.column},
                   {vd->name.selection.line, vd->name.selection.column + name.size()}};
        v.selection_range = v.range;
        v.is_local = !c.current_func.empty();
        v.scope_func = c.current_func;
        v.scope_end_line = c.block_ends.empty() ? -1 : c.block_ends.back();
        doc.symbols.push_back(v);
    }

    static std::string format_func_detail(const pyle::FuncDeclStmt* fd, const std::string& name) {
        std::string detail = "fn " + name + "(";
        for (size_t pi = 0; pi < fd->params.size(); ++pi) {
            if (pi > 0) detail += ", ";
            detail += std::string(fd->params[pi].lexeme);
            if (pi < fd->param_types.size() && !fd->param_types[pi].empty()) {
                detail += ": " + fd->param_types[pi];
            }
        }
        detail += ")";
        if (!fd->return_type.empty()) detail += " -> " + fd->return_type;
        return detail;
    }

    static void walk_func_decl(pyle::FuncDeclStmt* fd, DocumentModel& doc, Ctx& c, pyle::StructDeclStmt* owner) {
        std::string name(fd->name.lexeme);
        bool is_method = (owner != nullptr);
        bool is_static = false;
        if (owner) {
            is_static = fd->params.empty() || fd->params[0].lexeme != "self";
        }

        SymbolInfo f;
        f.name = name;
        f.kind = is_method ? (is_static ? SymbolKind::StaticFunction : SymbolKind::Method) : SymbolKind::Function;
        f.detail = format_func_detail(fd, name);
        f.file_path = doc.file_path;
        int fn_end = (fd->body && fd->body->close_line >= 0) ? fd->body->close_line : fd->name.selection.line + 500;
        size_t end_col = (fn_end == static_cast<int>(fd->name.selection.line))
            ? fd->name.selection.column + name.size() : 0;
        f.range = {{fd->name.selection.line, fd->name.selection.column},
                   {static_cast<size_t>(fn_end), end_col}};
        f.selection_range = {{fd->name.selection.line, fd->name.selection.column},
                             {fd->name.selection.line, fd->name.selection.column + name.size()}};
        f.type_name = fd->return_type;
        f.has_type_hint = !fd->return_type.empty();
        f.parent_struct = is_method ? std::string(owner->name.lexeme) : "";
        f.is_static = is_static;

        size_t f_idx = doc.symbols.size();
        doc.symbols.push_back(f);

        if (owner && !fd->params.empty() && fd->params[0].lexeme == "self") {
            SymbolInfo p;
            p.name = "self";
            p.kind = SymbolKind::Parameter;
            p.detail = "self: " + std::string(owner->name.lexeme);
            p.type_name = std::string(owner->name.lexeme);
            p.file_path = doc.file_path;
            p.range = {{fd->name.selection.line, 0},
                       {fd->name.selection.line + 500, 0}};
            p.selection_range = p.range;
            p.is_local = true;
            p.scope_func = name;
            p.scope_end_line = fn_end;
            doc.symbols.push_back(p);
        }
        size_t start = owner ? 1 : 0;
        for (size_t pi = start; pi < fd->params.size(); ++pi) {
            SymbolInfo p;
            p.name = std::string(fd->params[pi].lexeme);
            p.kind = SymbolKind::Parameter;
            p.detail = "parameter " + p.name;
            p.type_name = (pi < fd->param_types.size()) ? fd->param_types[pi] : "";
            p.has_type_hint = !p.type_name.empty();
            p.file_path = doc.file_path;
            p.range = {{fd->params[pi].selection.line, fd->params[pi].selection.column},
                       {fd->params[pi].selection.line, fd->params[pi].selection.column + p.name.size()}};
            p.selection_range = p.range;
            p.is_local = true;
            p.scope_func = name;
            p.scope_end_line = fn_end;
            doc.symbols.push_back(p);
        }

        Ctx saved = c;
        c.current_func = name;
        c.current_func_symbol = f_idx;
        if (fd->body) {
            c.block_ends.push_back(fd->body->close_line);
            walk_stmts(fd->body->statements, doc, c);
            c.block_ends.pop_back();
        }
        c = saved;
    }

    static void walk_struct_decl(pyle::StructDeclStmt* sd, DocumentModel& doc, Ctx& c) {
        std::string name(sd->name.lexeme);

        std::string detail = "struct " + name + "(";
        for (size_t fi = 0; fi < sd->fields.size(); ++fi) {
            if (fi > 0) detail += ", ";
            detail += std::string(sd->fields[fi].lexeme);
            if (fi < sd->field_types.size() && !sd->field_types[fi].empty()) {
                detail += ": " + sd->field_types[fi];
            }
        }
        detail += ")";

        SymbolInfo s;
        s.name = name;
        s.kind = SymbolKind::Struct;
        s.detail = detail;
        s.file_path = doc.file_path;
        s.range = {{sd->name.selection.line, sd->name.selection.column},
                   {sd->name.selection.line + 1000, 0}};
        s.selection_range = {{sd->name.selection.line, sd->name.selection.column},
                             {sd->name.selection.line, sd->name.selection.column + name.size()}};
        s.type_name = name;
        doc.symbols.push_back(s);

        for (size_t fi = 0; fi < sd->fields.size(); ++fi) {
            SymbolInfo f;
            f.name = std::string(sd->fields[fi].lexeme);
            f.kind = SymbolKind::Field;
            f.type_name = (fi < sd->field_types.size()) ? sd->field_types[fi] : "";
            f.has_type_hint = !f.type_name.empty();
            f.detail = "self." + f.name + (f.type_name.empty() ? "" : ": " + f.type_name);
            f.file_path = doc.file_path;
            f.range = {{sd->fields[fi].selection.line, sd->fields[fi].selection.column},
                       {sd->fields[fi].selection.line, sd->fields[fi].selection.column + f.name.size()}};
            f.selection_range = f.range;
            f.parent_struct = name;
            doc.symbols.push_back(f);
        }

        Ctx saved = c;
        c.current_struct = name;
        for (const auto& m : sd->methods) {
            if (m) walk_func_decl(m.get(), doc, c, sd);
        }
        c = saved;
    }

    static void walk_expression(pyle::Expr* expr, DocumentModel& doc, Ctx& c) {
        if (!expr) return;
        if (auto* sf = dynamic_cast<pyle::SetFieldExpr*>(expr)) {
            if (auto* obj = dynamic_cast<pyle::VariableExpr*>(sf->obj.get())) {
                if (obj->name.lexeme == "self" && !c.current_struct.empty()) {
                    std::string fname(sf->name.lexeme);
                    std::string ftype = sf->type_annotation.empty()
                        ? expr_to_chain_string(sf->value.get())
                        : sf->type_annotation;
                    bool in_ctor = (c.current_func == "_init");
                    bool found = false;
                    for (auto& sym : doc.symbols) {
                        if (sym.kind == SymbolKind::Field && sym.parent_struct == c.current_struct && sym.name == fname) {
                            if (!sym.has_type_hint && !ftype.empty() && (sym.type_name.empty() || in_ctor)) {
                                sym.type_name = ftype;
                                sym.detail = "self." + fname + ": " + ftype;
                            }
                            found = true;
                            break;
                        }
                    }
                    if (!found && !ftype.empty()) {
                        SymbolInfo f;
                        f.name = fname;
                        f.kind = SymbolKind::Field;
                        f.detail = "self." + fname + ": " + ftype;
                        f.type_name = ftype;
                        f.file_path = doc.file_path;
                        f.parent_struct = c.current_struct;
                        doc.symbols.push_back(f);
                    }
                }
            }
        }
        if (auto* call = dynamic_cast<pyle::CallExpr*>(expr)) {
            if (auto* callee = dynamic_cast<pyle::VariableExpr*>(call->callee.get())) {
                if (callee->name.lexeme == "add_import_path" && !call->args.empty()) {
                    if (auto* lit = dynamic_cast<pyle::LiteralExpr*>(call->args[0].get())) {
                        doc.import_paths.push_back(pyle::lsp::utils::strip_quotes(std::string(lit->token.lexeme)));
                    }
                }
            }
        }
    }
};

}
