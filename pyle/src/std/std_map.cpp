#include "pyle/std/std_map.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"
#include <fmt/format.h>

namespace pyle::MapMethods {

    Value size(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.size() takes 0 arguments.");
            return Value();
        }

        auto& map = vm.get_heap_object<MapType>(obj_idx);

        return Value(static_cast<int64_t>(map.size()));
    }

    Value remove(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.remove() takes exactly 1 argument.");
            return Value();
        }

        Value key = args[0];
        if (!vm.is_hashable(key)) {
            vm.runtime_error(RuntimeError::Type, fmt::format("Unhashable type '{}' cannot be used as a map key.", key.tag_to_string()));
            return Value();
        }

        auto& map = vm.get_heap_object<MapType>(obj_idx);

        auto it = map.find(key);
        if (it != map.end()) {
            Value val = it->second;
            map.erase(it);
            return val;
        }

        return Value(); 
    }

    Value keys(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.keys() takes 0 arguments.");
            return Value();
        }

        auto& map = vm.get_heap_object<MapType>(obj_idx);

        ArrayType key_array;
        key_array.reserve(map.size());
        for (const auto& [k, v] : map) {
            key_array.push_back(k);
        }

        HeapIdx array_idx = vm.alloc(Object(std::move(key_array)));
        return Value(Value::Tag::ArrayRef, array_idx);
    }

    Value values(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.values() takes 0 arguments.");
            return Value();
        }

        auto& map = vm.get_heap_object<MapType>(obj_idx);

        ArrayType key_array;
        key_array.reserve(map.size());
        for (const auto& [k, v] : map) {
            key_array.push_back(v);
        }

        HeapIdx array_idx = vm.alloc(Object(std::move(key_array)));
        return Value(Value::Tag::ArrayRef, array_idx);
    }

    Value has(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.has() expects exactly 1 argument (key).");
            return Value();
        }
        
        Value key = args[0];
        if (!vm.is_hashable(key)) {
            return Value(false); 
        }

        const auto& map = vm.get_heap_object<MapType>(obj_idx);
        return Value(map.find(key) != map.end());
    }

    Value clear(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "map.clear() expects 0 arguments.");
            return Value();
        }

        auto& map = vm.get_heap_object<MapType>(obj_idx);
        map.clear();
        return Value();
    }

    static NativeMethodMap methods = {
        {"size", size},
        {"remove", remove},
        {"keys", keys},
        {"values", values},
        {"has", has},  
        {"clear", clear}   
    };

    bool has_method(const std::string& name) {
        return methods.find(name) != methods.end();
    }

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, fmt::format("map has no native method '{}'", name));
            return Value();
        }

        return it->second(vm, obj_idx, args);
    }
}
