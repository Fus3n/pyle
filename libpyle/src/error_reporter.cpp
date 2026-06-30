#include <fmt/format.h>
#include "pyle/error_reporter.hpp"

namespace pyle {
    static std::string_view get_line_of_code(std::string_view src, size_t target_line) {
        size_t current_line = 0;
        size_t start = 0;
        for (size_t i = 0; i < src.size(); ++i) {
            if (src[i] == '\n') {
                if (current_line == target_line) {
                    return src.substr(start, i - start);
                }
                start = i + 1;
                current_line++;
            }
        }
        if (current_line == target_line && start < src.size()) {
            return src.substr(start);
        }
        return "";
    }

    static std::string get_hint(ErrorType type, const std::string& message) {
        if (type == ErrorType::Lexical) {
            if (message.find("Unterminated string") != std::string::npos) {
                return "Ensure your string is closed with a double quote (\").";
            }
        }
        if (type == ErrorType::Syntax) {
            if (message.find("Expected '}' after block") != std::string::npos || message.find("never closed") != std::string::npos) {
                return "The block or container has an unclosed delimiter. Ensure every opening brace, bracket, or parenthesis has a matching closer.";
            }
            if (message.find("Expected newline or ';'") != std::string::npos) {
                return "Make sure each statement ends with a newline or a semicolon ';'.";
            }
            if (message.find("Expected expression") != std::string::npos) {
                return "Provide a valid value, variable, or grouping inside parentheses.";
            }
            if (message.find("Invalid assignment target") != std::string::npos) {
                return "Assignment targets must be variables, struct fields (e.g., self.x), or array indexes (e.g., arr[0]).";
            }
            if (message.find("Expected parameter name") != std::string::npos) {
                return "Parameters must be valid identifiers (alphabetic characters or underlines).";
            }
        }
        if (type == ErrorType::Compile) {
            if (message.find("Undefined global") != std::string::npos) {
                return "Check for spelling mistakes, or ensure the variable is declared with 'let' before use.";
            }
            if (message.find("Cannot use 'break'") != std::string::npos) {
                return "Move this 'break' statement inside a 'while' or 'for' loop body.";
            }
        }
        return "";
    }

    void ErrorReporter::report(const Span loc, ErrorType type, const std::string& message, size_t length) {
        had_error = true;
        errors.push_back({loc, type, message, length});
    }

    bool ErrorReporter::has_errors() const {
        return had_error;
    }

    void ErrorReporter::print_errors() const {
        if (errors.empty()) return;
        
        const auto& err = errors.front(); 
        std::string_view type_str = to_string(err.type);
        
        fmt::print(stderr, "\033[1;31m{}:\033[0m \033[1m{}\033[0m\n", type_str, err.message);
        fmt::print(stderr, "   --> {}:{}:{}\n", script_name, err.loc.line + 1, err.loc.column);
        
        std::string_view line_text = get_line_of_code(source, err.loc.line);
        if (!line_text.empty()) {
            size_t line_num = err.loc.line + 1;
            fmt::print(stderr, " {:4d} | {}\n", line_num, line_text);
            
            std::string carets = "        | ";
            for (size_t col = 1; col < err.loc.column && (col - 1) < line_text.size(); ++col) {
                char c = line_text[col - 1];
                if (c == '\t') {
                    carets += '\t';
                } else {
                    carets += ' ';
                }
            }
            
            carets += "\033[1;31m";
            size_t safe_length = (err.length > 0) ? err.length : 1;
            for (size_t i = 0; i < safe_length; ++i) {
                carets += '^';
            }
            carets += "\033[0m";
            fmt::print(stderr, "{}\n", carets);
        }
        
        std::string hint = get_hint(err.type, err.message);
        if (!hint.empty()) {
            fmt::print(stderr, "    \033[1;36mHint:\033[0m {}\n", hint);
        }
        fmt::print(stderr, "\n");
    }

    void ErrorReporter::clear() {
        had_error = false;
        errors.clear();
    }
    
    std::string get_runtime_hint(const RuntimeError& type, const std::string& msg) {
        if (type == RuntimeError::Name) {
            if (msg.find("Struct has no field") != std::string::npos) {
                return "Ensure the field was declared in the struct template and is spelled correctly.";
            }
            if (msg.find("Method") != std::string::npos && msg.find("not found") != std::string::npos) {
                return "The method is not defined on this struct. Make sure it is inside the struct block and spelled correctly.";
            }
        }
        if (type == RuntimeError::Type) {
            if (msg.find("Only structs have fields") != std::string::npos) {
                return "You attempted to access a field (using '.') on a non-struct value. Ensure the variable is initialized to a struct instance.";
            }
            if (msg.find("Object is not callable") != std::string::npos) {
                return "You tried to call a value that isn't a function, closure, or struct constructor. Check if you overshadowed its name.";
            }
            if (msg.find("Expected object with method") != std::string::npos) {
                return "The target value does not support method calls. Method calls are only supported on structs, arrays, and strings.";
            }
        }
        if (type == RuntimeError::ArgumentError) {
            if (msg.find("_init expects") != std::string::npos) {
                return "Make sure the number of arguments passed to the constructor matches the parameter list of your '_init' method.";
            }
            if (msg.find("Too many arguments") != std::string::npos) {
                return "The constructor was given more parameters than the fields declared on the struct.";
            }
        }
        if (type == RuntimeError::Index) {
            if (msg.find("out of bounds") != std::string::npos) {
                return "Ensure your index is between 0 and size - 1.";
            }
            if (msg.find("Key not found") != std::string::npos) {
                return "The specified key does not exist in this map.";
            }
        }
        return "";
    }
}