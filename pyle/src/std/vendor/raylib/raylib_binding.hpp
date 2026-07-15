#pragma once
#include "raylib.h"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"

namespace pyle {
namespace raylib_binding {

    // Extract a heap-allocated native object of type T from a Value.
    template <typename T>
    inline T* as_native(VM& vm, const Value& v, const char* type_name) {
        if (v.tag != Value::Tag::NativeObjectRef) {
            vm.runtime_error(RuntimeError::Type, std::string("Expected ") + type_name + " object.");
            return nullptr;
        }
        auto& ud = std::get<NativeObject>(vm.get_heap_object(v.as_ref).data);
        return static_cast<T*>(ud.ptr);
    }

    // Same as above but reads directly from a heap slot index 
    template <typename T>
    inline T* native_at(VM& vm, HeapIdx idx, const char* type_name) {
        Object& o = vm.get_heap_object(idx);
        if (!std::holds_alternative<NativeObject>(o.data)) {
            vm.runtime_error(RuntimeError::Type, std::string("Expected ") + type_name + " object.");
            return nullptr;
        }
        return static_cast<T*>(std::get<NativeObject>(o.data).ptr);
    }

    void add_fn(VM& vm, MapType& exports, const std::string& name, NativeFn fn);
    void add_val(VM& vm, MapType& exports, const std::string& name, Value v);
    void add_class(VM& vm, MapType& exports, const std::string& name, Value type_val);

    Value ctor_texture(VM& vm, ArgView args);

    void register_math(VM& vm, MapType& exports);
    void register_window(VM& vm, MapType& exports);
    void register_drawing(VM& vm, MapType& exports);
    void register_input(VM& vm, MapType& exports);
    void register_textures(VM& vm, MapType& exports);
    void register_fonts(VM& vm, MapType& exports);
    void register_constants(VM& vm, MapType& exports);
    void register_audio(VM& vm, MapType& exports);

}
}
