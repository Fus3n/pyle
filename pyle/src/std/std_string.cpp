#include "pyle/std/std_string.hpp"

#include "pyle/vm.hpp"

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

    static const ankerl::unordered_dense::map<std::string, MethodFn> methods = {
        {"size", size}
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
