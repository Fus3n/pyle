#pragma once
#include "raylib.h"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include "pyle/vm.hpp"

namespace pyle {
namespace raylib_binding {

    template <typename T>
    inline T* as_native(VM& vm, const Value& v, const char* type_name) {
        if (v.tag != Value::Tag::NativeObjectRef) {
            vm.runtime_error(RuntimeError::Type, std::string("Expected ") + type_name + " object.");
            return nullptr;
        }
        auto& ud = std::get<NativeObject>(vm.get_heap_object(v.as_ref).data);
        return static_cast<T*>(ud.ptr);
    }

    template <typename T>
    inline T* native_at(VM& vm, HeapIdx idx, const char* type_name) {
        Object& o = vm.get_heap_object(idx);
        if (!std::holds_alternative<NativeObject>(o.data)) {
            vm.runtime_error(RuntimeError::Type, std::string("Expected ") + type_name + " object.");
            return nullptr;
        }
        return static_cast<T*>(std::get<NativeObject>(o.data).ptr);
    }

    Value ctor_texture(VM& vm, ArgView args);

    void register_math(NativeModule& mod);
    void register_window(NativeModule& mod);
    void register_drawing(NativeModule& mod);
    void register_input(NativeModule& mod);
    void register_textures(NativeModule& mod);
    void register_fonts(NativeModule& mod);
    void register_constants(NativeModule& mod);
    void register_audio(NativeModule& mod);
    void register_3d(NativeModule& mod);
    void register_shader(NativeModule& mod);
    void register_rendertarget(NativeModule& mod);
    void register_mesh(NativeModule& mod);
    void register_material(NativeModule& mod);
    void register_core(NativeModule& mod);
    void register_rlgl(NativeModule& mod);
}
}
