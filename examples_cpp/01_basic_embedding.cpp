#include <iostream>
#include <string>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"
#include "pyle/binder.hpp"

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
        std::cerr << "Base Script execution failed!\n";
        return 1;
    }

    // Call pyle function from cpp
    pyle::Value pyle_func = interpreter.vm.get_global("calculate_factorial");

    if (pyle_func.tag != pyle::Value::Tag::None) {
        pyle::Value raw_result = interpreter.vm.call_func(pyle_func, 5); // calc factorial of 5 from pyle

        // OR call it with raw Values without auto conversion
        // must include #include "pyle/value.hpp"
        // std::vector<pyle::Value> args = {
        //     pyle::Value(int64_t(5)) 
        // };
        // pyle::Value raw_result = interpreter.vm.call_func_raw(pyle_func, args); // calc factorial of 2


        double cpp_result = pyle::from_value<double>(interpreter.vm, raw_result);
        std::cout << "Result from pyle: " << cpp_result << "\n";
    }

    std::cout << "\nBasic Embedding Example Finished\n";
    return 0;
}