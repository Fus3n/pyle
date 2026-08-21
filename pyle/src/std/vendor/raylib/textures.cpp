#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_textures(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<Image> img(vm, "Image");
        img.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Image>(vm, new Image{});
        })
           .member<int, &Image::width>("width")
           .member<int, &Image::height>("height")
           .member<int, &Image::mipmaps>("mipmaps")
           .member<int, &Image::format>("format");
        mod.class_type(img);

        ClassBinder<Texture2D> tex(vm, "Texture");
        tex.custom_constructor(ctor_texture)
           .member<unsigned int, &Texture2D::id>("id")
           .member<int, &Texture2D::width>("width")
           .member<int, &Texture2D::height>("height")
           .member<int, &Texture2D::mipmaps>("mipmaps")
           .member<int, &Texture2D::format>("format");
        mod.class_type(tex);

        mod.raw_function("LoadImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadImage expects (fileName)."); return Value(); }
            const std::string& str = from_value<std::string>(vm, args[0]);
            Image img = LoadImage(str.c_str());
            return to_value_owned<Image>(vm, new Image{ img.data, img.width, img.height, img.mipmaps, img.format });
        });
        mod.raw_function("UnloadImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadImage expects 1 Image."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            if (!img) return Value();
            UnloadImage(*img);
            return Value();
        });
        mod.raw_function("LoadTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadTexture expects (fileName)."); return Value(); }
            const std::string& str = from_value<std::string>(vm, args[0]);
            Texture2D tex = LoadTexture(str.c_str());
            return to_value_owned<Texture2D>(vm, new Texture2D{ tex.id, tex.width, tex.height, tex.mipmaps, tex.format });
        });
        mod.raw_function("LoadTextureFromImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadTextureFromImage expects 1 Image."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            if (!img) return Value();
            Texture2D tex = LoadTextureFromImage(*img);
            return to_value_owned<Texture2D>(vm, new Texture2D{ tex.id, tex.width, tex.height, tex.mipmaps, tex.format });
        });
        mod.raw_function("UnloadTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadTexture expects 1 Texture."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            if (!t) return Value();
            UnloadTexture(*t);
            return Value();
        });
        mod.raw_function("SetTextureFilter", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetTextureFilter expects (texture, filter)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            int filter = from_value<int64_t>(vm, args[1]);
            if (!t) return Value();
            SetTextureFilter(*t, filter);
            return Value();
        });
        mod.raw_function("DrawTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTexture expects (texture, posX, posY, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            int x = from_value<int64_t>(vm, args[1]);
            int y = from_value<int64_t>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!t || !c) return Value();
            DrawTexture(*t, x, y, *c);
            return Value();
        });
        mod.raw_function("DrawTextureV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureV expects (texture, position, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Vector2* p = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!t || !p || !c) return Value();
            DrawTextureV(*t, *p, *c);
            return Value();
        });
        mod.raw_function("DrawTextureEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureEx expects (texture, position, rotation, scale, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Vector2* p = as_native<Vector2>(vm, args[1], "Vector2");
            float rot = from_value<float>(vm, args[2]);
            float scale = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!t || !p || !c) return Value();
            DrawTextureEx(*t, *p, rot, scale, *c);
            return Value();
        });
        mod.raw_function("DrawTextureRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextureRec expects (texture, source, position, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Rectangle* src = as_native<Rectangle>(vm, args[1], "Rectangle");
            Vector2* p = as_native<Vector2>(vm, args[2], "Vector2");
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!t || !src || !p || !c) return Value();
            DrawTextureRec(*t, *src, *p, *c);
            return Value();
        });
        mod.raw_function("DrawTexturePro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTexturePro expects (texture, source, dest, origin, rotation, tint)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Rectangle* src = as_native<Rectangle>(vm, args[1], "Rectangle");
            Rectangle* dst = as_native<Rectangle>(vm, args[2], "Rectangle");
            Vector2* orig = as_native<Vector2>(vm, args[3], "Vector2");
            float rot = from_value<float>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!t || !src || !dst || !orig || !c) return Value();
            DrawTexturePro(*t, *src, *dst, *orig, rot, *c);
            return Value();
        });

        // ---------------------------------------------------------------- Image manipulation
        mod.raw_function("GenImageChecked", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "GenImageChecked expects (width, height, checksX, checksY, col1, col2)."); return Value(); }
            int w = from_value<int64_t>(vm, args[0]);
            int h = from_value<int64_t>(vm, args[1]);
            int cx = from_value<int64_t>(vm, args[2]);
            int cy = from_value<int64_t>(vm, args[3]);
            Color* c1 = as_native<Color>(vm, args[4], "Color");
            Color* c2 = as_native<Color>(vm, args[5], "Color");
            if (!c1 || !c2) return Value();
            Image img = GenImageChecked(w, h, cx, cy, *c1, *c2);
            return to_value_owned<Image>(vm, new Image{ img.data, img.width, img.height, img.mipmaps, img.format });
        });

        mod.raw_function("ImageCrop", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ImageCrop expects (image, crop)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            Rectangle* r = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!img || !r) return Value();
            ImageCrop(img, *r);
            return Value();
        });

        mod.raw_function("ImageResize", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "ImageResize expects (image, newWidth, newHeight)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            int nw = from_value<int64_t>(vm, args[1]);
            int nh = from_value<int64_t>(vm, args[2]);
            if (!img) return Value();
            ImageResize(img, nw, nh);
            return Value();
        });

        mod.raw_function("ImageResizeNN", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "ImageResizeNN expects (image, newWidth, newHeight)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            int nw = from_value<int64_t>(vm, args[1]);
            int nh = from_value<int64_t>(vm, args[2]);
            if (!img) return Value();
            ImageResizeNN(img, nw, nh);
            return Value();
        });

        mod.raw_function("ImageResizeCanvas", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "ImageResizeCanvas expects (image, newWidth, newHeight, offsetX, offsetY, fill)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            int nw = from_value<int64_t>(vm, args[1]);
            int nh = from_value<int64_t>(vm, args[2]);
            int ox = from_value<int64_t>(vm, args[3]);
            int oy = from_value<int64_t>(vm, args[4]);
            Color* fill = as_native<Color>(vm, args[5], "Color");
            if (!img || !fill) return Value();
            ImageResizeCanvas(img, nw, nh, ox, oy, *fill);
            return Value();
        });

        mod.raw_function("ImageFormat", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ImageFormat expects (image, newFormat)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            int fmt = from_value<int64_t>(vm, args[1]);
            if (!img) return Value();
            ImageFormat(img, fmt);
            return Value();
        });

        mod.raw_function("ImageDraw", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "ImageDraw expects (dst, src, srcRec, dstRec, tint)."); return Value(); }
            Image* dst = as_native<Image>(vm, args[0], "Image");
            Image* src = as_native<Image>(vm, args[1], "Image");
            Rectangle* srcRec = as_native<Rectangle>(vm, args[2], "Rectangle");
            Rectangle* dstRec = as_native<Rectangle>(vm, args[3], "Rectangle");
            Color* tint = as_native<Color>(vm, args[4], "Color");
            if (!dst || !src || !srcRec || !dstRec || !tint) return Value();
            ImageDraw(dst, *src, *srcRec, *dstRec, *tint);
            return Value();
        });

        mod.raw_function("ImageFlipHorizontal", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ImageFlipHorizontal expects (image)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            if (!img) return Value();
            ImageFlipHorizontal(img);
            return Value();
        });

        mod.raw_function("ImageFlipVertical", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ImageFlipVertical expects (image)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            if (!img) return Value();
            ImageFlipVertical(img);
            return Value();
        });

        mod.raw_function("ExportImage", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ExportImage expects (image, fileName)."); return Value(); }
            Image* img = as_native<Image>(vm, args[0], "Image");
            const std::string& fn = from_value<std::string>(vm, args[1]);
            if (!img) return Value();
            return to_value(vm, ExportImage(*img, fn.c_str()));
        });
    }

}
}
