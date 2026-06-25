#include <iostream>
#include <string>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"
#include "pyle/binder.hpp"

// This example demonstrates how to wrap and expose raw, free-standing C++ functions directly to Pyle's global scope.

// Define C++ free functions to expose
int add_integers(int a, int b) {
    return a + b;
}

void log_from_cpp(const std::string& message) {
    std::cout << "[C++ Host Log] " << message << "\n";
}

int main() {
    pyle::Pyle interpreter;
    pyle::register_core_natives(interpreter.vm);

    // Wrap and bind C++ functions directly to Pyle's global namespace
    pyle::bind_function<&add_integers>(interpreter.vm, "add");
    pyle::bind_function<&log_from_cpp>(interpreter.vm, "log");

    // Another way is using FreeFnDeducer wrap your function pointer into a standard pyle::NativeFn
    // interpreter.vm.define_native("add", pyle::FreeFnDeducer<&add_integers>::wrap);

    std::string code = R"(
        # Call C++ function: add_integers(45, 12)
        let total = add(45, 12)

        # Call C++ function: log_from_cpp(...)

        log(format("The computed total from C++ is: {}", total))
    )";

    std::cout << "Running Pyle Function Binding Example\n\n";

    bool success = interpreter.execute(code, false, "function_binding.pyl");

    if (!success) {
        std::cerr << "Script execution failed!\n";
        return 1;
    }

    std::cout << "\nFunction Binding Example Finished\n";
    return 0;
}