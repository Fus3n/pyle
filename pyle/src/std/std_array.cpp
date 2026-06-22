#include "pyle/std/std_array.hpp"
#include "pyle/vm.hpp"
#include <algorithm>
#include <fmt/format.h>


namespace pyle::ArrayMethods {

    Value append(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "array.append() takes exactly 1 argument.");
            return Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& vec = std::get<ArrayType>(obj.data);

        vec.push_back(args[0]);

        return Value();
    }

    Value size(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "array.size() takes 0 arguments.");
            return Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& vec = std::get<ArrayType>(obj.data);

        return Value(static_cast<int64_t>(vec.size()));
    }

    Value pop(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "array.pop() takes 0 arguments.");
            return Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& vec = std::get<ArrayType>(obj.data);
        
        if (vec.empty()) {
            vm.runtime_error(RuntimeError::Index, "array.pop() called on empty array.");
            return Value();
        }

        Value val = vec.back();
        vec.pop_back();
        return val;
    }

    Value reverse(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "array.reverse() takes 0 arguments.");
            return Value();
        }

        Object& obj = vm.get_heap_object(obj_idx);
        auto& vec = std::get<ArrayType>(obj.data);

        std::reverse(vec.begin(), vec.end());
        return Value();
    }


    static const ankerl::unordered_dense::map<std::string, MethodFn> methods = {
        {"append", append},
        {"size", size},
        {"pop", pop},
        {"reverse", reverse},
    };

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, fmt::format("array has no method '{}'", name));
        }

        return it->second(vm, obj_idx, args);
    }
}
