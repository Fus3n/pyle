#include "pyle/std/std_string.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"
#include <sstream>

namespace  pyle::StringMethods {
    Value size(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.size() takes 0 arguments.");
            return Value();
        }

        auto& str = vm.get_heap_object<std::string>(obj_idx);
        return Value(static_cast<int64_t>(str.size()));
    }

    pyle::Value to_num(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(pyle::RuntimeError::ArgumentError, "string.to_num() takes 0 arguments.");
            return pyle::Value();
        }

        auto& str = vm.get_heap_object<std::string>(obj_idx);

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
        int64_t start, end;

        if (args.size() == 1 && args[0].tag == Value::Tag::RangeRef) {
            const auto& range = vm.get_heap_object<Range>(args[0].as_ref);
            start = range.start;
            end = range.end;
        } else if (args.size() == 2 && args[0].tag == Value::Tag::Int && args[1].tag == Value::Tag::Int) {
            start = args[0].as_int;
            end = args[1].as_int;
        } else {
            vm.runtime_error(RuntimeError::ArgumentError, "string.slice() expects either 2 integers (start, end) or 1 range.");
            return Value();
        }

        const auto& str = vm.get_heap_object<std::string>(obj_idx);

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

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
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

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
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

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
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

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
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

        const auto& delim = vm.get_heap_object<std::string>(obj_idx);
        const auto& arr = vm.get_heap_object<ArrayType>(args[0].as_ref);

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

    Value split(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.split() expects exactly 1 string argument (delimiter).");
            return Value();
        }

        std::string str = vm.get_heap_object<std::string>(obj_idx);
        std::string delim = vm.get_heap_object<std::string>(args[0].as_ref);

        // turning off gc as intern might allocate
        bool was_enabled = vm.is_gc_enabled();
        vm.set_gc_enabled(false);

        ArrayType parts;
        if (delim.empty()) {
            for (char c : str)
                parts.push_back(Value(Value::Tag::StringRef, vm.intern_string(std::string(1, c))));
        } else {
            size_t start = 0, end = str.find(delim);
            while (end != std::string::npos) {
                parts.push_back(Value(Value::Tag::StringRef, vm.intern_string(str.substr(start, end - start))));
                start = end + delim.size();
                end = str.find(delim, start);
            }
            parts.push_back(Value(Value::Tag::StringRef, vm.intern_string(str.substr(start))));
        }

        vm.set_gc_enabled(was_enabled);
        HeapIdx arr_idx = vm.alloc(Object(std::move(parts)));
        return Value(Value::Tag::ArrayRef, arr_idx);
    }

    Value lower(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.lower() expects 0 arguments.");
            return Value();
        }

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
        std::string result = str;
        for (char& c : result) {
            c = std::tolower(static_cast<unsigned char>(c));
        }
        return Value(Value::Tag::StringRef, vm.intern_string(result));
    }

    Value upper(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "string.upper() expects 0 arguments.");
            return Value();
        }

        const auto& str = vm.get_heap_object<std::string>(obj_idx);
        std::string result = str;
        for (char& c : result) {
            c = std::toupper(static_cast<unsigned char>(c));
        }
        return Value(Value::Tag::StringRef, vm.intern_string(result));
    }

    static NativeMethodMap methods = {
        {"size", size},
        {"to_num", to_num},
        {"slice", slice},
        {"is_digit", is_digit},
        {"is_alpha", is_alpha},
        {"is_alnum", is_alnum},
        {"is_space", is_space},
        {"join", join},
        {"lower", lower},
        {"upper", upper},
        {"split", split}
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
