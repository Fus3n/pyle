#include "pyle/std/std_core.hpp"
#include "pyle/value.hpp"
#include "pyle/config.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <atomic>
#include <fmt/args.h>
#include <pyle/vm.hpp>
#include <pyle/binder.hpp>
#include "pyle/binder.hpp"
#include "pyle/lexer.hpp"
#include "pyle/parser.hpp"
#include "pyle/compiler.hpp"
#include "pyle/std/std_core_modules.hpp"
#include "pyle/std/std_future.hpp"
#include <fmt/color.h>
#include "pyle/std/prelude.hpp" 
#include "pyle/std/std_file_module.hpp"
#include <filesystem>

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    using DynLib = HMODULE;
    #define DynLibOpen(path) LoadLibraryA(path)
    #define DynLibSym(lib, name) reinterpret_cast<void*>(GetProcAddress(lib, name))
    #define DynLibClose(lib) FreeLibrary(lib)
#elif defined(__linux__) || defined(__APPLE__)
    #include <dlfcn.h>
    using DynLib = void*;
    #define DynLibOpen(path) dlopen(path, RTLD_NOW | RTLD_LOCAL)
    #define DynLibSym(lib, name) dlsym(lib, name)
    #define DynLibClose(lib) dlclose(lib)
#else
    #error "Unsupported platform for .pyled native modules"
#endif


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
                    store.push_back("none");
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

    Value native_input(VM& vm, ArgView args) {
        if (args.size() > 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "input() takes at most 1 argument (optional prompt).");
            return Value();
        }
        if (args.size() == 1) {
            std::cout << vm.value_to_string(args[0]);
        }
        std::string line;
        if (!std::getline(std::cin, line)) {
            vm.runtime_error(RuntimeError::Runtime, "Failed to read from stdin.");
            return Value();
        }
        HeapIdx idx = vm.intern_string(line);
        return Value(Value::Tag::StringRef, idx);
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

        // Try loading as .pyled native module
        {
            std::string mod_name = std::get<std::string>(vm.get_heap_object(mod_name_idx).data);
            std::string pyled_filepath = mod_name;
            if (pyled_filepath.size() < 6 || pyled_filepath.substr(pyled_filepath.size() - 6) != ".pyled") {
                pyled_filepath += ".pyled";
            }

            auto try_load_pyled = [&](const std::string& path) -> std::optional<Value> {
                std::ifstream f(path);
                if (!f.is_open()) return std::nullopt;
                f.close();

                DynLib lib = DynLibOpen(path.c_str());
                if (!lib) return std::nullopt;

                auto version_ptr = reinterpret_cast<int64_t*>(DynLibSym(lib, "pyle_module_version"));
                if (!version_ptr || *version_ptr != PYLE_MODULE_ABI_VERSION) {
                    DynLibClose(lib);
                    return std::nullopt;
                }

                auto init_fn = reinterpret_cast<int64_t(*)(void*)>(DynLibSym(lib, "pyle_module_init"));
                if (!init_fn) {
                    DynLibClose(lib);
                    return std::nullopt;
                }

                HeapIdx map_idx(init_fn(static_cast<void*>(&vm)));
                Value module_val(Value::Tag::MapRef, map_idx);
                vm.loaded_modules[mod_name_idx] = module_val;
                return module_val;
            };

            if (auto result = try_load_pyled(pyled_filepath)) return *result;

            for (const auto& dir : vm.import_paths) {
                if (auto result = try_load_pyled(dir + pyled_filepath)) return *result;
            }
        }

        std::string mod_name = std::get<std::string>(vm.get_heap_object(mod_name_idx).data);
        std::string filepath = mod_name;
        if (filepath.size() < 4 || filepath.substr(filepath.size() - 4) != ".pyl") {
            filepath += ".pyl";
        }

        auto try_read_file =  [](const std::string& path) -> std::optional<std::string>   {
            std::ifstream file(path);
            if (!file.is_open()) return std::nullopt;
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        };

        std::optional<std::string> code_opt = try_read_file(filepath);
        std::string resolved_path = filepath;

        if (!code_opt && filepath.size() >= 2 && filepath[0] == '.' && (filepath[1] == '/' || filepath[1] == '\\')) {
            std::string script_dir = std::filesystem::path(vm.script_name).parent_path().string();
            if (!script_dir.empty()) {
                std::string rel_path = script_dir + "/" + filepath;
                code_opt = try_read_file(rel_path);
                if (code_opt) resolved_path = rel_path;
            }
        }

        if (!code_opt) {
            for (const auto& dir : vm.import_paths) {
                std::string test_path = dir + filepath;
                code_opt = try_read_file(test_path);
                if (code_opt) {
                    resolved_path = test_path; 
                    break;
                }
            }
        }

        if (!code_opt) {
            vm.runtime_error(RuntimeError::Name, fmt::format("Module '{}' not found (checked import paths).", mod_name));
            return Value();
        }
        
        std::string source = std::move(*code_opt);
        
        // Save the caller's active global storage (as an index) and give the
        // module its own storage seeded with the caller's builtins.
        vm.saved_globals_stack.push_back(vm.globals_idx);
        vm.saved_slot_maps_stack.push_back(std::move(vm.global_slot_map));

        ArrayType module_storage;
        if (vm.global_slots) {
            module_storage.assign(vm.global_slots->begin(),
                                  vm.global_slots->begin() + vm.builtin_count);
        }
        HeapIdx module_storage_idx = vm.alloc(Object(std::move(module_storage)));
        vm.globals_idx = module_storage_idx;
        vm.global_slots = &std::get<ArrayType>(vm.get_heap_object(module_storage_idx).data);
        vm.global_slot_map = vm.builtin_slot_map;
        
        ErrorReporter reporter(source, resolved_path);
        Lexer lexer(source, reporter);
        auto tokens = lexer.tokenize();
        
        std::string_view saved_source_code = vm.source_code;
        std::string_view saved_script_name = vm.script_name;
        vm.source_code = source;
        vm.script_name = resolved_path;
        vm.source_cache[resolved_path] = source;
        
        bool success = false;
        
        if (!reporter.has_errors()) {
            Parser parser(tokens, reporter);
            auto ast = parser.parse();

            if (!reporter.has_errors()) {
                Compiler compiler(vm, reporter);
                Chunk chunk = compiler.compile(ast);
                if (!reporter.has_errors()) {
                    vm.execute(chunk);
                    success = !vm.is_panicked();
                }
            }
        }
        
        vm.script_name = saved_script_name;
        vm.source_code = saved_source_code;
        
        if (!success) {
            vm.globals_idx = vm.saved_globals_stack.back();
            vm.global_slots = (vm.globals_idx == HeapIdx(-1))
                ? &vm.root_globals
                : &std::get<ArrayType>(vm.get_heap_object(vm.globals_idx).data);
            vm.saved_globals_stack.pop_back();
            vm.global_slot_map = std::move(vm.saved_slot_maps_stack.back());
            vm.saved_slot_maps_stack.pop_back();
            reporter.print_errors();
            if (!vm.is_panicked()) {
                vm.set_panicked(true);
            }
            return Value();
        }
        
        // Tag every module-level function/closure with this module's persistent
        // global storage (module_storage_idx) so that when the function runs, the
        // VM aliases its globals instead of the caller's.
        auto tag_function = [&](HeapIdx fn_idx) {
            if (fn_idx != 0) {
                Function& fn = std::get<Function>(vm.get_heap_object(fn_idx).data);
                if (fn.module_env == 0) {
                    fn.module_env = module_storage_idx;
                }
            }
        };
        for (const auto& [var_name_idx, slot_idx] : vm.global_slot_map) {
            if (slot_idx >= static_cast<int>(vm.builtin_count)) {
                Value v = (*vm.global_slots)[slot_idx];
                if (v.tag == Value::Tag::FuncRef) {
                    tag_function(v.as_ref);
                } else if (v.tag == Value::Tag::ClosureRef) {
                    Closure& clo = std::get<Closure>(vm.get_heap_object(v.as_ref).data);
                    tag_function(clo.function);
                } else if (v.tag == Value::Tag::StructTypeRef) {
                    // Struct methods are compiled as standalone functions stored on
                    // the type; they also need their module's globals so they run
                    // with the correct globals when invoked cross-module.
                    StructType& type = std::get<StructType>(vm.get_heap_object(v.as_ref).data);
                    for (auto& [_, fn_idx] : type.methods) tag_function(fn_idx);
                    for (HeapIdx fn_idx : type.special_methods) tag_function(fn_idx);
                    for (auto& [_, fn_idx] : type.setters) tag_function(fn_idx);
                    for (auto& [_, fn_idx] : type.getters) tag_function(fn_idx);
                }
            }
        }

        MapType module_map;
        for (const auto& [var_name_idx, slot_idx] : vm.global_slot_map) {
            if (slot_idx >= static_cast<int>(vm.builtin_count)) {
                Value key(Value::Tag::StringRef, var_name_idx);
                module_map[key] = (*vm.global_slots)[slot_idx];
            }
        }

        // Restore caller's active global storage on success.
        vm.globals_idx = vm.saved_globals_stack.back();
        vm.global_slots = (vm.globals_idx == HeapIdx(-1))
            ? &vm.root_globals
            : &std::get<ArrayType>(vm.get_heap_object(vm.globals_idx).data);
        vm.saved_globals_stack.pop_back();
        vm.global_slot_map = std::move(vm.saved_slot_maps_stack.back());
        vm.saved_slot_maps_stack.pop_back();
        
        HeapIdx map_idx = vm.alloc(Object(std::move(module_map)));
        vm.get_heap_object<MapObject>(map_idx).is_module = true;
        Value val(Value::Tag::MapRef, map_idx);
        vm.loaded_modules[mod_name_idx] = val;
        return val;
    }

    Value native_coro_constructor(VM& vm, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "Coro constructor expects exactly 1 argument (function or closure).");
            return Value();
        }

        Value callee = args[0];
        HeapIdx closure_idx = 0;

        if (callee.tag == Value::Tag::ClosureRef) {
            closure_idx = callee.as_ref;
        } else if (callee.tag == Value::Tag::FuncRef) {
            closure_idx = vm.alloc(Object(Closure{callee.as_ref}));
        } else {
            vm.runtime_error(RuntimeError::Type, "Coro constructor argument must be a valid callable function or closure.");
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

        {
            Function& fn = std::get<Function>(vm.get_heap_object(
                std::get<Closure>(vm.get_heap_object(closure_idx).data).function).data);
            if (fn.module_env != 0) {
                entry_frame.module_swap = true;
                entry_frame.saved_globals_idx = vm.globals_idx;
                entry_frame.module_env_idx = fn.module_env;
                coro.saved_globals_idx = fn.module_env;
            }
        }

        coro.frames[coro.frame_count++] = entry_frame;

        coro.stack[0] = Value(Value::Tag::ClosureRef, closure_idx);
        coro.sp = coro.stack + 1;

        HeapIdx coro_idx = vm.alloc(Object(std::move(coro)));
        
        std::get<Coroutine>(vm.get_heap_object(coro_idx).data).self_idx = coro_idx;

        return Value(Value::Tag::CoroutineRef, coro_idx);
    }


    Value native_add_import_path(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "add_import_path expects 1 string argument.");
            return Value();
        }

        std::string new_path = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        vm.add_import_path(new_path);
        return Value();
    }

    Value native_typeof(VM& vm, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "typeof expects exactly 1 argument.");
            return Value();
        }

        return Value(Value::Tag::StringRef, vm.intern_string(args[0].tag_to_string()));
    }

    pyle::Value native_bytes(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != pyle::Value::Tag::ArrayRef) {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, "Bytes() expects 1 Array argument.");
            return pyle::Value();
        }

        const auto& arr = std::get<pyle::ArrayType>(vm.get_heap_object(args[0].as_ref).data);
        pyle::BytesType bytes;
        bytes.reserve(arr.size());

        for (const auto& val : arr) {
            if (val.tag != pyle::Value::Tag::Int) {
                vm.runtime_error(pyle::RuntimeError::Type, "Bytes() array must only contain integers.");
                return pyle::Value();
            }
            int64_t num = val.as_int;
            if (num < 0 || num > 255) {
                vm.runtime_error(pyle::RuntimeError::Runtime, "Byte values must be between 0 and 255.");
                return pyle::Value();
            }
            bytes.push_back(static_cast<uint8_t>(num));
        }

        pyle::HeapIdx idx = vm.alloc(pyle::Object(std::move(bytes)));
        return pyle::Value(pyle::Value::Tag::BytesRef, idx);
    }

    static pyle::Value native_background_tick(pyle::VM& vm, pyle::ArgView) {
        static std::atomic<bool> recursing{false};
        if (recursing.exchange(true)) return pyle::Value();
        for (auto& fn : pyle::background_ticks()) {
            fn(vm, {});
        }
        recursing.store(false);
        return pyle::Value();
    }

    static pyle::Value native_ready_pop(pyle::VM& vm, pyle::ArgView) {
        if (pyle::ready_tasks_owner() != 0 && vm.active_coroutine_idx != pyle::ready_tasks_owner()) {
            return pyle::Value();
        }
        std::lock_guard<std::mutex> lock(pyle::ready_tasks_mutex());
        auto& q = pyle::ready_tasks();
        for (auto it = q.begin(); it != q.end();) {
            pyle::Value v = *it;
            if (v.tag != pyle::Value::Tag::CoroutineRef) {
                it = q.erase(it);
                continue;
            }
            auto* c = std::get_if<pyle::Coroutine>(&vm.get_heap_object(v.as_ref).data);
            if (!c) {
                it = q.erase(it);
                continue;
            }
            if (c->state == pyle::Coroutine::State::Dead) {
                it = q.erase(it);
                continue;
            }
            if (c->state == pyle::Coroutine::State::Suspended && v.as_ref != vm.active_coroutine_idx) {
                it = q.erase(it);
                return v;
            }
            ++it;
        }
        return pyle::Value();
    }

    void register_core_natives(VM& vm, bool load_core_modules) {
        pyle::bind_function<native_print>(vm, "print");
        pyle::bind_function<native_printf>(vm, "printf");
        pyle::bind_function<native_format>(vm, "format");
        pyle::bind_function<native_input>(vm, "input");
        pyle::bind_function<native_import>(vm, "import");
        pyle::bind_function<native_add_import_path>(vm, "add_import_path");
        pyle::bind_function<native_typeof>(vm, "typeof");
        pyle::bind_function<native_coro_constructor>(vm, "Coro");
        pyle::bind_function<native_bytes>(vm, "Bytes");
        pyle::bind_function<native_background_tick>(vm, "__tick");
        pyle::bind_function<native_ready_pop>(vm, "__next_ready_task");

        auto add_type_const = [&](const std::string& name) {
            pyle::HeapIdx name_idx = vm.intern_string(name);
            pyle::HeapIdx val_idx = vm.intern_string(name);
            int slot = vm.declare_global(name_idx);
            (*vm.global_slots)[slot] = pyle::Value(pyle::Value::Tag::StringRef, val_idx);
        };
        add_type_const("int");
        add_type_const("float");
        add_type_const("bool");
        add_type_const("string");
        add_type_const("array");
        add_type_const("map");
        add_type_const("bytes");
        add_type_const("function");
        add_type_const("none");
        add_type_const("struct");
        add_type_const("range");
        add_type_const("iterator");
        add_type_const("coro");
        add_type_const("native_function");
        add_type_const("native_object");

        pyle::register_core_future(vm); 
        pyle::register_file_module(vm);


        if (!vm.builtins_finalized) {
            std::string_view prelude_source = PRELUDE_SOURCE;
            ErrorReporter prelude_reporter(prelude_source, "<prelude>");
            Lexer prelude_lexer(prelude_source, prelude_reporter);
            Parser prelude_parser(prelude_lexer.tokenize(), prelude_reporter);
            auto prelude_ast = prelude_parser.parse();

            Compiler prelude_compiler(vm, prelude_reporter);
            Chunk prelude_chunk = prelude_compiler.compile(prelude_ast);

            if (prelude_reporter.has_errors()) {
                fmt::print(
                    stderr,
                    fg(fmt::color::red) | fmt::emphasis::bold,
                    "Failed to compile standard library prelude:\n"
                );
                prelude_reporter.print_errors();
                exit(1);
            }

            vm.execute(prelude_chunk);
            vm.builtins_finalized = true;
        }

        if (load_core_modules) {
            pyle::register_core_modules(vm);
        }

        if (vm.builtin_count == 0) {
            vm.builtin_count = vm.global_slots->size();
            vm.builtin_slot_map = vm.global_slot_map;
        }
    }
}
