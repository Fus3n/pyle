
#include "pyle/pyle.hpp"


#include "pyle/compiler.hpp"
#include "pyle/error_reporter.hpp"
#include "pyle/lexer.hpp"
#include "pyle/parser.hpp"
#include "pyle/ast.hpp"
#include "pyle/debug.hpp"


namespace pyle {
    bool Pyle::execute(std::string_view source, bool disassamble) {
        ErrorReporter reporter;

        Lexer lexer(source, reporter);
        auto tokens = lexer.tokenize();
        if (reporter.has_errors()) {
            reporter.print_errors();
            return false;
        }

        Parser parser(tokens, reporter);
        std::vector<std::unique_ptr<Stmt>> ast = parser.parse();
        if (reporter.has_errors() || ast.empty()) {
            reporter.print_errors();
            return false;
        }

        Compiler compiler(vm, reporter);
        Chunk chunk = compiler.compile(ast);
        if (reporter.has_errors()) {
            reporter.print_errors();
            return false;
        }

        if (disassamble) {
            disassemble_chunk(vm, chunk, "Main Script");
        }

        vm.execute(chunk);
        return true;
    }
}
