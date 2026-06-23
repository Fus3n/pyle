#pragma once

#include <memory>
#include <utility>
#include <vector>
#include "pyle/token.hpp"

namespace pyle {
    struct Visitor;

    struct ASTNode {
        virtual ~ASTNode() {}
    };

    struct Expr: public ASTNode {
        virtual void accept(Visitor* visitor) = 0;
    };

    struct Stmt: public ASTNode {
        virtual void accept(Visitor* visitor) = 0;
    };

    struct LiteralExpr: public Expr {
        Token token;

        explicit LiteralExpr(Token value): token(value) {}
        void accept(Visitor* visitor) override;
    };

    struct GroupingExpr: public Expr {
        std::unique_ptr<Expr> expression;

        explicit GroupingExpr(std::unique_ptr<Expr> expression): expression(std::move(expression)) {}
        void accept(Visitor* visitor) override;
    };


    struct BinaryExpr : public Expr {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        BinaryExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right) : left(std::move(left)),
            op(std::move(op)), right(std::move(right)) {}

        void accept(Visitor* visitor) override;
    };


    struct ExpressionStmt : public Stmt {
        std::unique_ptr<Expr> expression;

        explicit ExpressionStmt(std::unique_ptr<Expr> expression)
            : expression(std::move(expression)) {}
        void accept(Visitor* visitor) override;
    };

    struct VarDeclStmt: public Stmt {
        Token name;
        std::unique_ptr<Expr> initializer;

        VarDeclStmt(Token name, std::unique_ptr<Expr> initializer)
            : name(name), initializer(std::move(initializer)) {}

        void accept(Visitor* visitor) override;
    };

    struct VariableExpr: public Expr {
        Token name;

        explicit VariableExpr(Token name): name(name) {}
        void accept(Visitor* visitor) override;
    };

    struct AssignExpr: public Expr {
        Token name;
        std::unique_ptr<Expr> value;

        AssignExpr(Token name, std::unique_ptr<Expr> value)
            : name(name), value(std::move(value)) {}
        void accept(Visitor* visitor) override;
    };

    struct CallExpr : public Expr {
        std::unique_ptr<Expr> callee;
        Token paren;
        std::vector<std::unique_ptr<Expr>> args;

        CallExpr(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> args)
            : callee(std::move(callee)), paren(paren), args(std::move(args)) {}

        void accept(Visitor* visitor) override;
    };

    struct BlockStmt: public Stmt {
        std::vector<std::unique_ptr<Stmt>> statements;

      explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
          : statements(std::move(statements)) {}

        void accept(Visitor* visitor) override;
    };

    struct ArrayExpr: public Expr {
        std::vector<std::unique_ptr<Expr>> elements;

        explicit ArrayExpr(std::vector<std::unique_ptr<Expr>> elements)
            : elements(std::move(elements)) {}

        void accept(Visitor* visitor) override;
    };

    struct MethodCallExpr: public Expr {
        std::unique_ptr<Expr> callee;
        Token method_name;
        Token paren;

        std::vector<std::unique_ptr<Expr>> arguments;

        MethodCallExpr(std::unique_ptr<Expr> callee, Token method_name, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
            : callee(std::move(callee)), method_name(method_name), paren(paren), arguments(std::move(arguments)) {}

        void accept(Visitor* visitor) override;
    };
    
    struct IfStmt: public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> then_branch;
        std::unique_ptr<Stmt> else_branch;


        IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> then_branch, std::unique_ptr<Stmt> else_branch)
                : condition(std::move(cond)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}

        void accept(Visitor* visitor) override;
    };

    struct WhileStmt: public Stmt {
        std::unique_ptr<Expr> condition;
        std::unique_ptr<Stmt> body;

        WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<Stmt> body)
            : condition(std::move(cond)), body(std::move(body)) {}

        void accept(Visitor* visitor) override;
    };

    struct LogicalExpr: public Expr {
        std::unique_ptr<Expr> left;
        Token op;
        std::unique_ptr<Expr> right;

        LogicalExpr(std::unique_ptr<Expr> left, Token op, std::unique_ptr<Expr> right)
            : left(std::move(left)), op(op), right(std::move(right)) {}
        void accept(Visitor* visitor) override;
    };

    struct UnaryExpr: public Expr {
        Token op;
        std::unique_ptr<Expr> right;

         UnaryExpr(Token op, std::unique_ptr<Expr> right)
            : op(op), right(std::move(right)) {}
        void accept(Visitor* visitor) override;
    };

    struct IndexExpr: public Expr {
        std::unique_ptr<Expr> callee;
        std::unique_ptr<Expr> index;

        IndexExpr(std::unique_ptr<Expr> callee, std::unique_ptr<Expr> index)
            : callee(std::move(callee)), index(std::move(index)) {}
        void accept(Visitor* visitor) override;
    };

    struct IndexAssignExpr: public Expr {
        std::unique_ptr<Expr> callee;
        std::unique_ptr<Expr> index;
        std::unique_ptr<Expr> value;
        IndexAssignExpr(std::unique_ptr<Expr> callee, std::unique_ptr<Expr> index, std::unique_ptr<Expr> value)
            : callee(std::move(callee)), index(std::move(index)), value(std::move(value)) {}
        void accept(Visitor* visitor) override;
    };


    struct ReturnStmt: public Stmt {
        std::unique_ptr<Expr> value;
        explicit ReturnStmt(std::unique_ptr<Expr> value): value(std::move(value)) {}
        void accept(Visitor* visitor) override;
    };


    struct FuncDeclStmt: public Stmt {
        Token name;
        std::vector<Token> params;
        std::unique_ptr<BlockStmt> body;
        FuncDeclStmt(Token name, std::vector<Token> params, std::unique_ptr<BlockStmt> body)
            : name(std::move(name)), params(std::move(params)), body(std::move(body)) {}
        void accept(Visitor* visitor) override;
    };

    struct ForStmt: public Stmt {
        Token var_name;
        std::unique_ptr<Expr> iterable;
        std::unique_ptr<BlockStmt> body;
        
        ForStmt(Token name, std::unique_ptr<Expr> iterable, std::unique_ptr<BlockStmt> body)
            : var_name(name), iterable(std::move(iterable)), body(std::move(body)) {}
        
        void accept(Visitor *visitor) override;
    };

    struct BreakStmt : public Stmt {
        Token token;
        explicit BreakStmt(Token token) : token(token) {}
        void accept(Visitor* visitor) override;
    };

    struct FuncExpr : public Expr {
        std::vector<Token> params;
        std::unique_ptr<BlockStmt> body;
        FuncExpr(std::vector<Token> params, std::unique_ptr<BlockStmt> body)
            : params(std::move(params)), body(std::move(body)) {}
        void accept(Visitor* visitor) override;
    };

    struct StructDeclStmt : public Stmt {
        Token name;
        std::vector<Token> fields;
        std::vector<std::unique_ptr<FuncDeclStmt>> methods; // <-- ADD THIS
        StructDeclStmt(Token name, std::vector<Token> fields, std::vector<std::unique_ptr<FuncDeclStmt>> methods)
            : name(name), fields(std::move(fields)), methods(std::move(methods)) {}
        void accept(Visitor* visitor) override;
    };

    struct GetFieldExpr : public Expr {
        std::unique_ptr<Expr> obj;
        Token name;
        GetFieldExpr(std::unique_ptr<Expr> obj, Token name) : obj(std::move(obj)), name(name) {}
        void accept(Visitor* visitor) override;
    };

    struct SetFieldExpr : public Expr {
        std::unique_ptr<Expr> obj;
        Token name;
        std::unique_ptr<Expr> value;
        SetFieldExpr(std::unique_ptr<Expr> obj, Token name, std::unique_ptr<Expr> value)
            : obj(std::move(obj)), name(name), value(std::move(value)) {}
        void accept(Visitor* visitor) override;
    };

    struct Visitor {
        virtual ~Visitor() = default;

        virtual void visit_literal(LiteralExpr* expr) = 0;
        virtual void visit_grouping(GroupingExpr* expr) = 0;
        virtual void visit_binary(BinaryExpr* expr) = 0;
        virtual void visit_expression(ExpressionStmt* stmt) = 0;
        virtual void visit_var_decl(VarDeclStmt* stmt) = 0;
        virtual void visit_variable(VariableExpr* expr) = 0;
        virtual void visit_assign(AssignExpr* expr) = 0;
        virtual void visit_call(CallExpr* expr) = 0;
        virtual void visit_block(BlockStmt* stmt) = 0;
        virtual void visit_array(ArrayExpr* expr) = 0;
        virtual void visit_method_call(MethodCallExpr* expr) = 0;
        virtual void visit_if(IfStmt* stmt) = 0;
        virtual void visit_while(WhileStmt* stmt) = 0;
        virtual void visit_logical(LogicalExpr* expr) = 0;
        virtual void visit_unary(UnaryExpr* expr) = 0;
        virtual void visit_index(IndexExpr* expr) = 0;
        virtual void visit_index_assign(IndexAssignExpr* expr) = 0;
        virtual void visit_return(ReturnStmt* stmt) = 0;
        virtual void visit_func_decl(FuncDeclStmt* stmt) = 0;
        virtual void visit_for(ForStmt* stmt) = 0;
        virtual void visit_break(BreakStmt* stmt) = 0; 
        virtual void visit_func_expr(FuncExpr* expr) = 0; 
        virtual void visit_struct_decl(StructDeclStmt* stmt) = 0;
        virtual void visit_get_field(GetFieldExpr* expr) = 0;
        virtual void visit_set_field(SetFieldExpr* expr) = 0;
    };  

    inline void LiteralExpr::accept(Visitor* visitor)  { visitor->visit_literal(this); }
    inline void GroupingExpr::accept(Visitor *visitor) { visitor->visit_grouping(this); }
    inline void BinaryExpr::accept(Visitor *visitor) { visitor->visit_binary(this); }
    inline void ExpressionStmt::accept(Visitor *visitor) { visitor->visit_expression(this); }
    inline void VarDeclStmt::accept(Visitor *visitor) { visitor->visit_var_decl(this); }
    inline void VariableExpr::accept(Visitor *visitor) { visitor->visit_variable(this); }
    inline void AssignExpr::accept(Visitor *visitor) { visitor->visit_assign(this); }
    inline void CallExpr::accept(Visitor *visitor) { visitor->visit_call(this); }
    inline void BlockStmt::accept(Visitor *visitor) { visitor->visit_block(this); }
    inline void ArrayExpr::accept(Visitor *visitor) { visitor->visit_array(this); }
    inline void MethodCallExpr::accept(Visitor *visitor) { visitor->visit_method_call(this); }
    inline void IfStmt::accept(Visitor *visitor) { visitor->visit_if(this); }
    inline void WhileStmt::accept(Visitor *visitor) { visitor->visit_while(this); }
    inline void LogicalExpr::accept(Visitor *visitor) { visitor->visit_logical(this); }
    inline void UnaryExpr::accept(Visitor *visitor) { visitor->visit_unary(this); }
    inline void IndexExpr::accept(Visitor *visitor) { visitor->visit_index(this); }
    inline void IndexAssignExpr::accept(Visitor *visitor) { visitor->visit_index_assign(this); }
    inline void ReturnStmt::accept(Visitor *visitor) { visitor->visit_return(this); }
    inline void FuncDeclStmt::accept(Visitor *visitor) { visitor->visit_func_decl(this); }
    inline void ForStmt::accept(Visitor* visitor) { visitor->visit_for(this); }
    inline void BreakStmt::accept(Visitor* visitor) { visitor->visit_break(this); }
    inline void FuncExpr::accept(Visitor* visitor) { visitor->visit_func_expr(this); }
    inline void StructDeclStmt::accept(Visitor* visitor) { visitor->visit_struct_decl(this); }
    inline void GetFieldExpr::accept(Visitor* visitor) { visitor->visit_get_field(this); }
    inline void SetFieldExpr::accept(Visitor* visitor) { visitor->visit_set_field(this); }
}
