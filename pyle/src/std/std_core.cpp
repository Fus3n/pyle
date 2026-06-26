#include "pyle/std/std_core.hpp"
#include "pyle/value.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <fmt/args.h> 
#include <pyle/vm.hpp>
#include <pyle/binder.hpp>
#include "pyle/binder.hpp"
#include "pyle/lexer.hpp"
#include "pyle/parser.hpp"
#include "pyle/compiler.hpp"
#include "pyle/std/std_core_mods.hpp"

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

        const std::string& fmt_str = std::get<std::string>(vm.get_heap_object(fmt_val.as_ref).data);

        fmt::dynamic_format_arg_store<fmt::format_context> store; 
        for (size_t i = 1; i < args.size(); ++i) {
            const Value& arg = args[i];
            switch (arg.tag) {
                case Value::Tag::Int:
                    store.push_back(arg.as_int);
                    break;
                case Value::Tag::Float:
                    store.push_back(arg.as_float); 
                    break;
                case Value::Tag::Bool:
                    store.push_back(arg.as_bool);
                    break;
                case Value::Tag::None:
                    store.push_back("null");
                    break;
                default:
                    store.push_back(vm.value_to_string(arg));
                    break;
            }
        }

        try {
            return fmt::vformat(fmt_str, store); 
        } catch (const fmt::format_error& err) {
            vm.runtime_error(RuntimeError::ArgumentError, std::string("Format error: ") + err.what());
            return "";
        }
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

    static std::optional<std::string> try_read_file(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return std::nullopt;
        std::stringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    Value native_import(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "import() expects 1 string argument.");
            return Value();
        }

        HeapIdx mod_name_idx = args[0].as_ref;
        
        auto cache_it = vm.loaded_modules.find(mod_name_idx);
        if (cache_it != vm.loaded_modules.end()) {
            return cache_it->second;
        }
        
        auto registry_it = vm.module_registry.find(mod_name_idx);
        if (registry_it != vm.module_registry.end()) {
            Value module_val = registry_it->second(vm);
            vm.loaded_modules[mod_name_idx] = module_val;
            return module_val;
        }
        
        std::string mod_name = std::get<std::string>(vm.get_heap_object(mod_name_idx).data);
        std::string filepath = mod_name;
        if (filepath.size() < 4 || filepath.substr(filepath.size() - 4) != ".pyl") {
            filepath += ".pyl";
        }
        
        auto code_opt = try_read_file(filepath);
        if (!code_opt) {
            vm.runtime_error(RuntimeError::Name, fmt::format("Module '{}' not found (checked: '{}').", mod_name, filepath));
            return Value();
        }
        
        std::string source = std::move(*code_opt);
        
        vm.saved_globals_stack.push_back(std::move(vm.global_slots));
        vm.saved_slot_maps_stack.push_back(std::move(vm.global_slot_map));
        
        const auto& saved_slots = vm.saved_globals_stack.back();
        vm.global_slots.assign(saved_slots.begin(), saved_slots.begin() + vm.builtin_count);
        
        vm.global_slot_map = vm.builtin_slot_map;
        
        ErrorReporter reporter(source, filepath);
        Lexer lexer(source, reporter);
        auto tokens = lexer.tokenize();
        bool success = false;
        
        if (!reporter.has_errors()) {
            Parser parser(tokens, reporter);
            auto ast = parser.parse();
            if (!reporter.has_errors() && !ast.empty()) {
                Compiler compiler(vm, reporter);
                Chunk chunk = compiler.compile(ast);
                if (!reporter.has_errors()) {
                    vm.source_code = source;
                    vm.script_name = filepath;
                    vm.execute(chunk);
                    success = !vm.is_panicked();
                }
            }
        }
        
        if (!success) {
            vm.global_slots = std::move(vm.saved_globals_stack.back());
            vm.saved_globals_stack.pop_back();
            vm.global_slot_map = std::move(vm.saved_slot_maps_stack.back());
            vm.saved_slot_maps_stack.pop_back();
            reporter.print_errors();
            vm.runtime_error(RuntimeError::Runtime, fmt::format("Failed to compile module '{}'.", mod_name));
            return Value();
        }
        
        MapType module_map;
        for (const auto& [var_name_idx, slot_idx] : vm.global_slot_map) {
            if (slot_idx >= static_cast<int>(vm.builtin_count)) {
                Value key(Value::Tag::StringRef, var_name_idx);
                module_map[key] = vm.global_slots[slot_idx];
            }
        }
        
        // Restore parent environment on success
        vm.global_slots = std::move(vm.saved_globals_stack.back());
        vm.saved_globals_stack.pop_back();
        vm.global_slot_map = std::move(vm.saved_slot_maps_stack.back());
        vm.saved_slot_maps_stack.pop_back();
        
        HeapIdx map_idx = vm.alloc(Object(std::move(module_map)));
        Value val(Value::Tag::MapRef, map_idx);
        vm.loaded_modules[mod_name_idx] = val;
        return val;
    }

    Value native_fiber_constructor(VM& vm, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "Fiber constructor expects exactly 1 argument (function or closure).");
            return Value();
        }

        Value callee = args[0];
        HeapIdx closure_idx = 0;

        if (callee.tag == Value::Tag::ClosureRef) {
            closure_idx = callee.as_ref;
        } else if (callee.tag == Value::Tag::FuncRef) {
            closure_idx = vm.alloc(Object(Closure{callee.as_ref}));
        } else {
            vm.runtime_error(RuntimeError::Type, "Fiber constructor argument must be a valid callable function or closure.");
            return Value();
        }

        Coroutine coro;
        coro.stack_capacity = 256; 
        coro.stack = new Value[coro.stack_capacity];
        coro.sp = coro.stack;

        coro.frame_capacity = 64; 
        coro.frames = new CallFrame[coro.frame_capacity];
        coro.frame_count = 0;

        coro.closure_idx = closure_idx;
        coro.state = Coroutine::State::Suspended;
        coro.is_main = false;

        CallFrame entry_frame;
        entry_frame.closure = closure_idx;
        entry_frame.ip = 0;
        entry_frame.stack_base = 1; 

        coro.frames[coro.frame_count++] = entry_frame;

        coro.stack[0] = Value(Value::Tag::ClosureRef, closure_idx);
        coro.sp = coro.stack + 1;

        HeapIdx coro_idx = vm.alloc(Object(std::move(coro)));
        
        std::get<Coroutine>(vm.get_heap_object(coro_idx).data).self_idx = coro_idx;

        return Value(Value::Tag::CoroutineRef, coro_idx);
    }


    void register_core_natives(VM& vm, bool load_core_modules) {
        pyle::bind_function<native_print>(vm, "print");
        pyle::bind_function<native_printf>(vm, "printf");
        pyle::bind_function<native_format>(vm, "format");
        pyle::bind_function<native_import>(vm, "import");
        pyle::bind_function<native_fiber_constructor>(vm, "Fiber");

        if (load_core_modules) {
            pyle::register_core_modules(vm);
        }
    }
}