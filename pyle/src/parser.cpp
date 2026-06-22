

#include "pyle/parser.hpp"
#include "pyle/ast.hpp"
#include "pyle/error_reporter.hpp"
#include "pyle/token.hpp"
#include <memory>
#include <utility>
#include <vector>

namespace pyle {

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

    bool Parser::is_at_end() const {
        return peek().type == TokenType::EOF_TOKEN;
    }

    Token Parser::peek() const {
        return tokens[current];
    }

    Token Parser::previous() const {
        return tokens[current - 1];
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
                case TokenType::FUNC:
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
            if (match({TokenType::FOR})) return for_statement();
            if (match({TokenType::FUNC})) return function_declaration();
            if (match({TokenType::RETURN})) return return_statement();

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

        consume(TokenType::LEFT_BRACE, "Expected '{' before function body.");
        std::unique_ptr<BlockStmt> body = block();

        return std::make_unique<FuncDeclStmt>(std::move(name), std::move(params), std::move(body));
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

        while (!check(TokenType::RIGHT_BRACE)) {
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
            
            if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
                Token name = var_expr->name;
                
                // Map += to +
                TokenType binary_op_type;
                std::string_view op_lexeme;
                if (op_eq.type == TokenType::PLUS_EQUAL) { binary_op_type = TokenType::PLUS; op_lexeme = "+"; }
                else if (op_eq.type == TokenType::MINUS_EQUAL) { binary_op_type = TokenType::MINUS; op_lexeme = "-"; }
                else if (op_eq.type == TokenType::STAR_EQUAL) { binary_op_type = TokenType::STAR; op_lexeme = "*"; }
                else { binary_op_type = TokenType::SLASH; op_lexeme = "/"; }
                
                Token binary_op(binary_op_type, op_lexeme, op_eq.selection);
                
                auto left_var = std::make_unique<VariableExpr>(name);
                auto binary_expr = std::make_unique<BinaryExpr>(std::move(left_var), binary_op, std::move(value));
                return std::make_unique<AssignExpr>(name, std::move(binary_expr));
            }
            
            reporter.report(op_eq.selection, ErrorType::Syntax, "Invalid compound assignment target.");
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
                Token method_name = consume(TokenType::IDENTIFIER, "Expected method name '.'.");
                consume(TokenType::LEFT_PAREN, "Expected '(' after method name.");

                std::vector<std::unique_ptr<Expr>> arguments;

                if (!check(TokenType::RIGHT_PAREN)) {
                    do {
                        arguments.push_back(expression());
                    } while (match({TokenType::COMMA}));
                }

                Token paren = consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
                expr = std::make_unique<MethodCallExpr>(std::move(expr), method_name, paren, std::move(arguments));
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

    std::unique_ptr<Expr> Parser::primary() {
        if (match({TokenType::INT, TokenType::FLOAT, TokenType::STRING, 
                TokenType::TRUE, TokenType::FALSE, TokenType::NIL})) {
            return std::make_unique<LiteralExpr>(previous());
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
                    elements.push_back(expression());
                } while (match({TokenType::COMMA}));
            }

            consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements.");
            return std::make_unique<ArrayExpr>(std::move(elements));
        }

        reporter.report(peek().selection, ErrorType::Syntax, "Expected expression");
        throw ParserError();
    }


}

