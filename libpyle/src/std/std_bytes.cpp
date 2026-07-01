#include "pyle/std/std_bytes.hpp"

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

    static NativeMethodMap methods = {
        {"size", size},
        {"to_string", to_string}
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