#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    Value native_LoadTexture(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadTexture expects 1 string."); return Value(); }
        const std::string& path = from_value<std::string>(vm, args[0]);
        Texture2D t = LoadTexture(path.c_str());
        return to_value_owned<Texture2D>(vm, new Texture2D{ t.id, t.width, t.height, t.mipmaps, t.format });
    }

    Value native_DrawTexture(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTexture expects (texture, x, y, tint)."); return Value(); }
        Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
        int x = from_value<int64_t>(vm, args[1]);
        int y = from_value<int64_t>(vm, args[2]);
        Color* c = as_native<Color>(vm, args[3], "Color");
        if (!t || !c) return Value();
        DrawTexture(*t, x, y, *c);
        return Value();
    }

    Value native_DrawTextureEx(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureEx expects (texture, position, rotation, scale, tint)."); return Value(); }
        Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
        Vector2* pos = as_native<Vector2>(vm, args[1], "Vector2");
        float rot = from_value<float>(vm, args[2]);
        float scale = from_value<float>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!t || !pos || !c) return Value();
        DrawTextureEx(*t, *pos, rot, scale, *c);
        return Value();
    }

    Value native_UnloadTexture(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadTexture expects 1 Texture."); return Value(); }
        Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
        if (!t) return Value();
        UnloadTexture(*t);
        return Value();
    }

    void register_textures(VM& vm, MapType& exports) {
        ClassBinder<Texture2D> tex(vm, "Texture");
        tex.custom_constructor(ctor_texture)
            .member<int, &Texture2D::width>("width")
            .member<int, &Texture2D::height>("height");
        add_class(vm, exports, "Texture", tex.get_constructor());

        add_fn(vm, exports, "LoadTexture", native_LoadTexture);
        add_fn(vm, exports, "DrawTexture", native_DrawTexture);
        add_fn(vm, exports, "DrawTextureEx", native_DrawTextureEx);
        add_fn(vm, exports, "UnloadTexture", native_UnloadTexture);
    }

}
}
