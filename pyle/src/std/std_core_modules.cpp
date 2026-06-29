#include "pyle/std/std_core_modules.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <chrono>
#include "pyle/std/std_future.hpp"
#include <thread>
#include <fstream>
#include <sstream>


namespace pyle {


    // OS MODULE
    Value os_time() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        return Value(seconds);
    }
    
    Value os_sys(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.system expects 1 string argument.");
            return Value();
        }
        std::string cmd = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        int result = std::system(cmd.c_str());
        return Value(static_cast<int64_t>(result));
    }

    pyle::Value os_read_file_async(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() < 1) {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, "os.read_file_async expects 1 argument (file path).");
            return pyle::Value();
        }

        std::string path = pyle::from_value<std::string>(vm, args[0]);

        auto* sp = new std::shared_ptr<pyle::Future>(std::make_shared<pyle::Future>());

        std::thread([sp_copy = *sp, path]() {
            std::ifstream file(path);
            if (!file.is_open()) {
                sp_copy->error = "Could not open file: " + path;
                sp_copy->failed = true;
                sp_copy->finished.store(true);
                return;
            }

            std::stringstream ss;
            ss << file.rdbuf();
            
            sp_copy->raw_string = ss.str();
            sp_copy->finished.store(true);
        }).detach();

        pyle::NativeObject ud;
        ud.ptr = sp;
        ud.deleter = [](void* p) { 
            delete static_cast<std::shared_ptr<pyle::Future>*>(p); 
        };
        ud.type_idx = pyle::BindRegistry<std::shared_ptr<pyle::Future>>::type_idx;
        
        pyle::HeapIdx idx = vm.alloc(pyle::Object(ud));
        return pyle::Value(pyle::Value::Tag::NativeObjectRef, idx);
    }

    pyle::Value os_read_file(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() < 1) {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, "os.read_file expects 1 argument (file path).");
            return pyle::Value();
        }

        std::string path = pyle::from_value<std::string>(vm, args[0]);
        if (vm.is_panicked()) {
            return pyle::Value();
        }

        std::ifstream file(path);
        if (!file.is_open()) {
            return pyle::Value(); 
        }

        std::stringstream ss;
        ss << file.rdbuf();
        std::string contents = ss.str();
        pyle::HeapIdx idx = vm.intern_string(contents);
        return pyle::Value(pyle::Value::Tag::StringRef, idx);
    }

    Value os_module_factory(VM& vm) {
        return NativeModule(vm, "os")
            .raw_function("system", os_sys)
            .function<os_time>("time")
            .raw_function("read_file_async", os_read_file_async)
            .raw_function("read_file", os_read_file)
            .build();
    }
    
    
    Value color_module_factory(VM& vm) {
        MapType exports;

        auto add_color = [&](const std::string& name, const std::string& code) {
            Value key(Value::Tag::StringRef, vm.intern_string(name));
            Value val(Value::Tag::StringRef, vm.intern_string(code));
            exports[key] = val;
        };

        // Standard Foreground Colors
        add_color("black",         "\033[30m");
        add_color("red",           "\033[31m");
        add_color("green",         "\033[32m");
        add_color("yellow",        "\033[33m");
        add_color("blue",          "\033[34m");
        add_color("magenta",       "\033[35m");
        add_color("cyan",          "\033[36m");
        add_color("white",         "\033[37m");
        add_color("reset",         "\033[0m");

        // High-Intensity (Bright) Foreground Colors
        add_color("gray",           "\033[90m"); 
        add_color("bright_red",     "\033[91m");
        add_color("bright_green",   "\033[92m");
        add_color("bright_yellow",  "\033[93m");
        add_color("bright_blue",    "\033[94m");
        add_color("bright_magenta", "\033[95m");
        add_color("bright_cyan",    "\033[96m");
        add_color("bright_white",   "\033[97m");

        // Standard Background Colors
        add_color("bg_black",      "\033[40m");
        add_color("bg_red",        "\033[41m");
        add_color("bg_green",      "\033[42m");
        add_color("bg_yellow",     "\033[43m");
        add_color("bg_blue",       "\033[44m");
        add_color("bg_magenta",    "\033[45m");
        add_color("bg_cyan",       "\033[46m");
        add_color("bg_white",      "\033[47m");

        // Bright Background Colors
        add_color("bg_gray",           "\033[100m");
        add_color("bg_bright_red",     "\033[101m");
        add_color("bg_bright_green",   "\033[102m");
        add_color("bg_bright_yellow",  "\033[103m");
        add_color("bg_bright_blue",    "\033[104m");
        add_color("bg_bright_magenta", "\033[105m");
        add_color("bg_bright_cyan",    "\033[106m");
        add_color("bg_bright_white",   "\033[107m");

        // Text Styles
        add_color("bold",          "\033[1m");
        add_color("dim",           "\033[2m");
        add_color("italic",        "\033[3m");
        add_color("underline",     "\033[4m");
        add_color("inverse",       "\033[7m");  
        add_color("strikethrough", "\033[9m");

        HeapIdx map_idx = vm.alloc(Object(std::move(exports)));
        return Value(Value::Tag::MapRef, map_idx);
    }

    void register_core_modules(VM& vm) {
        pyle::register_module(vm, "os", os_module_factory);
        pyle::register_module(vm, "color", color_module_factory);
    }
}