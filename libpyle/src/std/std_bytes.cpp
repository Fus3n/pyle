#include "pyle/std/std_bytes.hpp"
#include <cstring>

namespace pyle::BytesMethods {

    Value size(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        return Value(static_cast<int64_t>(bytes.size()));
    }

    Value to_string(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        std::string str(bytes.begin(), bytes.end());
        return Value(Value::Tag::StringRef, vm.intern_string(str));
    }

    static uint64_t load_le(const BytesType& bytes) {
        uint64_t v = 0;
        for (size_t i = 0; i < 8; ++i) {
            v |= static_cast<uint64_t>(bytes[i]) << (8 * i);
        }
        return v;
    }

    static uint64_t load_be(const BytesType& bytes) {
        uint64_t v = 0;
        for (size_t i = 0; i < 8; ++i) {
            v = (v << 8) | static_cast<uint64_t>(bytes[i]);
        }
        return v;
    }

    static bool require_exact_8(VM& vm, const BytesType& bytes, const char* name) {
        if (bytes.size() != 8) {
            vm.runtime_error(RuntimeError::ArgumentError,
                fmt::format("{} expects exactly 8 bytes, got {}.", name, bytes.size()));
            return false;
        }
        return true;
    }

    Value to_int_le(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        if (!require_exact_8(vm, bytes, "to_int_le")) return Value();
        return Value(static_cast<int64_t>(load_le(bytes)));
    }

    Value to_int_be(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        if (!require_exact_8(vm, bytes, "to_int_be")) return Value();
        return Value(static_cast<int64_t>(load_be(bytes)));
    }

    Value to_float_le(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        if (!require_exact_8(vm, bytes, "to_float_le")) return Value();
        uint64_t bits = load_le(bytes);
        double d = 0.0;
        std::memcpy(&d, &bits, sizeof(d));
        return Value(d);
    }

    Value to_float_be(VM& vm, HeapIdx obj_idx, ArgView args) {
        const auto& bytes = vm.get_heap_object<BytesType>(obj_idx);
        if (!require_exact_8(vm, bytes, "to_float_be")) return Value();
        uint64_t bits = load_be(bytes);
        double d = 0.0;
        std::memcpy(&d, &bits, sizeof(d));
        return Value(d);
    }

    static NativeMethodMap methods = {
        {"size", size},
        {"to_string", to_string},
        {"to_int_le", to_int_le},
        {"to_int_be", to_int_be},
        {"to_float_le", to_float_le},
        {"to_float_be", to_float_be}
    };

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        auto it = methods.find(name);
        if (it == methods.end()) {
            vm.runtime_error(RuntimeError::Name, "bytes object has no method '" + name + "'.");
            return Value();
        }
        return it->second(vm, obj_idx, args);
    }
}
