#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    Value ctor_font(VM& vm, ArgView) {
        return to_value_owned<Font>(vm, new Font{});
    }

    Value native_LoadFont(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadFont expects 1 string."); return Value(); }
        const std::string& path = from_value<std::string>(vm, args[0]);
        Font f = LoadFont(path.c_str());
        Font* copy = new Font{};
        *copy = f;
        return to_value_owned<Font>(vm, copy);
    }

    Value native_LoadFontEx(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "LoadFontEx expects (path, font_size)."); return Value(); }
        const std::string& path = from_value<std::string>(vm, args[0]);
        int sz = from_value<int>(vm, args[1]);
        Font f = LoadFontEx(path.c_str(), sz, nullptr, 0);
        Font* copy = new Font{};
        *copy = f;
        return to_value_owned<Font>(vm, copy);
    }

    Value native_UnloadFont(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadFont expects 1 Font."); return Value(); }
        Font* f = as_native<Font>(vm, args[0], "Font");
        if (!f) return Value();
        UnloadFont(*f);
        return Value();
    }

    Value native_DrawTextEx(VM& vm, ArgView args) {
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
    }

    Value native_MeasureTextEx(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "MeasureTextEx expects (font, text, size, spacing)."); return Value(); }
        Font* f = as_native<Font>(vm, args[0], "Font");
        const std::string& text = from_value<std::string>(vm, args[1]);
        float sz = from_value<float>(vm, args[2]);
        float spacing = from_value<float>(vm, args[3]);
        if (!f) return Value();
        Vector2 m = MeasureTextEx(*f, text.c_str(), sz, spacing);
        return to_value_owned<Vector2>(vm, new Vector2{m.x, m.y});
    }

    void register_fonts(VM& vm, MapType& exports) {
        ClassBinder<Font> font(vm, "Font");
        font.custom_constructor(ctor_font);
        add_class(vm, exports, "Font", font.get_constructor());

        add_fn(vm, exports, "LoadFont", native_LoadFont);
        add_fn(vm, exports, "LoadFontEx", native_LoadFontEx);
        add_fn(vm, exports, "UnloadFont", native_UnloadFont);
        add_fn(vm, exports, "DrawTextEx", native_DrawTextEx);
        add_fn(vm, exports, "MeasureTextEx", native_MeasureTextEx);
    }

}
}
