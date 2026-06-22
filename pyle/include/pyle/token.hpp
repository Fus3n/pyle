#pragma once
#include <string_view>
#include <unordered_map>

namespace pyle {

    enum class TokenType {
        LEFT_PAREN, RIGHT_PAREN,  
        LEFT_BRACE, RIGHT_BRACE, 
        LEFT_BRACKET, RIGHT_BRACKET, 
        COMMA, DOT, DOT_DOT, SEMICOLON,    
        COLON,                    
        PLUS, MINUS, STAR, SLASH, PERCENT,
        PLUS_EQUAL, MINUS_EQUAL, STAR_EQUAL, SLASH_EQUAL,

        BANG, BANG_EQUAL,         
        EQUAL, EQUAL_EQUAL,        
        GREATER, GREATER_EQUAL,    
        LESS, LESS_EQUAL, ARROW,      

        IDENTIFIER,                
        STRING,                  
        INT,
        FLOAT,

        AND, OR, NOT,         
        IF, ELSE, ELIF,            
        FOR, WHILE, IN, LOOP, BREAK,        
        FN, RETURN,             
        LET, GLOBAL,              
        STRUCT,                     
        NONE, TRUE, FALSE,       

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
        {"in",     TokenType::IN},
        {"while",  TokenType::WHILE},
        {"while",  TokenType::WHILE},
        {"loop", TokenType::LOOP},
        {"break", TokenType::BREAK},
        {"fn",   TokenType::FN},
        {"return", TokenType::RETURN},
        {"let",    TokenType::LET},
        {"global", TokenType::GLOBAL},
        {"struct", TokenType::STRUCT},
        {"none",    TokenType::NONE},
        {"true",   TokenType::TRUE},
        {"false",  TokenType::FALSE},
    };

}
