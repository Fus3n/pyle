#include <iostream>
#include <string>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"

// This example demonstrates how to instantiate the Pyle interpreter, register the standard core library, and execute Pyle scripts directly from C++ source strings.
int main() {
    pyle::Pyle interpreter;

    // Register Pyle standard core functions & modules (like print, printf, format, os, etc.)
    pyle::register_core_natives(interpreter.vm);

    std::string code = R"(
        fn calculate_factorial(n) {
            if n <= 1 {
                return 1
            }
            return n * calculate_factorial(n - 1)
        }

        let num = 5
        let result = calculate_factorial(num)
        printf("Factorial of {}, is: {}", num, result)
    )";

    std::cout << "Running Pyle Basic Embedding Example\n\n";

    // execute takes: (source_code, disassemble_flag, virtual_filename)
    bool success = interpreter.execute(code, false, "factorial.pyl");

    if (!success) {
        std::cerr << "Script execution failed!\n";
        return 1;
    }

    std::cout << "\nBasic Embedding Example Finished\n";
    return 0;
}