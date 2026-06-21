#pragma once
#include <string_view>
#include <unordered_map>

namespace pyle {

    enum class TokenType {
        LEFT_PAREN, RIGHT_PAREN,  
        LEFT_BRACE, RIGHT_BRACE, 
        LEFT_BRACKET, RIGHT_BRACKET, 
        COMMA, DOT, SEMICOLON,    
        COLON,                    
        PLUS, MINUS, STAR, SLASH, PERCENT,
        PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL,

        BANG, BANG_EQUAL,         
        EQUAL, EQUAL_EQUAL,        
        GREATER, GREATER_EQUAL,    
        LESS, LESS_EQUAL,          

        IDENTIFIER,                
        STRING,                  
        INT,
        FLOAT,

        AND, OR, NOT,         
        IF, ELSE, ELIF,            
        FOR, WHILE,                
        FUNC, RETURN,             
        LET, GLOBAL,              
        STRUCT,                     
        NIL, TRUE, FALSE,          

        // Special
        ERROR,
        EOF_TOKEN
    };

    struct Span {
        size_t line;
        size_t column;
    };

    struct Token {
        TokenType type;
        std::string_view lexeme;
        Span selection;

        Token(const TokenType type, const std::string_view lexeme, const Span selection): type(type), lexeme(lexeme), selection(selection) {}
    };

    const std::unordered_map<std::string_view, TokenType> KEYWORDS = {
        {"and",    TokenType::AND},
        {"or",     TokenType::OR},
        {"not",    TokenType::NOT},
        {"if",     TokenType::IF},
        {"else",   TokenType::ELSE},
        {"elif",   TokenType::ELIF},
        {"for",    TokenType::FOR},
        {"while",  TokenType::WHILE},
        {"func",   TokenType::FUNC},
        {"return", TokenType::RETURN},
        {"let",    TokenType::LET},
        {"global", TokenType::GLOBAL},
        {"struct", TokenType::STRUCT},
        {"nil",    TokenType::NIL},
        {"true",   TokenType::TRUE},
        {"false",  TokenType::FALSE},
    };

}
