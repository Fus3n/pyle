#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void add_fn(VM& vm, MapType& exports, const std::string& name, NativeFn fn) {
        HeapIdx idx = vm.alloc(Object(fn));
        Value key(Value::Tag::StringRef, vm.intern_string(name));
        exports[key] = Value(Value::Tag::NativeFuncRef, idx);
    }

    void add_val(VM& vm, MapType& exports, const std::string& name, Value v) {
        Value key(Value::Tag::StringRef, vm.intern_string(name));
        exports[key] = v;
    }

    void add_class(VM& vm, MapType& exports, const std::string& name, Value type_val) {
        Value key(Value::Tag::StringRef, vm.intern_string(name));
        exports[key] = type_val;
    }

}
}
