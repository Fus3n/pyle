#include "pyle/std/std_bytes_builder.hpp"

namespace pyle::BytesBuilderMethods {

    Value push(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::Int) {
            vm.runtime_error(RuntimeError::ArgumentError, "BytesBuilder.push expects 1 int argument.");
            return Value();
        }
        int64_t byte = args[0].as_int;
        if (byte < 0 || byte > 255) {
            vm.runtime_error(RuntimeError::ArgumentError, "Byte values must be between 0 and 255.");
            return Value();
        }
        auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        builder.push_back(static_cast<uint8_t>(byte));
        return Value(static_cast<int64_t>(builder.size()));
    }

    Value extend_array(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::ArrayRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "BytesBuilder.extend_array expects 1 array argument.");
            return Value();
        }
        const auto& arr = std::get<ArrayType>(vm.get_heap_object(args[0].as_ref).data);
        auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        builder.reserve(builder.size() + arr.size());
        for (const auto& val : arr) {
            if (val.tag != Value::Tag::Int) {
                vm.runtime_error(RuntimeError::Type, "BytesBuilder.extend_array expects an array of integers.");
                return Value();
            }
            int64_t byte = val.as_int;
            if (byte < 0 || byte > 255) {
                vm.runtime_error(RuntimeError::ArgumentError, "Byte values must be between 0 and 255.");
                return Value();
            }
            builder.push_back(static_cast<uint8_t>(byte));
        }
        return Value(static_cast<int64_t>(builder.size()));
    }

    Value reserve(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::Int) {
            vm.runtime_error(RuntimeError::ArgumentError, "BytesBuilder.reserve expects 1 int argument.");
            return Value();
        }
        int64_t capacity = args[0].as_int;
        if (capacity < 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "Capacity must be non-negative.");
            return Value();
        }
        auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        builder.reserve(static_cast<size_t>(capacity));
        return Value(static_cast<int64_t>(builder.capacity()));
    }

    Value size(VM& vm, HeapIdx obj_idx, ArgView) {
        const auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        return Value(static_cast<int64_t>(builder.size()));
    }

    Value to_string(VM& vm, HeapIdx obj_idx, ArgView) {
        const auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        std::string str(builder.begin(), builder.end());
        return Value(Value::Tag::StringRef, vm.intern_string(str));
    }

    Value to_bytes(VM& vm, HeapIdx obj_idx, ArgView) {
        auto& builder = vm.get_heap_object<BytesType>(obj_idx);
        BytesType bytes;
        bytes.swap(builder);
        HeapIdx idx = vm.alloc(Object(std::move(bytes)));
        return Value(Value::Tag::BytesRef, idx);
    }

    static NativeMethodMap methods = {
        {"push", push},
        {"extend_array", extend_array},
        {"reserve", reserve},
        {"size", size},
        {"to_bytes", to_bytes},
        {"to_string", to_string}
    };

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, "bytes_builder object has no method '" + name + "'.");
            return Value();
        }
        return it->second(vm, obj_idx, args);
    }
}
