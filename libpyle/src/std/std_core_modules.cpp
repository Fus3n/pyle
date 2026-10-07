#include "pyle/std/std_core_modules.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <chrono>
#include "pyle/std/std_future.hpp"
#include <thread>
#include <filesystem>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <algorithm>

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

    bool os_file_exists(std::string file_path) {
        return std::filesystem::exists(file_path);
    }

    pyle::Value os_remove(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.remove expects 1 string arugment");
            return Value();
        }

        std::string file_path = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);

        std::error_code ec;
        bool result = std::filesystem::remove(file_path, ec);
        return Value(result && !ec);
    }

    pyle::Value os_sleep(pyle::VM& vm, pyle::ArgView args) {
        int64_t ms = pyle::from_value<int64_t>(vm, args[0]);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        return pyle::Value();
    }

    pyle::Value os_sleep_async(pyle::VM& vm, pyle::ArgView args) {
        int64_t ms = pyle::from_value<int64_t>(vm, args[0]);
        auto [val, future] = pyle::Future::create(vm);
        
        std::thread([future, ms]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            future->finished.store(true);
        }).detach();
        
        return val;
    }

    Value os_script_path(VM& vm, ArgView args) {
        (void)args;
        return Value(Value::Tag::StringRef, vm.intern_string(std::string(vm.script_name)));
    }

    Value os_script_dir(VM& vm, ArgView args) {
        (void)args;
        std::filesystem::path p(vm.script_name);
        return Value(Value::Tag::StringRef, vm.intern_string(p.parent_path().string()));
    }

    pyle::Value os_mkdir(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.mkdir expects 1 string argument.");
            return Value();
        }

        std::string dir = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);

        std::error_code ec;
        bool created = std::filesystem::create_directories(dir, ec);
        return Value((created || std::filesystem::is_directory(dir, ec)) && !ec);
    }

    pyle::Value os_listdir(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.listdir expects 1 string argument.");
            return Value();
        }

        std::string dir = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);

        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec) || ec) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.listdir path is not a readable directory.");
            return Value();
        }

        ArrayType names;
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (ec) break;
            names.push_back(Value(Value::Tag::StringRef,
                vm.intern_string(entry.path().filename().string())));
        }
        if (ec) {
            vm.runtime_error(RuntimeError::Runtime, "os.listdir failed while reading directory.");
            return Value();
        }
        std::sort(names.begin(), names.end(), [&vm](const Value& a, const Value& b) {
            return std::get<std::string>(vm.get_heap_object(a.as_ref).data) <
                   std::get<std::string>(vm.get_heap_object(b.as_ref).data);
        });

        HeapIdx arr_idx = vm.alloc(Object(std::move(names)));
        return Value(Value::Tag::ArrayRef, arr_idx);
    }

    pyle::Value os_rename(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 2 || args[0].tag != Value::Tag::StringRef || args[1].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.rename expects (src: string, dst: string).");
            return Value();
        }

        std::string src = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        std::string dst = std::get<std::string>(vm.get_heap_object(args[1].as_ref).data);

        std::error_code ec;
        std::filesystem::rename(src, dst, ec);
        return Value(!ec);
    }

    pyle::Value os_getenv(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.getenv expects 1 string argument.");
            return Value();
        }

        std::string name = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        const char* val = std::getenv(name.c_str());
        if (!val) {
            return Value();
        }
        return Value(Value::Tag::StringRef, vm.intern_string(val));
    }

    pyle::Value os_date(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.date() takes 0 arguments.");
            return Value();
        }

        std::time_t now = std::time(nullptr);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &now);
#else
        localtime_r(&now, &local);
#endif
        std::ostringstream ss;
        ss << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
        return Value(Value::Tag::StringRef, vm.intern_string(ss.str()));
    }

    pyle::Value os_strftime(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() < 1 || args.size() > 2 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.strftime expects (format: string, timestamp = now).");
            return Value();
        }
        if (args.size() == 2 && args[1].tag != Value::Tag::Int && args[1].tag != Value::Tag::Float) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.strftime timestamp expects a number.");
            return Value();
        }

        std::string fmt = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        std::time_t when = std::time(nullptr);
        if (args.size() == 2) {
            when = (args[1].tag == Value::Tag::Int)
                ? static_cast<std::time_t>(args[1].as_int)
                : static_cast<std::time_t>(args[1].as_float);
        }
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &when);
#else
        localtime_r(&when, &local);
#endif
        char buf[256];
        if (std::strftime(buf, sizeof(buf), fmt.c_str(), &local) == 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.strftime format produced no output.");
            return Value();
        }
        return Value(Value::Tag::StringRef, vm.intern_string(buf));
    }

    Value os_module_factory(VM& vm) {
        NativeModule mod(vm, "os");
        mod.raw_function("system", os_sys)
            .function<os_time>("time")
            .function<os_file_exists>("file_exists")
            .raw_function("remove", os_remove)
            .raw_function("mkdir", os_mkdir)
            .raw_function("listdir", os_listdir)
            .raw_function("rename", os_rename)
            .raw_function("getenv", os_getenv)
            .raw_function("date", os_date)
            .raw_function("strftime", os_strftime)
            .raw_function("sleep", os_sleep)
            .raw_function("sleep_async", os_sleep_async)
            .raw_function("script_path", os_script_path)
            .raw_function("script_dir", os_script_dir);
        proc::bind_to_os(vm, mod);
        return mod.build();
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
        pyle::register_module(vm, "math", math_module_factory);
    }
}