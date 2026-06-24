#pragma once

#pragma once
#include <vector>
#include <string>
#include <initializer_list>
#include <memory>

#include "pyle/token.hpp"
#include "pyle/error_reporter.hpp"
#include "pyle/ast.hpp"

namespace pyle {
    class Parser {
    private:
        std::vector<Token> tokens;
        size_t current = 0;
        ErrorReporter& reporter;

        bool is_at_end() const {
            return peek().type == TokenType::EOF_TOKEN;
        }

        Token peek() const {
            return tokens[current];
        }
        TokenType peek_next() const {
            if (current + 1 >= tokens.size()) return TokenType::EOF_TOKEN;
            return tokens[current + 1].type;
        }
        Token previous() const {
            return tokens[current - 1];
        }

        Token advance();
        bool check(TokenType type) const;
        bool match(std::initializer_list<TokenType> types);
        Token consume(TokenType type, const std::string& message);
        void consume_statement_end();

        std::unique_ptr<Stmt> statement();
        std::unique_ptr<Stmt> if_statement();
        std::unique_ptr<Stmt> while_statement();
        std::unique_ptr<Stmt> loop_statement();
        std::unique_ptr<Stmt> break_statement();
        std::unique_ptr<Stmt> for_statement();
        std::unique_ptr<Stmt> var_declaration();
        std::unique_ptr<Stmt> expression_statement();
        std::unique_ptr<Stmt> function_declaration();
        std::unique_ptr<Stmt> return_statement();
        std::unique_ptr<Stmt> struct_declaration();

        std::unique_ptr<BlockStmt> block();

        std::unique_ptr<Expr> expression();
        std::unique_ptr<Expr> assignment();
        std::unique_ptr<Expr> logical_or();
        std::unique_ptr<Expr> logical_and();
        std::unique_ptr<Expr> equality();
        std::unique_ptr<Expr> comparison();
        std::unique_ptr<Expr> range();
        std::unique_ptr<Expr> term(); // + and -
        std::unique_ptr<Expr> factor(); // * and /
        
        std::unique_ptr<Expr> unary();
        std::unique_ptr<Expr> call();
        std::unique_ptr<Expr> finish_call(std::unique_ptr<Expr> callee, Token open_paren);
        std::unique_ptr<Expr> func_expression();
        std::unique_ptr<Expr> primary(); // basic types, most prio

        std::vector<Token> parse_params();

        // error
        struct ParserError: public std::exception {};
        void synchronize();

    public:
        Parser(std::vector<Token> tokens, ErrorReporter& reporter)
            : tokens(std::move(tokens)), reporter(reporter) {};

        std::vector<std::unique_ptr<Stmt>> parse();
    };
}
