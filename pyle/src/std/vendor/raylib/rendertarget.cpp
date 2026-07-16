#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_rendertarget(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<RenderTexture2D> rt(vm, "RenderTexture");
        rt.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<RenderTexture2D>(vm, new RenderTexture2D{});
        })
            .member<unsigned int, &RenderTexture2D::id>("id");
        mod.class_type(rt);

        mod.raw_function("LoadRenderTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "LoadRenderTexture expects (width, height)."); return Value(); }
            int w = from_value<int64_t>(vm, args[0]);
            int h = from_value<int64_t>(vm, args[1]);
            RenderTexture2D t = LoadRenderTexture(w, h);
            return to_value_owned<RenderTexture2D>(vm, new RenderTexture2D{ t.id, t.texture, t.depth });
        });
        
        mod.raw_function("UnloadRenderTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadRenderTexture expects 1 RenderTexture."); return Value(); }
            RenderTexture2D* t = as_native<RenderTexture2D>(vm, args[0], "RenderTexture");
            if (!t) return Value();
            UnloadRenderTexture(*t);
            return Value();
        });
        
        mod.raw_function("BeginTextureMode", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginTextureMode expects 1 RenderTexture."); return Value(); }
            RenderTexture2D* t = as_native<RenderTexture2D>(vm, args[0], "RenderTexture");
            if (!t) return Value();
            BeginTextureMode(*t);
            return Value();
        });
        
        mod.function<EndTextureMode>("EndTextureMode");
    }

}
}
