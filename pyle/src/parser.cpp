

#include "pyle/parser.hpp"
#include "pyle/ast.hpp"
#include "pyle/error_reporter.hpp"
#include "pyle/token.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace pyle {

    static std::unique_ptr<Expr> clone_expr(Expr* e) {
        if (auto* v = dynamic_cast<VariableExpr*>(e)) {
            return std::make_unique<VariableExpr>(v->name);
        }
        if (auto* l = dynamic_cast<LiteralExpr*>(e)) {
            return std::make_unique<LiteralExpr>(l->token);
        }
        if (auto* g = dynamic_cast<GetFieldExpr*>(e)) {
            return std::make_unique<GetFieldExpr>(clone_expr(g->obj.get()), g->name);
        }
        if (auto* idx = dynamic_cast<IndexExpr*>(e)) {
            return std::make_unique<IndexExpr>(clone_expr(idx->callee.get()), clone_expr(idx->index.get()));
        }
        return nullptr;
    }

    std::vector<std::unique_ptr<Stmt>> Parser::parse() {
        std::vector<std::unique_ptr<Stmt>> stmts;
        try {
            while (!is_at_end()) {
                stmts.push_back(statement());
            }
        } catch (ParserError& error) {
            return {};
        }
        return stmts;
    }

    Token Parser::advance() {
        if (!is_at_end()) current++;
        return previous();
    }

    bool Parser::check(const TokenType type) const {
        if (is_at_end()) return false;
        return peek().type == type;
    }

    bool Parser::match(std::initializer_list<TokenType> types) {
        for (const TokenType type: types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    Token Parser::consume(TokenType type, const std::string &message) {
        if (check(type)) return advance();

        reporter.report(peek().selection, ErrorType::Syntax, message);
        throw ParserError();
    }

    void Parser::consume_statement_end() {
        if (match({TokenType::SEMICOLON})) return;

        if (is_at_end()) return;

        if (peek().selection.line > previous().selection.line) return;

        reporter.report(peek().selection, ErrorType::Syntax, "Expected newline or ';' after statement.");
        throw ParserError();
    }

    void Parser::synchronize() {
        advance();

        while (!is_at_end()) {
            if (previous().type == TokenType::SEMICOLON) return;

            switch (peek().type) {
                case TokenType::IF:
                case TokenType::FOR:
                case TokenType::WHILE:
                case TokenType::FN:
                case TokenType::LET:
                case TokenType::GLOBAL:
                case TokenType::STRUCT:
                case TokenType::RETURN:
                    return;
                default:
                    break;
            }
            advance();
        }
    }

    std::unique_ptr<Stmt> Parser::statement() {
        try {
            if (match({TokenType::LET})) return var_declaration();
            if (match({TokenType::LEFT_BRACE})) return block();
            if (match({TokenType::IF})) return if_statement();
            if (match({TokenType::WHILE})) return while_statement();
            if (match({TokenType::LOOP})) return loop_statement();
            if (match({TokenType::FOR})) return for_statement();
            if (match({TokenType::FN})) return function_declaration();
            if (match({TokenType::RETURN})) return return_statement();
            if (match({TokenType::BREAK})) return break_statement();
            if (match({TokenType::STRUCT})) return struct_declaration();

            return expression_statement();
        } catch (ParserError& err) {
            synchronize();
            return nullptr;
        }
    }

    std::unique_ptr<Stmt> Parser::function_declaration() {
        Token name = consume(TokenType::IDENTIFIER, "Expected function name.");
        consume(TokenType::LEFT_PAREN, "Expected '(' after function name.");
        std::vector<Token> params;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                if (params.size() >= 255) {
                    reporter.report(peek().selection, ErrorType::Syntax, "Cannot have more than 255 parameters");
                    throw ParserError();
                }
                params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
        
        std::unique_ptr<BlockStmt> body;

        if (match({TokenType::ARROW})) {
            std::unique_ptr<Expr> expr = expression();
            consume_statement_end();
            
            std::vector<std::unique_ptr<Stmt>> statements;
            statements.push_back(std::make_unique<ReturnStmt>(std::move(expr)));
            body = std::make_unique<BlockStmt>(std::move(statements));
        } else {
            consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
            body = block();
        }
        
        return std::make_unique<FuncDeclStmt>(std::move(name), std::move(params), std::move(body));
    }

    // In pyle/src/parser.cpp
std::unique_ptr<Stmt> Parser::struct_declaration() {
    Token name = consume(TokenType::IDENTIFIER, "Expected struct name.");

    consume(TokenType::LEFT_PAREN, "Expected '(' after struct name for fields.");
        std::vector<Token> fields;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                fields.push_back(consume(TokenType::IDENTIFIER, "Expected field name."));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after struct fields.");

        consume(TokenType::LEFT_BRACE, "Expected '{' before struct body.");
        std::vector<std::unique_ptr<FuncDeclStmt>> methods;
        
        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            consume(TokenType::FN, "Expected 'fn' for method declaration inside struct.");
            Token method_name = consume(TokenType::IDENTIFIER, "Expected method name.");
            consume(TokenType::LEFT_PAREN, "Expected '(' after method name.");
            
            std::vector<Token> params;
            Token self_tok(TokenType::IDENTIFIER, "self", method_name.selection);
            params.push_back(self_tok);

            if (!check(TokenType::RIGHT_PAREN)) {
                do {
                    params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
            
            consume(TokenType::LEFT_BRACE, "Expected '{' before method body.");
            std::unique_ptr<BlockStmt> body = block();

            if (method_name.lexeme == "_init") {
                auto return_self = std::make_unique<ReturnStmt>(std::make_unique<VariableExpr>(self_tok));
                body->statements.push_back(std::move(return_self));
            }

            methods.push_back(std::make_unique<FuncDeclStmt>(method_name, std::move(params), std::move(body)));
        }
        consume(TokenType::RIGHT_BRACE, "Expected '}' after struct body.");
        return std::make_unique<StructDeclStmt>(name, std::move(fields), std::move(methods));
    }

    std::unique_ptr<Stmt> Parser::return_statement() {
        Token keyword = previous();
        std::unique_ptr<Expr> value = nullptr;

        if (!check(TokenType::RIGHT_BRACE) && !check(TokenType::EOF_TOKEN) && 
            peek().selection.line == keyword.selection.line) {
            value = expression();
        }

        consume_statement_end();
        return std::make_unique<ReturnStmt>(std::move(value)); 
    }

    std::unique_ptr<Stmt> Parser::if_statement() {
        std::unique_ptr<Expr> condition = expression();

        std::unique_ptr<Stmt> then_branch = statement();
        std::unique_ptr<Stmt> else_branch = nullptr;

        if (match({TokenType::ELSE})) {
            else_branch = statement();
        } else if (match({TokenType::ELIF})){
            else_branch = if_statement();
        }

        return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
    }

    std::unique_ptr<Stmt> Parser::while_statement() {
        std::unique_ptr<Expr> condition = expression();
        std::unique_ptr<Stmt> body = statement();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    std::unique_ptr<Stmt> Parser::loop_statement() {
        Token true_token(TokenType::TRUE, "true", previous().selection);
        auto condition = std::make_unique<LiteralExpr>(true_token);
        
        std::unique_ptr<Stmt> body = statement();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }

    std::unique_ptr<Stmt> Parser::break_statement() {
        Token token = previous();
        consume_statement_end();
        return std::make_unique<BreakStmt>(token);
    }

    std::unique_ptr<Stmt> Parser::for_statement() {
        Token loop_var = consume(TokenType::IDENTIFIER, "Expected variable name after 'for'.");
        consume(TokenType::IN, "Expected 'in' after variable.");
        
        std::unique_ptr<Expr> iterable = expression();
        
        consume(TokenType::LEFT_BRACE, "Expected '{' before body.");
        std::unique_ptr<BlockStmt> body = block();
        
        return std::make_unique<ForStmt>(loop_var, std::move(iterable), std::move(body));
    }

    std::unique_ptr<Stmt> Parser::var_declaration() {
        Token name = consume(TokenType::IDENTIFIER, "Expected variable name.");

        std::unique_ptr<Expr> initializer = nullptr;
        if (match({TokenType::EQUAL})) {
            initializer = expression();
        }

        consume_statement_end();
        return std::make_unique<VarDeclStmt>(name, std::move(initializer));
    }

    std::unique_ptr<Stmt> Parser::expression_statement() {
        std::unique_ptr<Expr> expr = expression();
        consume_statement_end();
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    std::unique_ptr<BlockStmt> Parser::block() {
        std::vector<std::unique_ptr<Stmt>> statements;

        while (!check(TokenType::RIGHT_BRACE) && !is_at_end()) {
            statements.push_back(statement());
        }

        consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
        return std::make_unique<BlockStmt>(std::move(statements));
    }

    std::unique_ptr<Expr> Parser::expression() {
        return assignment();
    }

    std::unique_ptr<Expr> Parser::assignment() {
        std::unique_ptr<Expr> expr = logical_or();

        if (match({TokenType::EQUAL})) {
            Token equals = previous();

            std::unique_ptr<Expr> value = assignment();

            if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
                Token name = var_expr->name;
                return std::make_unique<AssignExpr>(name, std::move(value));
            }
            if (auto* get_field = dynamic_cast<GetFieldExpr*>(expr.get())) {
                return std::make_unique<SetFieldExpr>(std::move(get_field->obj), get_field->name, std::move(value));
            }
            if (auto* index_expr = dynamic_cast<IndexExpr*>(expr.get())) {
                return std::make_unique<IndexAssignExpr>(
                    std::move(index_expr->callee),
                    std::move(index_expr->index),
                    std::move(value)
                );
            }

            reporter.report(equals.selection, ErrorType::Syntax, "Invalid assignment target.");
        }

        if (match({TokenType::PLUS_EQUAL, TokenType::MINUS_EQUAL, TokenType::STAR_EQUAL, TokenType::SLASH_EQUAL})) {
            Token op_eq = previous();
            std::unique_ptr<Expr> value = assignment();
            
            TokenType binary_op_type;
            std::string_view op_lexeme;
            if (op_eq.type == TokenType::PLUS_EQUAL) { binary_op_type = TokenType::PLUS; op_lexeme = "+"; }
            else if (op_eq.type == TokenType::MINUS_EQUAL) { binary_op_type = TokenType::MINUS; op_lexeme = "-"; }
            else if (op_eq.type == TokenType::STAR_EQUAL) { binary_op_type = TokenType::STAR; op_lexeme = "*"; }
            else { binary_op_type = TokenType::SLASH; op_lexeme = "/"; }
                
            Token binary_op(binary_op_type, op_lexeme, op_eq.selection);
            
            auto left_clone = clone_expr(expr.get());
            if (!left_clone) {
                reporter.report(op_eq.selection, ErrorType::Syntax, "Invalid compound assignment target.");
                throw ParserError();
            }
            
            auto binary_expr = std::make_unique<BinaryExpr>(std::move(left_clone), binary_op, std::move(value));
            
            if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
                return std::make_unique<AssignExpr>(var_expr->name, std::move(binary_expr));
            }
            if (auto* get_field = dynamic_cast<GetFieldExpr*>(expr.get())) {
                return std::make_unique<SetFieldExpr>(std::move(get_field->obj), get_field->name, std::move(binary_expr));
            }
            if (auto* index_expr = dynamic_cast<IndexExpr*>(expr.get())) {
                return std::make_unique<IndexAssignExpr>(std::move(index_expr->callee), std::move(index_expr->index), std::move(binary_expr));
            }
            
            reporter.report(op_eq.selection, ErrorType::Syntax, "Invalid compound assignment target.");
            throw ParserError();
    }
    

        return expr;
    }

    std::unique_ptr<Expr> Parser::logical_or() {
        std::unique_ptr<Expr> expr = logical_and();

        while (match({TokenType::OR})) {
            Token op = previous();
            std::unique_ptr<Expr> right = logical_and();
            expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::logical_and() {
        std::unique_ptr<Expr> expr = equality();

        while (match({TokenType::AND})) {
            Token op = previous();
            std::unique_ptr<Expr> right = equality();
            expr = std::make_unique<LogicalExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::equality() {
        std::unique_ptr<Expr> expr = comparison();

        while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = comparison();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::comparison() {
        std::unique_ptr<Expr> expr = range();

        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::range() {
        std::unique_ptr<Expr> expr = term(); 
        if (match({TokenType::DOT_DOT})) {
            Token op = previous();
            std::unique_ptr<Expr> right = term();
            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::term() {
        std::unique_ptr<Expr> expr = factor();
        while (match({TokenType::PLUS, TokenType::MINUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = factor();


            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::factor() {
        std::unique_ptr<Expr> expr = unary();

        while (match({TokenType::STAR, TokenType::SLASH, TokenType::PERCENT})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();

            expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    std::unique_ptr<Expr> Parser::unary() {
        if (match({TokenType::NOT, TokenType::MINUS, TokenType::PLUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();
            return std::make_unique<UnaryExpr>(op, std::move(right));
        }

        return call();
    }

    std::unique_ptr<Expr> Parser::call() {
        std::unique_ptr<Expr> expr = primary();

        while (true) {
            if (match({TokenType::LEFT_PAREN})) {
                expr = finish_call(std::move(expr));
            } else if (match({TokenType::DOT})) {
                Token field_name = consume(TokenType::IDENTIFIER, "Expected field or method name.");
                
                if (match({TokenType::LEFT_PAREN})) {
                    std::vector<std::unique_ptr<Expr>> arguments;
                    if (!check(TokenType::RIGHT_PAREN)) {
                        do {
                            arguments.push_back(expression());
                        } while (match({TokenType::COMMA}));
                    }

                    Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
                    expr = std::make_unique<MethodCallExpr>(std::move(expr), field_name, paren, std::move(arguments));
                } else {
                    expr = std::make_unique<GetFieldExpr>(std::move(expr), field_name);
                }

            } else if(match({TokenType::LEFT_BRACKET})) {
                std::unique_ptr<Expr> index = expression();
                consume(TokenType::RIGHT_BRACKET, "Expected ']' after index.");
                expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expr> Parser::finish_call(std::unique_ptr<Expr> callee) {
        std::vector<std::unique_ptr<Expr>> arguments;

        if (check(TokenType::IDENTIFIER) && peek_next() == TokenType::COLON) {
            std::vector<std::pair<Token, std::unique_ptr<Expr>>> kwargs;
            do {
                Token key = consume(TokenType::IDENTIFIER, "Expected field name.");
                consume(TokenType::COLON, "Expected ':' after field name.");
                kwargs.push_back({key, expression()});
            } while (match({TokenType::COMMA}));
            Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
            return std::make_unique<CallKwExpr>(std::move(callee), paren, std::move(kwargs));
        }

        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                if (arguments.size() >= 255) {
                    reporter.report(peek().selection, ErrorType::Syntax, "Cannot have more than 255 arguments.");
                    throw ParserError();
                }
                arguments.push_back(expression());
            } while (match({TokenType::COMMA}));
        }

        Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
        return std::make_unique<CallExpr>(std::move(callee), paren, std::move(arguments));
    }
    
    std::unique_ptr<Expr> Parser::fun_expression() {
        consume(TokenType::LEFT_PAREN, "Expected '(' after 'fn' inside expression.");
        std::vector<Token> params;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                if (params.size() >= 255) {
                    reporter.report(peek().selection, ErrorType::Syntax, "Cannot have more than 255 parameters");
                    throw ParserError();
                }
                params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name."));
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");
        
        std::unique_ptr<BlockStmt> body;
        if (match({TokenType::ARROW})) {
            auto expr = expression();
            std::vector<std::unique_ptr<Stmt>> statements;
            statements.push_back(std::make_unique<ReturnStmt>(std::move(expr)));
            body = std::make_unique<BlockStmt>(std::move(statements));
        } else {
            consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
            body = block();
        }
        
        return std::make_unique<FuncExpr>(std::move(params), std::move(body));
    }

    std::unique_ptr<Expr> Parser::primary() {
        if (match({TokenType::INT, TokenType::FLOAT, TokenType::STRING, 
                TokenType::TRUE, TokenType::FALSE, TokenType::NONE})) {
            return std::make_unique<LiteralExpr>(previous());
        }

        if (match({TokenType::FN})) { 
            return fun_expression();
        }

        if (match({TokenType::IDENTIFIER})) {
            return std::make_unique<VariableExpr>(previous());
        }

        if (match({TokenType::LEFT_PAREN})) {
            std::unique_ptr<Expr> expr = expression();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after expression");
            return std::make_unique<GroupingExpr>(std::move(expr));
        }

        if (match({TokenType::LEFT_BRACKET})) {
            std::vector<std::unique_ptr<Expr>> elements;

            if (!check(TokenType::RIGHT_BRACKET)) {
                do {
                    if (check(TokenType::RIGHT_BRACKET)) break; 
                    elements.push_back(expression());
                } while (match({TokenType::COMMA}));
            }

            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements.");
            return std::make_unique<ArrayExpr>(std::move(elements));
        }

        if (match({TokenType::LEFT_BRACE})) {
            std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
            if (!check(TokenType::RIGHT_BRACE)) {
                do {
                    if (check(TokenType::RIGHT_BRACE)) break;

                    std::unique_ptr<Expr> key;
                    if (match({TokenType::IDENTIFIER})) {
                        key = std::make_unique<ImplicitStringExpr>(previous());
                    } else {
                        key = expression();
                    }
                    consume(TokenType::COLON, "Expected ':' after map key.");
                    std::unique_ptr<Expr> value = expression();
                    entries.push_back({std::move(key), std::move(value)});
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RIGHT_BRACE, "Expected '}' after map entries.");
            return std::make_unique<MapExpr>(std::move(entries));
        }

        reporter.report(peek().selection, ErrorType::Syntax, "Expected expression");
        throw ParserError();
    }


}

