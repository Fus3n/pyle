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
#include <cstring>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
extern char** _environ;
#else
#include <unistd.h>
extern char** environ;
#endif

namespace pyle {

    // OS MODULE
    Value os_time() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        return Value(seconds);
    }

    Value os_epoch() {
        auto now = std::chrono::system_clock::now();
        int64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
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

    pyle::Value os_strptime(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 2 || args[0].tag != Value::Tag::StringRef || args[1].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.strptime expects (date: string, format: string).");
            return Value();
        }
        const std::string& text = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        const std::string& fmt = std::get<std::string>(vm.get_heap_object(args[1].as_ref).data);
        std::tm parts{};
        std::istringstream ss(text);
        ss >> std::get_time(&parts, fmt.c_str());
        if (ss.fail()) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.strptime could not parse the date.");
            return Value();
        }
        std::time_t epoch = std::mktime(&parts);
        if (epoch == static_cast<std::time_t>(-1)) {
            vm.runtime_error(RuntimeError::Runtime, "os.strptime could not convert the date.");
            return Value();
        }
        return Value(static_cast<int64_t>(epoch));
    }

    namespace {
        // Auto-detect order for os.parse_date: unambiguous ISO first, then
        // month-first numerics, then month names. A wrong format rejects on
        // the first mismatched character, so trial costs almost nothing.
        constexpr const char* DATE_FORMATS[] = {
            "%Y-%m-%d %H:%M:%S",
            "%Y-%m-%d",
            "%m/%d/%Y",
            "%m-%d-%Y",
            "%b %d, %Y",
            "%B %d, %Y",
        };

        bool try_parse_date(const std::string& text, const char* fmt, std::tm& out) {
            out = std::tm{};
            out.tm_isdst = -1;
            std::istringstream ss(text);
            ss >> std::get_time(&out, fmt);
            return !ss.fail();
        }

        HeapIdx date_type_idx(VM& vm) {
            static HeapIdx idx = 0;
            static std::unique_ptr<GCRoot> root;
            if (idx == 0) {
                StructType meta;
                meta.name_idx = vm.intern_string("Date");
                for (const char* field : {"year", "month", "day", "hour", "min", "sec", "epoch"}) {
                    meta.field_names.push_back(vm.intern_string(field));
                }
                idx = vm.alloc(Object(std::move(meta)));
                root = std::make_unique<GCRoot>(vm, idx, Value::Tag::StructTypeRef);
            }
            return idx;
        }

        Value date_struct(VM& vm, const std::tm& p, std::time_t epoch) {
            Struct instance;
            instance.type_idx = date_type_idx(vm);
            instance.fields = {
                Value(static_cast<int64_t>(p.tm_year) + 1900),
                Value(static_cast<int64_t>(p.tm_mon) + 1),
                Value(static_cast<int64_t>(p.tm_mday)),
                Value(static_cast<int64_t>(p.tm_hour)),
                Value(static_cast<int64_t>(p.tm_min)),
                Value(static_cast<int64_t>(p.tm_sec)),
                Value(static_cast<int64_t>(epoch)),
            };
            return Value(Value::Tag::StructRef, vm.alloc(Object(std::move(instance))));
        }
    }

    pyle::Value os_parse_date(pyle::VM& vm, pyle::ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.parse_date expects (date: string).");
            return Value();
        }
        const std::string& text = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        for (const char* fmt : DATE_FORMATS) {
            std::tm parts;
            if (!try_parse_date(text, fmt, parts)) continue;
            std::time_t epoch = std::mktime(&parts);
            if (epoch == static_cast<std::time_t>(-1)) continue;
            return date_struct(vm, parts, epoch);
        }
        vm.runtime_error(RuntimeError::ArgumentError, "os.parse_date could not parse the date.");
        return Value();
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

    Value os_pid() {
#ifdef _WIN32
        return Value(static_cast<int64_t>(GetCurrentProcessId()));
#else
        return Value(static_cast<int64_t>(getpid()));
#endif
    }

    Value os_platform(VM& vm) {
#if defined(_WIN32)
        return to_value(vm, std::string("windows"));
#elif defined(__APPLE__)
        return to_value(vm, std::string("macos"));
#elif defined(__linux__)
        return to_value(vm, std::string("linux"));
#else
        return to_value(vm, std::string("unknown"));
#endif
    }

    Value os_arch(VM& vm) {
#if defined(_M_X64) || defined(__x86_64__)
        return to_value(vm, std::string("x86_64"));
#elif defined(_M_ARM64) || defined(__aarch64__)
        return to_value(vm, std::string("aarch64"));
#elif defined(_M_IX86) || defined(__i386__)
        return to_value(vm, std::string("x86"));
#else
        return to_value(vm, std::string("unknown"));
#endif
    }

    Value os_hostname(VM& vm) {
        char buf[256] = {0};
#ifdef _WIN32
        DWORD len = sizeof(buf);
        if (GetComputerNameA(buf, &len)) return to_value(vm, std::string(buf, len));
#else
        if (gethostname(buf, sizeof(buf) - 1) == 0) return to_value(vm, std::string(buf));
#endif
        return Value();
    }

    Value os_username(VM& vm) {
#ifdef _WIN32
        if (const char* u = std::getenv("USERNAME")) return to_value(vm, std::string(u));
#else
        if (const char* u = std::getenv("USER")) return to_value(vm, std::string(u));
#endif
        return Value();
    }

    Value os_environ(VM& vm) {
        MapType env;
        auto add = [&](const char* kv) {
            const char* eq = std::strchr(kv, '=');
            if (!eq || eq == kv) return;
            Value key(Value::Tag::StringRef, vm.intern_string(std::string(kv, eq - kv)));
            Value val(Value::Tag::StringRef, vm.intern_string(std::string(eq + 1)));
            env[key] = val;
        };
#ifdef _WIN32
        for (char** e = _environ; e && *e; ++e) add(*e);
#else
        for (char** e = environ; e && *e; ++e) add(*e);
#endif
        return Value(Value::Tag::MapRef, vm.alloc(Object(std::move(env))));
    }

    Value os_setenv(VM& vm, ArgView args) {
        if (args.size() != 2 || args[0].tag != Value::Tag::StringRef || args[1].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.setenv expects (name: string, value: string).");
            return Value();
        }
        std::string name = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        std::string value = std::get<std::string>(vm.get_heap_object(args[1].as_ref).data);
#ifdef _WIN32
        return Value(_putenv_s(name.c_str(), value.c_str()) == 0);
#else
        return Value(setenv(name.c_str(), value.c_str(), 1) == 0);
#endif
    }

    Value os_unsetenv(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.unsetenv expects (name: string).");
            return Value();
        }
        std::string name = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
#ifdef _WIN32
        return Value(_putenv_s(name.c_str(), "") == 0);
#else
        return Value(unsetenv(name.c_str()) == 0);
#endif
    }

    Value os_cwd(VM& vm) {
        std::error_code ec;
        std::filesystem::path p = std::filesystem::current_path(ec);
        if (ec) return Value();
        return to_value(vm, p.string());
    }

    Value os_chdir(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.chdir expects (path: string).");
            return Value();
        }
        std::string p = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        std::error_code ec;
        std::filesystem::current_path(p, ec);
        return Value(!ec);
    }

    Value os_home(VM& vm) {
#ifdef _WIN32
        if (const char* h = std::getenv("USERPROFILE")) return to_value(vm, std::string(h));
#else
        if (const char* h = std::getenv("HOME")) return to_value(vm, std::string(h));
#endif
        return Value();
    }

    Value os_tempdir(VM& vm) {
        std::error_code ec;
        std::filesystem::path dir = std::filesystem::temp_directory_path(ec);
        if (ec) return Value();
        return to_value(vm, dir.string());
    }

    Value os_tmpname(VM& vm) {
        std::error_code ec;
        std::filesystem::path dir = std::filesystem::temp_directory_path(ec);
        if (ec) dir = ".";
        int64_t seed = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        for (int i = 0; i < 128; ++i) {
            std::filesystem::path candidate = dir / ("pyle_" + std::to_string(seed) + "_" + std::to_string(i));
            if (!std::filesystem::exists(candidate, ec)) {
                std::ofstream(candidate.string()).close();
                return to_value(vm, candidate.string());
            }
        }
        vm.runtime_error(RuntimeError::Runtime, "os.tmpname could not create a temporary file.");
        return Value();
    }

    Value os_clock() {
        return Value(static_cast<double>(std::clock()) / static_cast<double>(CLOCKS_PER_SEC));
    }

    Value os_difftime(VM& vm, ArgView args) {
        if (args.size() != 2 || (args[0].tag != Value::Tag::Int && args[0].tag != Value::Tag::Float) ||
            (args[1].tag != Value::Tag::Int && args[1].tag != Value::Tag::Float)) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.difftime expects (t2: number, t1: number).");
            return Value();
        }
        double t2 = (args[0].tag == Value::Tag::Int) ? static_cast<double>(args[0].as_int) : args[0].as_float;
        double t1 = (args[1].tag == Value::Tag::Int) ? static_cast<double>(args[1].as_int) : args[1].as_float;
        return Value(t2 - t1);
    }

    Value os_timezone() {
        std::time_t now = std::time(nullptr);
        std::tm local_tm{}, utc_tm{};
#if defined(_WIN32)
        localtime_s(&local_tm, &now);
        gmtime_s(&utc_tm, &now);
#else
        localtime_r(&now, &local_tm);
        gmtime_r(&now, &utc_tm);
#endif
        return Value(static_cast<int64_t>(std::difftime(std::mktime(&local_tm), std::mktime(&utc_tm))));
    }

    Value os_localtime(VM& vm, ArgView args) {
        if (args.size() != 1 || (args[0].tag != Value::Tag::Int && args[0].tag != Value::Tag::Float)) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.localtime expects (epoch: number).");
            return Value();
        }
        std::time_t when = (args[0].tag == Value::Tag::Int)
            ? static_cast<std::time_t>(args[0].as_int)
            : static_cast<std::time_t>(args[0].as_float);
        std::tm parts{};
#if defined(_WIN32)
        localtime_s(&parts, &when);
#else
        localtime_r(&when, &parts);
#endif
        return date_struct(vm, parts, when);
    }

    Value os_gmtime(VM& vm, ArgView args) {
        if (args.size() != 1 || (args[0].tag != Value::Tag::Int && args[0].tag != Value::Tag::Float)) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.gmtime expects (epoch: number).");
            return Value();
        }
        std::time_t when = (args[0].tag == Value::Tag::Int)
            ? static_cast<std::time_t>(args[0].as_int)
            : static_cast<std::time_t>(args[0].as_float);
        std::tm parts{};
#if defined(_WIN32)
        gmtime_s(&parts, &when);
#else
        gmtime_r(&when, &parts);
#endif
        return date_struct(vm, parts, when);
    }

    Value os_module_factory(VM& vm) {
        NativeModule mod(vm, "os");
        mod.constant("args", vm.argv);
        mod.raw_function("system", os_sys)
            .function<os_time>("time")
            .function<os_epoch>("epoch")
            .raw_function("getenv", os_getenv)
            .raw_function("date", os_date)
            .raw_function("strptime", os_strptime)
            .raw_function("parse_date", os_parse_date)
            .raw_function("strftime", os_strftime)
            .raw_function("localtime", os_localtime)
            .raw_function("gmtime", os_gmtime)
            .raw_function("sleep", os_sleep)
            .raw_function("sleep_async", os_sleep_async)
            .raw_function("script_path", os_script_path)
            .raw_function("script_dir", os_script_dir)
            .function<os_pid>("pid")
            .function<os_platform>("platform")
            .function<os_arch>("arch")
            .function<os_hostname>("hostname")
            .function<os_username>("username")
            .function<os_environ>("environ")
            .raw_function("setenv", os_setenv)
            .raw_function("unsetenv", os_unsetenv)
            .function<os_cwd>("cwd")
            .raw_function("chdir", os_chdir)
            .function<os_home>("home")
            .function<os_tempdir>("tempdir")
            .function<os_tmpname>("tmpname")
            .function<os_clock>("clock")
            .raw_function("difftime", os_difftime)
            .function<os_timezone>("timezone");
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

    void register_os_module(VM& vm) {
        pyle::register_module(vm, "os", os_module_factory);
    }

    void register_path_module(VM& vm) {
        pyle::register_module(vm, "path", path_module_factory);
    }

    void register_fs_module(VM& vm) {
        pyle::register_module(vm, "fs", fs_module_factory);
    }

    void register_color_module(VM& vm) {
        pyle::register_module(vm, "color", color_module_factory);
    }

    void register_math_module(VM& vm) {
        pyle::register_module(vm, "math", math_module_factory);
    }

    void register_core_modules(VM& vm) {
        register_os_module(vm);
        register_path_module(vm);
        register_fs_module(vm);
        register_color_module(vm);
        register_math_module(vm);
        pyle::register_proc_module(vm);
    }
}