#pragma once
#include <string>
#include <vector>
#include "pyle/token.hpp"  

namespace pyle {

    enum class ErrorType {
        Lexical,
        Syntax,
        Compile
    };

    inline std::string_view to_string(ErrorType type) {
        switch (type) {
            case ErrorType::Lexical: return "LexicalError";
            case ErrorType::Syntax:  return "SyntaxError";
            case ErrorType::Compile: return "CompileError";
            default:                 return "Error";
        }
    }


    class ErrorReporter {
    private:
        bool had_error = false;
        std::vector<std::string> error_messages;

    public:
        void report(const Span loc, ErrorType type, const std::string& message);
        bool has_errors() const;
        void print_errors() const;
        void clear();
    };


    enum class RuntimeError {
        Type, Name, Index, ZeroDivision, StackUnderflow, OutOfBounds, ArgumentError, Runtime
    };

    inline std::string_view err_to_string(const RuntimeError& err) {
        switch (err) {
            case RuntimeError::Type: return "TypeError";
            case RuntimeError::Name: return "NameError";
            case RuntimeError::Index: return "IndexError";
            case RuntimeError::StackUnderflow: return "StackUnderFlowError";
            case RuntimeError::ArgumentError: return "ArgumentError";
            case RuntimeError::OutOfBounds: return "OutOfBoundsError";
            default: return "RuntimeError";
        }
    }


}