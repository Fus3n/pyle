#include "pyle/std/std_core.hpp"
#include "pyle/value.hpp"
#include <chrono>
#include <iostream>

namespace pyle {

    std::string format_string_impl(VM& vm, ArgView args) {
        if (args.size() == 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "format/printf expects at least 1 argument.");
            return "";
        }
        
        const Value& fmt_val = args[0];
        if (fmt_val.tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::Type, "Format string must be a string.");
            return "";
        }
        
        const std::string& fmt = std::get<std::string>(vm.get_heap_object(fmt_val.as_ref).data);
        std::string result;
        
        size_t arg_idx = 1;
        for (size_t i = 0; i < fmt.size(); ++i) {
            char c = fmt[i];
            if (c == '{' && i + 1 < fmt.size() && fmt[i+1] == '}') {
                if (arg_idx >= args.size()) {
                    vm.runtime_error(RuntimeError::ArgumentError, "Not enough arguments for format string.");
                    return "";
                }
                result += vm.value_to_string(args[arg_idx]);
                arg_idx++;
                i++; 
            } else {
                result += c;
            }
        }
        return result;
    }

    Value native_print(VM& vm, ArgView args) {
        for (const auto& arg : args) {
            std::cout << vm.value_to_string(arg) << " ";
        }
        std::cout << std::endl;
        return Value();
    }

    Value native_printf(VM& vm, ArgView args) {
        std::string result = format_string_impl(vm, args);
        std::cout << result << std::endl;
        return Value();
    }

    Value native_format(VM& vm, ArgView args) {
        std::string result = format_string_impl(vm, args);
        HeapIdx idx = vm.intern_string(result);
        return Value(Value::Tag::StringRef, idx);
    }

    Value native_clock(VM& vm, ArgView args) { 
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();

        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        return Value(seconds);
    }

    void register_core_natives(VM& vm) {
        vm.define_native("print", native_print);
        vm.define_native("printf", native_printf);
        vm.define_native("format", native_format);
        vm.define_native("clock", native_clock);
    }
}