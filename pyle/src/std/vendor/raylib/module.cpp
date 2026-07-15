#include "raylib_binding.hpp"
#include "pyle/config.hpp"

namespace pyle {

    Value register_raylib_module(VM& vm) {
        using namespace raylib_binding;

        // Disable GC while assembling the module: several NativeFn/struct objects
        // live only in the local exports map until the module map is built.
        bool was_gc = vm.is_gc_enabled();
        vm.set_gc_enabled(false);

        MapType exports;
        register_math(vm, exports);    
        register_window(vm, exports);
        register_drawing(vm, exports);
        register_input(vm, exports);
        register_textures(vm, exports);
        register_fonts(vm, exports);
        register_constants(vm, exports);
        register_audio(vm, exports);

        HeapIdx map_idx = vm.alloc(Object(std::move(exports)));
        vm.get_heap_object<MapObject>(map_idx).is_module = true;

        vm.set_gc_enabled(was_gc);
        return Value(Value::Tag::MapRef, map_idx);
    }

}

#if defined(_WIN32)
    #define PYLE_MODULE_EXPORT __declspec(dllexport)
#else
    #define PYLE_MODULE_EXPORT __attribute__((visibility("default")))
#endif

extern "C" {

PYLE_MODULE_EXPORT int64_t pyle_module_version = PYLE_MODULE_ABI_VERSION;

PYLE_MODULE_EXPORT int64_t pyle_module_init(void* vm_ptr) {
    pyle::VM& vm = *static_cast<pyle::VM*>(vm_ptr);
    pyle::Value val = pyle::register_raylib_module(vm);
    return val.as_ref;
}

}
