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
            .member<int, &Texture2D::height>("height")
            .member<unsigned int, &Texture2D::id>("id");
        add_class(vm, exports, "Texture", tex.get_constructor());

        ClassBinder<Image> img(vm, "Image");
        img.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Image>(vm, new Image{});
        })
            .member<int, &Image::width>("width")
            .member<int, &Image::height>("height");
        add_class(vm, exports, "Image", img.get_constructor());

        add_fn(vm, exports, "LoadTexture", native_LoadTexture);
        add_fn(vm, exports, "LoadTextureFromImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadTextureFromImage expects 1 Image."); return Value(); }
            Image* i = as_native<Image>(vm, args[0], "Image");
            if (!i) return Value();
            Texture2D t = LoadTextureFromImage(*i);
            return to_value_owned<Texture2D>(vm, new Texture2D{ t.id, t.width, t.height, t.mipmaps, t.format });
        });
        add_fn(vm, exports, "IsTextureValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsTextureValid expects 1 Texture."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            if (!t) return Value();
            return to_value(vm, IsTextureValid(*t));
        });
        add_fn(vm, exports, "UnloadTexture", native_UnloadTexture);
        add_fn(vm, exports, "DrawTexture", native_DrawTexture);
        add_fn(vm, exports, "DrawTextureV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureV expects (texture, position, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Vector2* p = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!t || !p || !c) return Value();
            DrawTextureV(*t, *p, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawTextureEx", native_DrawTextureEx);
        add_fn(vm, exports, "DrawTextureRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureRec expects (texture, source, position, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Rectangle* s = as_native<Rectangle>(vm, args[1], "Rectangle");
            Vector2* p = as_native<Vector2>(vm, args[2], "Vector2");
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!t || !s || !p || !c) return Value();
            DrawTextureRec(*t, *s, *p, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawTexturePro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTexturePro expects (texture, source, dest, origin, rotation, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Rectangle* s = as_native<Rectangle>(vm, args[1], "Rectangle");
            Rectangle* d = as_native<Rectangle>(vm, args[2], "Rectangle");
            Vector2* o = as_native<Vector2>(vm, args[3], "Vector2");
            float rot = from_value<float>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!t || !s || !d || !o || !c) return Value();
            DrawTexturePro(*t, *s, *d, *o, rot, *c);
            return Value();
        });
        add_fn(vm, exports, "SetTextureFilter", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetTextureFilter expects (texture, filter)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            int f = from_value<int64_t>(vm, args[1]);
            if (!t) return Value();
            SetTextureFilter(*t, f);
            return Value();
        });
        add_fn(vm, exports, "SetTextureWrap", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetTextureWrap expects (texture, wrap)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            int w = from_value<int64_t>(vm, args[1]);
            if (!t) return Value();
            SetTextureWrap(*t, w);
            return Value();
        });
        add_fn(vm, exports, "LoadImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadImage expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            Image im = LoadImage(path.c_str());
            return to_value_owned<Image>(vm, new Image{ im.data, im.width, im.height, im.mipmaps, im.format });
        });
        add_fn(vm, exports, "UnloadImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadImage expects 1 Image."); return Value(); }
            Image* i = as_native<Image>(vm, args[0], "Image");
            if (!i) return Value();
            UnloadImage(*i);
            return Value();
        });
        add_fn(vm, exports, "GenImageColor", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "GenImageColor expects (width, height, color)."); return Value(); }
            int w = from_value<int64_t>(vm, args[0]);
            int h = from_value<int64_t>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!c) return Value();
            Image im = GenImageColor(w, h, *c);
            return to_value_owned<Image>(vm, new Image{ im.data, im.width, im.height, im.mipmaps, im.format });
        });
        add_fn(vm, exports, "GenImageChecked", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "GenImageChecked expects (w, h, checksX, checksY, col1, col2)."); return Value(); }
            int w = from_value<int64_t>(vm, args[0]);
            int h = from_value<int64_t>(vm, args[1]);
            int cx = from_value<int64_t>(vm, args[2]);
            int cy = from_value<int64_t>(vm, args[3]);
            Color* c1 = as_native<Color>(vm, args[4], "Color");
            Color* c2 = as_native<Color>(vm, args[5], "Color");
            if (!c1 || !c2) return Value();
            Image im = GenImageChecked(w, h, cx, cy, *c1, *c2);
            return to_value_owned<Image>(vm, new Image{ im.data, im.width, im.height, im.mipmaps, im.format });
        });
        add_fn(vm, exports, "ExportImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ExportImage expects (image, fileName)."); return Value(); }
            Image* i = as_native<Image>(vm, args[0], "Image");
            const std::string& name = from_value<std::string>(vm, args[1]);
            if (!i) return Value();
            return to_value(vm, ExportImage(*i, name.c_str()));
        });
        add_fn(vm, exports, "ImageCopy", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ImageCopy expects 1 Image."); return Value(); }
            Image* src = as_native<Image>(vm, args[0], "Image");
            if (!src) return Value();
            Image im = ImageCopy(*src);
            return to_value_owned<Image>(vm, new Image{ im.data, im.width, im.height, im.mipmaps, im.format });
        });
        add_fn(vm, exports, "ImageCrop", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ImageCrop expects (image, cropRect)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            Rectangle* r = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!img || !r) return Value();
            ImageCrop(img, *r);
            return Value();
        });
        add_fn(vm, exports, "GenTextureMipmaps", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GenTextureMipmaps expects 1 Texture."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            if (!t) return Value();
            GenTextureMipmaps(t);
            return Value();
        });
        add_fn(vm, exports, "UpdateTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "UpdateTexture expects (texture, pixels)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            if (!t) return Value();
            if (args[1].tag != Value::Tag::BytesRef) {
                vm.runtime_error(RuntimeError::Type, "Expected bytes for pixels.");
                return Value();
            }
            const auto& pixels = std::get<BytesType>(vm.get_heap_object(args[1].as_ref).data);
            UpdateTexture(*t, pixels.data());
            return Value();
        });
    }

}
}
