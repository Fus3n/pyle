#include "pyle/lexer.hpp"
#include "pyle/token.hpp"

#include <fmt/format.h>

namespace pyle {
    std::vector<Token> Lexer::tokenize() {
        std::vector<Token> tokens;

        while (!is_at_end()) {
            start_pos = curr_pos;
            tokens.push_back(next_token());
        }

        tokens.emplace_back(TokenType::EOF_TOKEN, "", Span{line, column});
        return tokens;
    }

    Token Lexer::next_token() {
        skip_whitespace();

        start_pos = curr_pos;
        Span token_start_span{line, column};

        if (is_at_end()) {
            return Token(TokenType::EOF_TOKEN, "", token_start_span);
        }

        char c = advance();

        if (std::isalpha(c) || c == '_') {
            while (std::isalnum(peek()) || peek() == '_') {
                advance();
            }

            std::string_view lexeme = get_current_lexeme();
            auto it = KEYWORDS.find(lexeme);
            TokenType type = (it != KEYWORDS.end()) ? it->second : TokenType::IDENTIFIER;

            return Token(type, lexeme, token_start_span);
        }

        if (std::isdigit(c)) {
            bool is_float = false;
            while (std::isdigit(peek())) {
                advance();
            }

            if (peek() == '.' && std::isdigit(source[curr_pos + 1])) {
                is_float = true;
                advance();
                while (std::isdigit(peek())) {
                    advance();
                }
            }

            TokenType type = is_float ? TokenType::FLOAT : TokenType::INT;
            return Token(type, get_current_lexeme(), token_start_span);
        }

        switch (c) {
            case '(': return Token(TokenType::LEFT_PAREN, "(", token_start_span);
            case ')': return Token(TokenType::RIGHT_PAREN, ")", token_start_span);
            case '{': return Token(TokenType::LEFT_BRACE, "{", token_start_span);
            case '}': return Token(TokenType::RIGHT_BRACE, "}", token_start_span);
            case '[': return Token(TokenType::LEFT_BRACKET, "[", token_start_span);
            case ']': return Token(TokenType::RIGHT_BRACKET, "]", token_start_span);
            case ',': return Token(TokenType::COMMA, ",", token_start_span);
            case '.': 
                if (match('.')) return Token(TokenType::DOT_DOT, "..", token_start_span); 
                return Token(TokenType::DOT, ".", token_start_span);
            case ';': return Token(TokenType::SEMICOLON, ";", token_start_span);
            case ':': return Token(TokenType::COLON, ":", token_start_span);
            case '+': 
                if (match('=')) return Token(TokenType::PLUS_EQUAL, "+=", token_start_span);
                return Token(TokenType::PLUS, "+", token_start_span);
            case '-': 
                if (match('=')) return Token(TokenType::MINUS_EQUAL, "-=", token_start_span);
                return Token(TokenType::MINUS, "-", token_start_span);
            case '*': 
                if (match('=')) return Token(TokenType::STAR_EQUAL, "*=", token_start_span);
                return Token(TokenType::STAR, "*", token_start_span);
            case '/': 
                if (match('=')) return Token(TokenType::SLASH_EQUAL, "/=", token_start_span);
                return Token(TokenType::SLASH, "/", token_start_span);
            case '%': return Token(TokenType::PERCENT, "%", token_start_span);
            // Operators
            case '!':
                if (match('=')) return Token(TokenType::BANG_EQUAL, "!=", token_start_span);
                return Token(TokenType::BANG, "!", token_start_span);
            case '=':
                if (match('=')) return Token(TokenType::EQUAL_EQUAL, "==", token_start_span);
                if (match('>')) return Token(TokenType::ARROW, "=>", token_start_span);
                return Token(TokenType::EQUAL, "=", token_start_span);
            case '<':
                if (match('=')) return Token(TokenType::LESS_EQUAL, "<=", token_start_span);
                return Token(TokenType::LESS, "<", token_start_span);
            case '>':
                if (match('=')) return Token(TokenType::GREATER_EQUAL, ">=", token_start_span);
                return Token(TokenType::GREATER, ">", token_start_span);
            case '"': {
                while (peek() != '"' && !is_at_end()) {
                    if (peek() == '\\') {
                        advance();
                        if (!is_at_end()) {
                            advance();
                        }
                        continue;
                    }
                    advance();
                }
                if (is_at_end()) {
                    reporter.report(token_start_span, ErrorType::Lexical, "Unterminated string literal.");
                    return Token(TokenType::ERROR, get_current_lexeme(), token_start_span);
                }
                advance();
                return Token(TokenType::STRING, get_current_lexeme(), token_start_span);
            }

            default:
                auto msg = fmt::format("Unexpected format '{}'.", c);
                reporter.report(token_start_span, ErrorType::Lexical, msg);
                return Token(TokenType::ERROR, get_current_lexeme(), token_start_span);
        }

    }

    bool Lexer::is_at_end() {
        return curr_pos >= source.size();
    }

    char Lexer::peek() {
        if (is_at_end()) return '\0';
        return source[curr_pos];
    }

    char Lexer::advance() {
        char c = source[curr_pos];
        curr_pos++;

        if (c == '\n') {
            line++;
            column = 1;
        } else
            column++;
        return c;
    }

    void Lexer::skip_whitespace() {
        while (!is_at_end()) {
            char c = peek();
            if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
                advance();
            } else if (c == '#') {
                while(peek() != '\n' && !is_at_end()) {
                    advance();
                }
            } else
                break;
        }
    }

    bool Lexer::match(char expected) {
        if (is_at_end()) return false;
        if (source[curr_pos] != expected) return false;

        advance();
        return true;
    }

    std::string_view Lexer::get_current_lexeme() {
        return source.substr(start_pos, curr_pos - start_pos);
    }
}
