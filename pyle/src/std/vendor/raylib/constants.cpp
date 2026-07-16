#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    struct ColorDef { const char* name; Color c; };
    const ColorDef PALETTE[] = {
        {"LIGHTGRAY", { 200, 200, 200, 255 }},
        {"GRAY",      { 130, 130, 130, 255 }},
        {"DARKGRAY",  { 80, 80, 80, 255 }},
        {"YELLOW",    { 253, 249, 0, 255 }},
        {"GOLD",      { 255, 203, 0, 255 }},
        {"ORANGE",    { 255, 161, 0, 255 }},
        {"PINK",      { 255, 109, 194, 255 }},
        {"RED",       { 230, 41, 55, 255 }},
        {"MAROON",    { 190, 33, 55, 255 }},
        {"GREEN",     { 0, 228, 48, 255 }},
        {"LIME",      { 0, 158, 47, 255 }},
        {"DARKGREEN", { 0, 117, 44, 255 }},
        {"SKYBLUE",   { 102, 191, 255, 255 }},
        {"BLUE",      { 0, 121, 241, 255 }},
        {"DARKBLUE",  { 0, 82, 172, 255 }},
        {"PURPLE",    { 200, 122, 255, 255 }},
        {"VIOLET",    { 135, 60, 190, 255 }},
        {"DARKPURPLE",{ 112, 31, 126, 255 }},
        {"BEIGE",     { 211, 176, 131, 255 }},
        {"BROWN",     { 127, 106, 79, 255 }},
        {"DARKBROWN", { 76, 63, 47, 255 }},
        {"WHITE",     { 255, 255, 255, 255 }},
        {"BLACK",     { 0, 0, 0, 255 }},
        {"BLANK",     { 0, 0, 0, 0 }},
        {"MAGENTA",   { 255, 0, 255, 255 }},
        {"RAYWHITE",  { 245, 245, 245, 255 }},
    };

    void register_constants(VM& vm, MapType& exports) {
        for (const auto& def : PALETTE) {
            add_val(vm, exports, def.name,
                    to_value_owned<Color>(vm, new Color{ def.c.r, def.c.g, def.c.b, def.c.a }));
        }

        auto key = [&](const std::string& name, int code) {
            add_val(vm, exports, name, Value(static_cast<int64_t>(code)));
        };
        key("KEY_SPACE", 32); key("KEY_APOSTROPHE", 39); key("KEY_COMMA", 44);
        key("KEY_MINUS", 45); key("KEY_PERIOD", 46); key("KEY_SLASH", 47);
        for (int i = 0; i < 10; ++i) key("KEY_" + std::string(1, char('0' + i)), 48 + i);
        key("KEY_SEMICOLON", 59); key("KEY_EQUAL", 61);
        for (int i = 0; i < 26; ++i) key("KEY_" + std::string(1, char('A' + i)), 65 + i);
        key("KEY_BACKSPACE", 259); key("KEY_TAB", 258); key("KEY_ENTER", 257);
        key("KEY_NULL", 0); key("KEY_ESCAPE", 256); key("KEY_DELETE", 261); key("KEY_INSERT", 260);
        key("KEY_RIGHT", 262); key("KEY_LEFT", 263); key("KEY_DOWN", 264); key("KEY_UP", 265);
        key("KEY_PAGEUP", 266); key("KEY_PAGEDOWN", 267); key("KEY_HOME", 268); key("KEY_END", 269);
        key("MOUSE_LEFT_BUTTON", 0); key("MOUSE_RIGHT_BUTTON", 1);
        key("MOUSE_MIDDLE_BUTTON", 2); key("MOUSE_SIDE_BUTTON", 3); key("MOUSE_EXTRA_BUTTON", 4);

        key("FLAG_VSYNC_HINT", 0x40); key("FLAG_FULLSCREEN_MODE", 2);
        key("FLAG_WINDOW_RESIZABLE", 4); key("FLAG_WINDOW_UNDECORATED", 8);
        key("FLAG_WINDOW_TRANSPARENT", 16); key("FLAG_MSAA_4X_HINT", 32);
        key("FLAG_WINDOW_HIGHDPI", 0x2000); key("FLAG_WINDOW_MAXIMIZED", 0x400);
        key("FLAG_WINDOW_HIDDEN", 0x80); key("FLAG_WINDOW_MINIMIZED", 0x200);
        key("FLAG_WINDOW_UNFOCUSED", 0x800); key("FLAG_WINDOW_TOPMOST", 0x1000);
        key("FLAG_WINDOW_ALWAYS_RUN", 0x100); key("FLAG_WINDOW_MOUSE_PASSTHROUGH", 0x4000);
        key("FLAG_BORDERLESS_WINDOWED_MODE", 0x8000); key("FLAG_INTERLACED_HINT", 0x10000);

        // Additional key codes
        key("KEY_LEFT_BRACKET", 91); key("KEY_BACKSLASH", 92); key("KEY_RIGHT_BRACKET", 93);
        key("KEY_GRAVE", 96);
        key("KEY_CAPS_LOCK", 280); key("KEY_SCROLL_LOCK", 281); key("KEY_NUM_LOCK", 282);
        key("KEY_PRINT_SCREEN", 283); key("KEY_PAUSE", 284);
        for (int i = 0; i < 12; ++i) key("KEY_F" + std::to_string(i + 1), 290 + i);
        key("KEY_LEFT_SHIFT", 340); key("KEY_LEFT_CONTROL", 341); key("KEY_LEFT_ALT", 342); key("KEY_LEFT_SUPER", 343);
        key("KEY_RIGHT_SHIFT", 344); key("KEY_RIGHT_CONTROL", 345); key("KEY_RIGHT_ALT", 346); key("KEY_RIGHT_SUPER", 347);
        key("KEY_KB_MENU", 348);
        for (int i = 0; i < 10; ++i) key("KEY_KP_" + std::to_string(i), 320 + i);
        key("KEY_KP_DECIMAL", 330); key("KEY_KP_DIVIDE", 331); key("KEY_KP_MULTIPLY", 332);
        key("KEY_KP_SUBTRACT", 333); key("KEY_KP_ADD", 334); key("KEY_KP_ENTER", 335); key("KEY_KP_EQUAL", 336);
        key("KEY_BACK", 4); key("KEY_MENU", 5); key("KEY_VOLUME_UP", 24); key("KEY_VOLUME_DOWN", 25);

        // Extra mouse buttons
        key("MOUSE_BUTTON_FORWARD", 5); key("MOUSE_BUTTON_BACK", 6);

        // Mouse cursors
        key("MOUSE_CURSOR_DEFAULT", 0); key("MOUSE_CURSOR_ARROW", 1);
        key("MOUSE_CURSOR_IBEAM", 2); key("MOUSE_CURSOR_CROSSHAIR", 3);
        key("MOUSE_CURSOR_POINTING_HAND", 4); key("MOUSE_CURSOR_RESIZE_EW", 5);
        key("MOUSE_CURSOR_RESIZE_NS", 6); key("MOUSE_CURSOR_RESIZE_NWSE", 7);
        key("MOUSE_CURSOR_RESIZE_NESW", 8); key("MOUSE_CURSOR_RESIZE_ALL", 9);
        key("MOUSE_CURSOR_NOT_ALLOWED", 10);

        // Camera modes and projection
        key("CAMERA_CUSTOM", 0); key("CAMERA_FREE", 1); key("CAMERA_ORBITAL", 2);
        key("CAMERA_FIRST_PERSON", 3); key("CAMERA_THIRD_PERSON", 4);
        key("CAMERA_PERSPECTIVE", 0); key("CAMERA_ORTHOGRAPHIC", 1);

        // Shader uniform types
        key("SHADER_UNIFORM_FLOAT", 0); key("SHADER_UNIFORM_VEC2", 1);
        key("SHADER_UNIFORM_VEC3", 2); key("SHADER_UNIFORM_VEC4", 3);
        key("SHADER_UNIFORM_INT", 4); key("SHADER_UNIFORM_IVEC2", 5);
        key("SHADER_UNIFORM_IVEC3", 6); key("SHADER_UNIFORM_IVEC4", 7);
        key("SHADER_UNIFORM_SAMPLER2D", 8);

        // Texture filter/wrap modes
        key("TEXTURE_FILTER_POINT", 0); key("TEXTURE_FILTER_BILINEAR", 1);
        key("TEXTURE_FILTER_TRILINEAR", 2); key("TEXTURE_FILTER_ANISOTROPIC_4X", 3);
        key("TEXTURE_FILTER_ANISOTROPIC_8X", 4); key("TEXTURE_FILTER_ANISOTROPIC_16X", 5);
        key("TEXTURE_WRAP_REPEAT", 0); key("TEXTURE_WRAP_CLAMP", 1);
        key("TEXTURE_WRAP_MIRROR_REPEAT", 2); key("TEXTURE_WRAP_MIRROR_CLAMP", 3);

        // Blend modes
        key("BLEND_ALPHA", 0); key("BLEND_ADDITIVE", 1); key("BLEND_MULTIPLIED", 2);
        key("BLEND_ADD_COLORS", 3); key("BLEND_SUBTRACT_COLORS", 4);
        key("BLEND_ALPHA_PREMULTIPLY", 5); key("BLEND_CUSTOM", 6); key("BLEND_CUSTOM_SEPARATE", 7);
    }

}
}
