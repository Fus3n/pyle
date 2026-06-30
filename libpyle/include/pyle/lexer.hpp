#pragma once

#include <string_view>
#include <vector>
#include "pyle/error_reporter.hpp"
#include "pyle/token.hpp"

namespace pyle {
    class Lexer {
    public:
        Lexer(std::string_view source, ErrorReporter& reporter): source(source), reporter(reporter) {}
        std::vector<Token> tokenize();

    private:
        Token next_token();
        bool is_at_end();
        char  peek();
        char advance();
        void skip_whitespace();
        bool match(char expected);
        std::string_view get_current_lexeme();


        size_t curr_pos = 0;
        size_t start_pos = 0;
        size_t line = 0;
        size_t column = 0;

        std::string_view source;
        ErrorReporter& reporter;
    };
}

