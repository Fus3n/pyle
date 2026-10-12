#include "pyle/std/std_core_modules.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <filesystem>
#include <string>

namespace pyle {

    namespace {
        namespace fs = std::filesystem;

        bool path_arg(VM& vm, ArgView args, std::string& out, const char* who) {
            if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError, std::string(who) + " expects (path: string).");
                return false;
            }
            out = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
            return true;
        }
    }

    Value path_join(VM& vm, ArgView args) {
        if (args.size() == 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "path.join expects at least one string.");
            return Value();
        }
        fs::path result;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i].tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::ArgumentError, "path.join expects string arguments.");
                return Value();
            }
            result /= std::get<std::string>(vm.get_heap_object(args[i].as_ref).data);
        }
        return to_value(vm, result.string());
    }

    Value path_basename(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.basename")) return Value();
        return to_value(vm, fs::path(p).filename().string());
    }

    Value path_dirname(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.dirname")) return Value();
        return to_value(vm, fs::path(p).parent_path().string());
    }

    Value path_extension(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.extension")) return Value();
        return to_value(vm, fs::path(p).extension().string());
    }

    Value path_stem(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.stem")) return Value();
        return to_value(vm, fs::path(p).stem().string());
    }

    Value path_normalize(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.normalize")) return Value();
        return to_value(vm, fs::path(p).lexically_normal().string());
    }

    Value path_absolute(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.absolute")) return Value();
        std::error_code ec;
        fs::path abs = fs::absolute(p, ec);
        if (ec) {
            vm.runtime_error(RuntimeError::Runtime, "path.absolute failed: " + ec.message());
            return Value();
        }
        return to_value(vm, abs.lexically_normal().string());
    }

    Value path_is_absolute(VM& vm, ArgView args) {
        std::string p;
        if (!path_arg(vm, args, p, "path.is_absolute")) return Value();
        return Value(fs::path(p).is_absolute());
    }

    Value path_module_factory(VM& vm) {
        NativeModule mod(vm, "path");
        mod.raw_function("join", path_join)
           .raw_function("basename", path_basename)
           .raw_function("dirname", path_dirname)
           .raw_function("extension", path_extension)
           .raw_function("stem", path_stem)
           .raw_function("normalize", path_normalize)
           .raw_function("absolute", path_absolute)
           .raw_function("is_absolute", path_is_absolute)
           .constant("sep", std::string(1, static_cast<char>(fs::path::preferred_separator)));
        return mod.build();
    }

}
