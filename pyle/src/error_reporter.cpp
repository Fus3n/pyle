#include <iostream>
#include <fmt/format.h>
#include "pyle/error_reporter.hpp"

namespace pyle {
    void ErrorReporter::report(const Span loc, ErrorType type, const std::string& message) {
        had_error = true;
        error_messages.push_back(
            fmt::format("{}: {} {}:{}",
                        to_string(type), message, loc.line + 1, loc.column)
        );
    }

    bool ErrorReporter::has_errors() const {
        return had_error;
    }

    void ErrorReporter::print_errors() const {
        for (const auto& err : error_messages) {
            std::cerr << err << "\n";
        }
    }

    void ErrorReporter::clear() {
        had_error = false;
        error_messages.clear();
    }

}