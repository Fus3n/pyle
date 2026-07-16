#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_material(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<Material> mat(vm, "Material");
        mat.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Material>(vm, new Material{});
        });
        mod.class_type(mat);

        mod.raw_function("LoadMaterialDefault", +[](VM& vm, ArgView) -> Value {
            Material m = LoadMaterialDefault();
            return to_value_owned<Material>(vm, new Material(m));
        });
        
        mod.raw_function("IsMaterialValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsMaterialValid expects 1 Material."); return Value(); }
            Material* m = as_native<Material>(vm, args[0], "Material");
            if (!m) return Value();
            return to_value(vm, IsMaterialValid(*m));
        });
        
        mod.raw_function("UnloadMaterial", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadMaterial expects 1 Material."); return Value(); }
            Material* m = as_native<Material>(vm, args[0], "Material");
            if (!m) return Value();
            UnloadMaterial(*m);
            return Value();
        });
        
        mod.raw_function("SetMaterialTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "SetMaterialTexture expects (material, mapType, texture)."); return Value(); }
            Material* m = as_native<Material>(vm, args[0], "Material");
            int map = from_value<int64_t>(vm, args[1]);
            Texture2D* t = as_native<Texture2D>(vm, args[2], "Texture");
            if (!m || !t) return Value();
            SetMaterialTexture(m, map, *t);
            return Value();
        });
    }

}
}
