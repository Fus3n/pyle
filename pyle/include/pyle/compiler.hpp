#pragma once

#include "pyle/ast.hpp"
#include "pyle/bytecode.hpp"
#include "pyle/error_reporter.hpp"
#include <cstddef>

namespace pyle {

    class VM;

    struct Local {
        Token name;
        int depth = 0;
    };

    class Compiler: public Visitor {
        Chunk chunk;
        Chunk* current_chunk;
        ErrorReporter& reporter;
        VM& vm;

        std::vector<Local> locals;
        int scope_depth = 0;

        void emit_instruction(OpCode op, uint32_t operand, size_t line);
        size_t emit_jump(OpCode op, size_t line);
        void patch_jump(size_t offset);
        void emit_loop(size_t loop_start, size_t line);
        uint32_t make_constant(Value value);

        void begin_scope();
        void end_scope();
        int resolve_local(const Token& name) const;

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
    };


}