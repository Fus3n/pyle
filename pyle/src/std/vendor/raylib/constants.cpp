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

        key("FLAG_VSYNC_HINT", 64); key("FLAG_FULLSCREEN_MODE", 2);
        key("FLAG_WINDOW_RESIZABLE", 4); key("FLAG_WINDOW_UNDECORATED", 8);
        key("FLAG_WINDOW_TRANSPARENT", 16); key("FLAG_MSAA_4X_HINT", 32);
        key("FLAG_WINDOW_HIGHDPI", 128); key("FLAG_WINDOW_MAXIMIZED", 512);
    }

}
}
