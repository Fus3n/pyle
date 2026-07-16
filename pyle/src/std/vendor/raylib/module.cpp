#include "raylib_binding.hpp"
#include "pyle/config.hpp"

namespace pyle {

    Value register_raylib_module(VM& vm) {
        using namespace raylib_binding;

        // Disable GC during registration: the exports map lives on the C++ stack
        // and is invisible to Pyle's GC. Without this, GC may collect NativeFn
        // objects (and other heap objects) that are stored in the exports map
        // during the many vm.alloc() calls below, corrupting the module.
        bool gc_was_enabled = vm.is_gc_enabled();
        vm.set_gc_enabled(false);

        NativeModule mod(vm, "raylib");
        register_math(mod);
        register_window(mod);
        register_drawing(mod);
        register_input(mod);
        register_textures(mod);
        register_fonts(mod);
        register_constants(mod);
        register_audio(mod);
        register_3d(mod);
        register_shader(mod);
        register_rendertarget(mod);
        register_mesh(mod);
        register_material(mod);
        register_core(mod);
        register_rlgl(mod);

        Value result = mod.build();
        vm.set_gc_enabled(gc_was_enabled);
        return result;
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
