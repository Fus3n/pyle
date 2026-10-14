#include "raylib_binding.hpp"
#include <fstream>

namespace pyle {
namespace raylib_binding {

    namespace {

        bool is_loadable_font(const std::string& path) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) return false;
            std::streamoff size = file.tellg();
            if (size < 12) return false;
            file.seekg(0);
            unsigned char hdr[12] = {0};
            file.read(reinterpret_cast<char*>(hdr), 12);
            if (file.gcount() < 12) return false;

            unsigned int magic = (static_cast<unsigned int>(hdr[0]) << 24) |
                                 (static_cast<unsigned int>(hdr[1]) << 16) |
                                 (static_cast<unsigned int>(hdr[2]) << 8) |
                                  static_cast<unsigned int>(hdr[3]);
            if (magic == 0x74746366u) return true;
            if (magic != 0x00010000u && magic != 0x74727565u &&
                magic != 0x74797031u && magic != 0x4F54544Fu) return false;

            unsigned int num_tables = (static_cast<unsigned int>(hdr[4]) << 8) | hdr[5];
            return static_cast<std::streamoff>(12 + num_tables * 16) <= size;
        }

    }

    void register_fonts(NativeModule& mod) {
        VM& vm = mod.get_vm();
        
        ClassBinder<Font> font(vm, "Font");
        font.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Font>(vm, new Font{});
        });
        font.custom_getter("texture", +[](VM& vm, HeapIdx obj_idx, ArgView) -> Value {
            auto* f = static_cast<Font*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return to_value_owned<Texture2D>(vm, new Texture2D{ f->texture.id, f->texture.width, f->texture.height, f->texture.mipmaps, f->texture.format });
        });
        font.custom_getter("baseSize", +[](VM& vm, HeapIdx obj_idx, ArgView) -> Value {
            auto* f = static_cast<Font*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return to_value(vm, f->baseSize);
        });
        font.custom_getter("glyphCount", +[](VM& vm, HeapIdx obj_idx, ArgView) -> Value {
            auto* f = static_cast<Font*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return to_value(vm, f->glyphCount);
        });
        mod.class_type(font);

        mod.raw_function("IsFontValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsFontValid expects 1 Font."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            if (!f) return Value();
            return Value(IsFontValid(*f));
        });

        mod.raw_function("LoadFont", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadFont expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            if (!is_loadable_font(path)) {
                vm.runtime_error(RuntimeError::Type, "not a valid font file: '" + path + "'");
                return Value();
            }
            Font f = LoadFont(path.c_str());
            return to_value_owned<Font>(vm, new Font{ f.baseSize, f.glyphCount, f.glyphPadding, f.texture, f.recs, f.glyphs });
        });
        
        mod.raw_function("LoadFontEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() < 2 || args.size() > 4) { vm.runtime_error(RuntimeError::ArgumentError, "LoadFontEx expects (path, font_size[, codepoints, codepoint_count])."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            int sz = from_value<int>(vm, args[1]);
            if (!is_loadable_font(path)) {
                vm.runtime_error(RuntimeError::Type, "not a valid font file: '" + path + "'");
                return Value();
            }
            int* codepoints = nullptr;
            int codepoint_count = 0;
            std::vector<int> cp_vec;
            if (args.size() >= 3 && args[2].tag != Value::Tag::None) {
                if (args[2].tag == Value::Tag::ArrayRef) {
                    const auto& arr = vm.get_heap_object<ArrayType>(args[2].as_ref);
                    cp_vec.reserve(arr.size());
                    for (const auto& v : arr) {
                        cp_vec.push_back(static_cast<int>(v.as_int));
                    }
                    codepoints = cp_vec.data();
                    codepoint_count = static_cast<int>(cp_vec.size());
                } else {
                    vm.runtime_error(RuntimeError::Type, "LoadFontEx codepoints must be an array or none.");
                    return Value();
                }
            }
            if (args.size() >= 4) {
                codepoint_count = from_value<int>(vm, args[3]);
            }
            Font f = LoadFontEx(path.c_str(), sz, codepoints, codepoint_count);
            return to_value_owned<Font>(vm, new Font{ f.baseSize, f.glyphCount, f.glyphPadding, f.texture, f.recs, f.glyphs });
        });
        
        mod.raw_function("UnloadFont", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadFont expects 1 Font."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            if (!f) return Value();
            UnloadFont(*f);
            return Value();
        });
        
        mod.raw_function("DrawTextEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextEx expects (font, text, position, size, spacing, color)."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            const std::string& text = from_value<std::string>(vm, args[1]);
            Vector2* pos = as_native<Vector2>(vm, args[2], "Vector2");
            float sz = from_value<float>(vm, args[3]);
            float spacing = from_value<float>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!f || !pos || !c) return Value();
            DrawTextEx(*f, text.c_str(), *pos, sz, spacing, *c);
            return Value();
        });
        
        mod.raw_function("MeasureTextEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "MeasureTextEx expects (font, text, size, spacing)."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            const std::string& text = from_value<std::string>(vm, args[1]);
            float sz = from_value<float>(vm, args[2]);
            float spacing = from_value<float>(vm, args[3]);
            if (!f) return Value();
            Vector2 m = MeasureTextEx(*f, text.c_str(), sz, spacing);
            return to_value_owned<Vector2>(vm, new Vector2{m.x, m.y});
        });

        mod.raw_function("GetFontAscent", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetFontAscent expects 1 Font."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            if (!f) return Value();
            int ascent = 0;
            for (int i = 0; i < f->glyphCount; i++) {
                int oy = -f->glyphs[i].offsetY;
                if (oy > ascent) ascent = oy;
            }
            return to_value(vm, ascent);
        });
        
        mod.raw_function("DrawText", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawText expects (text, posX, posY, fontSize, color)."); return Value(); }
            const std::string& text = from_value<std::string>(vm, args[0]);
            int x = from_value<int64_t>(vm, args[1]);
            int y = from_value<int64_t>(vm, args[2]);
            int sz = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawText(text.c_str(), x, y, sz, *c);
            return Value();
        });

        mod.raw_function("MeasureText", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "MeasureText expects (text, fontSize)."); return Value(); }
            const std::string& text = from_value<std::string>(vm, args[0]);
            int sz = from_value<int64_t>(vm, args[1]);
            return to_value(vm, MeasureText(text.c_str(), sz));
        });

        mod.raw_function("DrawTextPro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 8) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextPro expects (font, text, position, origin, rotation, size, spacing, tint)."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            const std::string& text = from_value<std::string>(vm, args[1]);
            Vector2* pos = as_native<Vector2>(vm, args[2], "Vector2");
            Vector2* origin = as_native<Vector2>(vm, args[3], "Vector2");
            float rot = from_value<float>(vm, args[4]);
            float sz = from_value<float>(vm, args[5]);
            float spacing = from_value<float>(vm, args[6]);
            Color* c = as_native<Color>(vm, args[7], "Color");
            if (!f || !pos || !origin || !c) return Value();
            DrawTextPro(*f, text.c_str(), *pos, *origin, rot, sz, spacing, *c);
            return Value();
        });
    }

}
}
