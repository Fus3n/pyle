#include "pyle/std/std_string.hpp"
#include "pyle/vm.hpp"
#include <sstream>

namespace  pyle::StringMethods {
    Value size(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.size() takes 0 arguments.");
            return Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& str = std::get<std::string>(obj.data);

        return Value(static_cast<int64_t>(str.size()));
    }

    pyle::Value to_num(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, "string.to_num() takes 0 arguments.");
            return pyle::Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& str = std::get<std::string>(obj.data);

        try {
            if (str.find('.') != std::string::npos) {
                return pyle::Value(std::stod(str));
            } else {
                return pyle::Value(static_cast<int64_t>(std::stoll(str)));
            }
        } catch (...) {
            return pyle::Value(); 
        }
    }

    Value slice(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 2 || args[0].tag != Value::Tag::Int || args[1].tag != Value::Tag::Int) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.slice expects 2 integer arguments (start, end).");
            return Value();
        }

        const auto& str = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        int64_t start = args[0].as_int;
        int64_t end = args[1].as_int;

        if (start < 0) start = 0;
        if (end > static_cast<int64_t>(str.size())) end = str.size();
        if (start >= end) {
            return Value(Value::Tag::StringRef, vm.intern_string(""));
        }

        std::string sub = str.substr(start, end - start);
        return Value(Value::Tag::StringRef, vm.intern_string(sub));
    }

    Value is_digit(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.is_digit() expects 0 arguments.");
            return Value();
        }

        const auto& str = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        if (str.empty()) return Value(false);
        
        for (char c : str) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return Value(false);
            }
        }
        return Value(true);
    }

    Value is_alpha(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.is_alpha() expects 0 arguments.");
            return Value();
        }

        const auto& str = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        if (str.empty()) return Value(false);
        
        for (char c : str) {
            if (!std::isalpha(static_cast<unsigned char>(c))) {
                return Value(false);
            }
        }
        return Value(true);
    }

    Value is_alnum(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.is_alnum() expects 0 arguments.");
            return Value();
        }

        const auto& str = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        if (str.empty()) return Value(false);
        
        for (char c : str) {
            if (!std::isalnum(static_cast<unsigned char>(c))) {
                return Value(false);
            }
        }
        return Value(true);
    }

    Value is_space(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.is_space() expects 0 arguments.");
            return Value();
        }

        const auto& str = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        if (str.empty()) return Value(false);
        
        for (char c : str) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                return Value(false);
            }
        }
        return Value(true);
    }

    Value join(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::ArrayRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.join() expects exactly 1 array argument.");
            return Value();
        }

        const auto& delim = std::get<std::string>(vm.get_heap_object(obj_idx).data);
        const auto& arr = std::get<ArrayType>(vm.get_heap_object(args[0].as_ref).data);

        if (arr.empty()) {
            return Value(Value::Tag::StringRef, vm.intern_string(""));
        }

        std::stringstream ss;
        for (size_t i = 0; i < arr.size(); ++i) {
            ss << vm.value_to_string(arr[i]);
            if (i < arr.size() - 1) {
                ss << delim;
            }
        }
        HeapIdx idx = vm.intern_string(ss.str());
        return Value(Value::Tag::StringRef, idx);
    }

    static const ankerl::unordered_dense::map<std::string, NativeMethodFn> methods = {
        {"size", size},
        {"to_num", to_num},
        {"slice", slice},
        {"is_digit", is_digit},
        {"is_alpha", is_alpha},
        {"is_alnum", is_alnum},
        {"is_space", is_space},
        {"join", join}
    };

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        const auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, "string has no method '" + name + "'.");
            return Value();
        }

        return it->second(vm, obj_idx, args);
    }
}
