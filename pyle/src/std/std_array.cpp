#include "pyle/std/std_array.hpp"
#include "pyle/value.hpp"
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
            vm.runtime_error(RuntimeError::ArgumentError, "array.slice() expects either 2 integers (start, end) or 1 range.");
            return Value();
        }

        const auto& arr = vm.get_heap_object<ArrayType>(obj_idx);

        if (start < 0) start = 0;
        if (end > static_cast<int64_t>(arr.size())) end = arr.size();

        ArrayType sub_arr;
        if (start < end) {
            sub_arr.assign(arr.begin() + start, arr.begin() + end);
        }

        HeapIdx arr_idx = vm.alloc(Object(std::move(sub_arr)));
        return Value(Value::Tag::ArrayRef, arr_idx);
    }

    Value clear(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "array.clear() expects 0 arguments.");
            return Value();
        }

        auto& arr = vm.get_heap_object<ArrayType>(obj_idx);
        arr.clear();
        return Value();
    }

    static NativeMethodMap methods = {
        {"append", append},
        {"size", size},
        {"pop", pop},
        {"reverse", reverse},
        {"slice", slice},
        {"clear", clear},
    };

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, fmt::format("array has no method '{}'", name));
        }

        return it->second(vm, obj_idx, args);
    }
}
