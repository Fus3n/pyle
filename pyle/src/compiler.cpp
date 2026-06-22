#include "pyle/compiler.hpp"

#include "pyle/ast.hpp"
#include "pyle/bytecode.hpp"
#include "pyle/token.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"
#include <cstdint>
#include <vector>


namespace pyle {
    std::string process_str_escapes(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (c == '\\' && i + 1 < str.size()) {
                char next = str[i + 1];
                switch (next) {
                    case 'n':  result += '\n'; break;
                    case 't':  result += '\t'; break;
                    case 'r':  result += '\r'; break;
                    case '\\': result += '\\'; break;
                    case '"':  result += '"';  break;
                    case '\'': result += '\''; break;
                    case '0':  result += '\0'; break;
                    default: 
                        result += '\\'; 
                        result += next; 
                        break;
                }
                i++; 
            } else {
                result += c;
            }
        }
        return result;
    }

    void Compiler::emit_instruction(OpCode op, uint32_t operand, size_t line) {
        current_chunk->instr.push_back(encode(op, operand));
        current_chunk->lines.push_back(line);
    }

    size_t Compiler::emit_jump(OpCode op, size_t line) {
        emit_instruction(op, 0xFFFF, line);
        return current_chunk->instr.size() - 1;
    }

    void Compiler::patch_jump(size_t offset) {
        size_t jump = current_chunk->instr.size() - 1 - offset;

        OpCode op = get_op(current_chunk->instr[offset]);
        current_chunk->instr[offset] = encode(op, static_cast<uint32_t>(jump));
    }

    void Compiler::emit_loop(size_t loop_start, size_t line) {
        size_t jump = current_chunk->instr.size() - loop_start + 1;
        emit_instruction(OpCode::LOOP, static_cast<uint32_t>(jump), line);
    }

    uint32_t Compiler::make_constant(Value value) {
        current_chunk->const_pool.push_back(value);
        return static_cast<uint32_t>(current_chunk->const_pool.size() - 1);
    }

    void Compiler::begin_scope() {
        scope_depth++;
    }

    void Compiler::end_scope() {
        scope_depth--;

        while (!locals.empty() && locals.back().depth > scope_depth) {
            emit_instruction(OpCode::POP, 0, 0);
            locals.pop_back();
        }
    }

    int Compiler::resolve_local(const Token &name) const {
        for (int i = static_cast<int>(locals.size()) - 1; i >= 0; i--) {
            if (locals[i].name.lexeme == name.lexeme) {
                return i;
            }
        }
        return -1;
    }

    int Compiler::resolve_global_slot(const Token& name) {
        auto it = vm.global_slot_map.find(std::string(name.lexeme));
        if (it == vm.global_slot_map.end()) {
            reporter.report(name.selection, ErrorType::Compile,
                            fmt::format("Undefined global '{}'.", name.lexeme));
            return -1;
        }
        return it->second;
    }

    Chunk Compiler::compile(const std::vector<std::unique_ptr<Stmt>>& statements) {
        Chunk main_chunk;
        current_chunk = &main_chunk;
        vm.set_gc_enabled(false);
        for (const auto& stmt : statements) {
            if (stmt) {
                stmt->accept(this);
            }
        }
        emit_instruction(OpCode::HALT, 0, 0);
        vm.set_gc_enabled(true);
        return std::move(main_chunk);
    }

    void Compiler::visit_literal(LiteralExpr *expr) {
        Value v;
        Token tok = expr->token;

        if (tok.type == TokenType::INT) {
            v.tag = Value::Tag::Int;
            v.as_int = std::stoll(std::string(tok.lexeme));
        } else if (tok.type == TokenType::FLOAT) {
            v.tag = Value::Tag::Float;
            v.as_float = std::stod(std::string(tok.lexeme));
        } else if (tok.type == TokenType::STRING) {
            std::string_view raw_str = tok.lexeme.substr(1, tok.lexeme.size() - 2);
            std::string processed = process_str_escapes(raw_str);
            HeapIdx idx = vm.intern_string(processed);
            v.tag = Value::Tag::StringRef;
            v.as_ref = idx;
        } else if (tok.type == TokenType::TRUE) {
            v = Value(true);
        } else if (tok.type == TokenType::FALSE) {
            v = Value(false);
        } else if (tok.type == TokenType::NIL) {
            v = Value();
        }

        uint32_t const_idx = make_constant(v);
        emit_instruction(OpCode::LOAD_CONST, const_idx, 1);
    }

    void Compiler::visit_grouping(GroupingExpr *expr) {
        expr->expression->accept(this);
    }

    void Compiler::visit_binary(BinaryExpr *expr) {
        expr->left->accept(this);
        expr->right->accept(this);

        auto line = expr->op.selection.line;
        switch (expr->op.type) {
            case TokenType::PLUS: emit_instruction(OpCode::ADD, 0, line); break;
            case TokenType::MINUS: emit_instruction(OpCode::SUB, 0, line); break;
            case TokenType::STAR: emit_instruction(OpCode::MUL, 0, line); break;
            case TokenType::SLASH: emit_instruction(OpCode::DIV, 0, line); break;
            case TokenType::PERCENT: emit_instruction(OpCode::MOD, 0, line); break;
            case TokenType::EQUAL_EQUAL: emit_instruction(OpCode::EQ, 0, line); break;
            case TokenType::BANG_EQUAL: emit_instruction(OpCode::NEQ, 0, line); break;
            case TokenType::LESS: emit_instruction(OpCode::LT, 0, line); break;
            case TokenType::LESS_EQUAL: emit_instruction(OpCode::LTE, 0, line); break;
            case TokenType::GREATER: emit_instruction(OpCode::GT, 0, line); break;
            case TokenType::GREATER_EQUAL: emit_instruction(OpCode::GTE, 0, line); break;
            default:
                reporter.report(expr->op.selection, ErrorType::Compile, "Unknown binary operator.");
                break;
        }
    }

    void Compiler::visit_expression(ExpressionStmt *stmt) {
        
        if (auto* assign = dynamic_cast<AssignExpr*>(stmt->expression.get())) {
            assign->value->accept(this);
            int arg = resolve_local(assign->name);
            if (arg != -1) {
                emit_instruction(OpCode::SET_LOCAL_POP, arg, assign->name.selection.line);
            } else {
                int slot = resolve_global_slot(assign->name);
                if (slot >= 0) {
                    emit_instruction(OpCode::SET_GLOBAL_SLOT_POP, slot, assign->name.selection.line);
                }
            }
        } else {
            stmt->expression->accept(this);
            emit_instruction(OpCode::POP, 0, 1);
        }
    }

    void Compiler::visit_var_decl(VarDeclStmt *stmt) {
        if (stmt->initializer) {
            stmt->initializer->accept(this);
        } else {
            uint32_t nil_idx = make_constant(Value());
            emit_instruction(OpCode::LOAD_CONST, nil_idx, 0);
        }

        if (scope_depth > 0) {
            locals.push_back(Local{stmt->name, scope_depth});
        } else {
            int slot = vm.declare_global(std::string(stmt->name.lexeme));
            emit_instruction(OpCode::DEFINE_GLOBAL_SLOT, slot, stmt->name.selection.line);
        }
    }

    void Compiler::visit_variable(VariableExpr *expr) {
        int arg = resolve_local(expr->name);

        if (arg != -1) {
            emit_instruction(OpCode::LOAD_LOCAL, arg, expr->name.selection.line);
        } else {
            int slot = resolve_global_slot(expr->name);
            if (slot < 0) return;   // error already reported
            emit_instruction(OpCode::LOAD_GLOBAL_SLOT, slot, expr->name.selection.line);
        }
    }

    void Compiler::visit_assign(AssignExpr *expr) {
        expr->value->accept(this);

        int arg = resolve_local(expr->name);

        if (arg != -1) {
            emit_instruction(OpCode::SET_LOCAL, arg, expr->name.selection.line);
        } else {
            int slot = resolve_global_slot(expr->name);
            if (slot < 0) return;
            emit_instruction(OpCode::SET_GLOBAL_SLOT, slot, expr->name.selection.line);
        }
    }

    void Compiler::visit_call(CallExpr *expr) {
        expr->callee->accept(this);

        for (const auto& arg: expr->args) {
            arg->accept(this);
        }

        emit_instruction(OpCode::CALL, expr->args.size(), expr->paren.selection.line);
    }

    void Compiler::visit_block(BlockStmt *stmt) {
        begin_scope();
        for (const auto& s : stmt->statements) {
            if (s) s->accept(this);
        }
        end_scope();
    }

    void Compiler::visit_array(ArrayExpr *expr) {
        for (const auto& elements : expr->elements) {
            elements->accept(this);
        }

        emit_instruction(OpCode::NEW_ARRAY, expr->elements.size(), 1);
    }

    void Compiler::visit_method_call(MethodCallExpr *expr) {
        expr->callee->accept(this);

        HeapIdx name_idx = vm.intern_string(expr->method_name.lexeme);
        uint32_t const_idx = make_constant(Value(Value::Tag::StringRef, name_idx));
        emit_instruction(OpCode::LOAD_CONST, const_idx, expr->method_name.selection.line);

        for (const auto& arg: expr->arguments) {
            arg->accept(this);
        }

        emit_instruction(OpCode::CALL_METHOD, expr->arguments.size(), expr->paren.selection.line);
    }

    void Compiler::visit_if(IfStmt* stmt) {
        stmt->condition->accept(this);

        size_t then_jump = emit_jump(OpCode::JUMP_IF_FALSE, 0);
        emit_instruction(OpCode::POP, 0, 0);

        stmt->then_branch->accept(this);

        size_t else_jump = emit_jump(OpCode::JUMP, 0);

        patch_jump(then_jump);
        emit_instruction(OpCode::POP, 0, 0);

        if (stmt->else_branch) {
            stmt->else_branch->accept(this);
        }

        patch_jump(else_jump);
    }

    void Compiler::visit_while(WhileStmt* stmt) {
        size_t loop_start = current_chunk->instr.size();
        
        stmt->condition->accept(this);

        size_t exit_jump = emit_jump(OpCode::JUMP_IF_FALSE, 0);
        emit_instruction(OpCode::POP, 0, 0);

        stmt->body->accept(this);

        emit_loop(loop_start, 0);

        patch_jump(exit_jump);

        emit_instruction(OpCode::POP, 0, 0);
    }

    void Compiler::visit_logical(LogicalExpr* expr) {
        expr->left->accept(this);

        if (expr->op.type == TokenType::OR) {
            size_t else_jump = emit_jump(OpCode::JUMP_IF_TRUE, expr->op.selection.line);

            emit_instruction(OpCode::POP, 0, expr->op.selection.line);
            expr->right->accept(this);

            patch_jump(else_jump);
        } else if(expr->op.type == TokenType::AND) {
            size_t end_jump = emit_jump(OpCode::JUMP_IF_FALSE, expr->op.selection.line);
            
            emit_instruction(OpCode::POP, 0, expr->op.selection.line);
            expr->right->accept(this);

            patch_jump(end_jump);
        }
    }

    void Compiler::visit_unary(UnaryExpr* expr) {
        expr->right->accept(this);


        switch (expr->op.type) {
            case TokenType::NOT: {
                emit_instruction(OpCode::NOT, 0, expr->op.selection.line);
                break;
            }   
            case TokenType::MINUS: {
                emit_instruction(OpCode::NEG, 0, expr->op.selection.line);
                break;
            } 
            case TokenType::PLUS: break;
            default:
                reporter.report(expr->op.selection, ErrorType::Compile, "Unknown unary operator.");
                break;
        }
    }

    void Compiler::visit_index(IndexExpr* expr) {
        expr->callee->accept(this);
        expr->index->accept(this);
        emit_instruction(OpCode::GET_INDEX, 0, 0);
    }

    void Compiler::visit_index_assign(IndexAssignExpr* expr) {
        expr->callee->accept(this);
        expr->index->accept(this);
        expr->value->accept(this);
        emit_instruction(OpCode::SET_INDEX, 0, 0);
    }

    void Compiler::visit_func_decl(FuncDeclStmt* stmt) {
        Function fn;
        fn.name = stmt->name.lexeme;
        fn.arity = stmt->params.size();

        int slot = vm.declare_global(std::string(stmt->name.lexeme));

        Chunk* enclosing_chunk = current_chunk;
        std::vector<Local> enclosing_locals = std::move(locals);
        int enclosing_scope = scope_depth;
        
        current_chunk = &fn.chunk;
        locals.clear();
        scope_depth = 0;
        
        begin_scope();
        for (const auto& param : stmt->params) {
            locals.push_back(Local{param, scope_depth});
        }
        
        for (const auto& s : stmt->body->statements) {
            if (s) s->accept(this);
        }
        
        uint32_t nil_idx = make_constant(Value());
        emit_instruction(OpCode::LOAD_CONST, nil_idx, 0);
        emit_instruction(OpCode::RETURN, 0, 0);
        end_scope();
        
        current_chunk = enclosing_chunk;
        locals = std::move(enclosing_locals);
        scope_depth = enclosing_scope;
        
        HeapIdx fn_idx = vm.alloc(Object(std::move(fn)));
        
        Value fn_val(Value::Tag::FuncRef, fn_idx);
        uint32_t fn_const_idx = make_constant(fn_val);
        emit_instruction(OpCode::LOAD_CONST, fn_const_idx, stmt->name.selection.line);
        
        emit_instruction(OpCode::DEFINE_GLOBAL_SLOT, slot, stmt->name.selection.line);
    }

    void Compiler::visit_return(ReturnStmt* stmt) {
        if (stmt->value) {
            stmt->value->accept(this);
        } else {
            uint32_t nil_idx = make_constant(Value());
            emit_instruction(OpCode::LOAD_CONST, nil_idx, 0);
        }
        emit_instruction(OpCode::RETURN, 0, 0);
    }
    
}
