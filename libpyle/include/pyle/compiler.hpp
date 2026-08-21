#pragma once

#include "pyle/ast.hpp"
#include "pyle/bytecode.hpp"
#include "pyle/error_reporter.hpp"
#include "pyle/value.hpp"
#include <ankerl/unordered_dense.h>
#include <cstddef>
#include <cstdint>

namespace pyle {

    class VM;

    struct ConstPoolKey {
        uint8_t tag;
        uint64_t bits;

        bool operator==(const ConstPoolKey& other) const {
            return tag == other.tag && bits == other.bits;
        }
    };

    struct ConstPoolKeyHash {
        using is_avalanching = void;

        uint64_t operator()(const ConstPoolKey& key) const noexcept {
            const uint64_t tag_hash = ankerl::unordered_dense::hash<uint64_t>{}(key.tag);
            return ankerl::unordered_dense::hash<uint64_t>{}(key.bits) ^ (tag_hash * 0x9E3779B97F4A7C15ULL);
        }
    };

    struct Local {
        Token name;
        int depth = 0;
        bool on_stack = true;
    };

    struct CompileState {
        CompileState* enclosing = nullptr;
        std::vector<Local> locals;
        int scope_depth = 0;
        struct Upvalue {
            uint8_t index;
            bool is_local;
        };
        std::vector<Upvalue> upvalues;
        bool is_init = false;
    };

    class Compiler: public Visitor {
        Chunk chunk;
        Chunk* current_chunk;
        ErrorReporter& reporter;
        VM& vm;

        CompileState* current_state = nullptr; 

        void emit_instruction(OpCode op, uint32_t operand, size_t line);
        size_t emit_jump(OpCode op, size_t line);
        void patch_jump(size_t offset);
        void emit_loop(size_t loop_start, size_t line);
        uint32_t make_constant(Value value);

        uint64_t value_bits(const Value& value) const;
        bool instruction_is_load_const(size_t offset, uint32_t& const_idx) const;
        void pop_instructions(size_t count);
        bool fold_arithmetic(TokenType op, const Value& lhs, const Value& rhs, Value& out);
        bool fold_comparison(TokenType op, const Value& lhs, const Value& rhs, Value& out);
        bool try_fold_binary(BinaryExpr* expr, uint32_t lhs_idx, uint32_t rhs_idx);
        bool try_fold_unary(UnaryExpr* expr, uint32_t operand_idx);
        bool try_emit_local_increment(AssignExpr* expr);

        ankerl::unordered_dense::map<ConstPoolKey, uint32_t, ConstPoolKeyHash> const_lookup;

        void begin_scope();
        void end_scope();
        int resolve_local(const Token& name) const;
        int resolve_global_slot(const Token& name);

        int resolve_upvalue(CompileState* state, const Token& name);
        int add_upvalue(CompileState* state, uint8_t index, bool is_local);
        int resolve_local_in_state(CompileState* state, const Token& name);

        std::vector<std::vector<size_t>> loop_breaks;
        std::vector<size_t> loop_locals_start;
        std::vector<size_t> loop_continue_targets;

        HeapIdx compile_function(const std::vector<Token>& params, BlockStmt* body, std::string_view name);

    private:
        std::string process_str_escapes(const Token& token); 
    public:
        Compiler(VM& vm, ErrorReporter& reporter): reporter(reporter), vm(vm) {};

        Chunk compile(const std::vector<std::unique_ptr<Stmt>>& statements);

        void visit_literal(LiteralExpr* expr) override;
        void visit_grouping(GroupingExpr* expr) override;
        void visit_binary(BinaryExpr* expr) override;
        void visit_expression(ExpressionStmt* stmt) override;
        void visit_var_decl(VarDeclStmt *stmt) override;
        void visit_variable(VariableExpr *expr) override;
        void visit_assign(AssignExpr *expr) override;
        void visit_call(CallExpr *expr) override;
        void visit_block(BlockStmt *stmt) override;
        void visit_array(ArrayExpr *expr) override;
        void visit_method_call(MethodCallExpr *expr) override;
        void visit_if(IfStmt *stmt) override;
        void visit_while(WhileStmt *stmt) override;
        void visit_logical(LogicalExpr* expr) override;
        void visit_unary(UnaryExpr* expr) override;
        void visit_index(IndexExpr* expr) override;
        void visit_index_assign(IndexAssignExpr* expr) override;
        void visit_return(ReturnStmt* stmt) override;
        void visit_func_decl(FuncDeclStmt* stmt) override;
        void visit_for(ForStmt* stmt) override;
        void visit_break(BreakStmt* stmt) override;
        void visit_continue(ContinueStmt* stmt) override;
        void visit_func_expr(FuncExpr* expr) override;
        void visit_struct_decl(StructDeclStmt* stmt) override;
        void visit_get_field(GetFieldExpr* expr) override;
        void visit_set_field(SetFieldExpr* expr) override;
        void visit_implicit_string(ImplicitStringExpr* expr) override;
        void visit_map_expr(MapExpr* expr) override;
        void visit_call_kw_expr(CallKwExpr* expr) override;
        void visit_yield_expr(YieldExpr* expr) override;
    };


}