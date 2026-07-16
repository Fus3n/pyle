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

    void register_constants(NativeModule& mod) {
        for (const auto& def : PALETTE) {
            mod.native_object<Color>(def.name, new Color{ def.c.r, def.c.g, def.c.b, def.c.a });
        }

        mod.add_constants({
            {"KEY_SPACE", 32}, {"KEY_APOSTROPHE", 39}, {"KEY_COMMA", 44},
            {"KEY_MINUS", 45}, {"KEY_PERIOD", 46}, {"KEY_SLASH", 47},
            {"KEY_0", 48}, {"KEY_1", 49}, {"KEY_2", 50}, {"KEY_3", 51},
            {"KEY_4", 52}, {"KEY_5", 53}, {"KEY_6", 54}, {"KEY_7", 55},
            {"KEY_8", 56}, {"KEY_9", 57},
            {"KEY_SEMICOLON", 59}, {"KEY_EQUAL", 61},
            {"KEY_A", 65}, {"KEY_B", 66}, {"KEY_C", 67}, {"KEY_D", 68},
            {"KEY_E", 69}, {"KEY_F", 70}, {"KEY_G", 71}, {"KEY_H", 72},
            {"KEY_I", 73}, {"KEY_J", 74}, {"KEY_K", 75}, {"KEY_L", 76},
            {"KEY_M", 77}, {"KEY_N", 78}, {"KEY_O", 79}, {"KEY_P", 80},
            {"KEY_Q", 81}, {"KEY_R", 82}, {"KEY_S", 83}, {"KEY_T", 84},
            {"KEY_U", 85}, {"KEY_V", 86}, {"KEY_W", 87}, {"KEY_X", 88},
            {"KEY_Y", 89}, {"KEY_Z", 90},
            {"KEY_BACKSPACE", 259}, {"KEY_TAB", 258}, {"KEY_ENTER", 257},
            {"KEY_NULL", 0}, {"KEY_ESCAPE", 256}, {"KEY_DELETE", 261}, {"KEY_INSERT", 260},
            {"KEY_RIGHT", 262}, {"KEY_LEFT", 263}, {"KEY_DOWN", 264}, {"KEY_UP", 265},
            {"KEY_PAGEUP", 266}, {"KEY_PAGEDOWN", 267}, {"KEY_HOME", 268}, {"KEY_END", 269},
            {"MOUSE_LEFT_BUTTON", 0}, {"MOUSE_RIGHT_BUTTON", 1},
            {"MOUSE_MIDDLE_BUTTON", 2}, {"MOUSE_SIDE_BUTTON", 3}, {"MOUSE_EXTRA_BUTTON", 4},

            {"FLAG_VSYNC_HINT", 0x40}, {"FLAG_FULLSCREEN_MODE", 2},
            {"FLAG_WINDOW_RESIZABLE", 4}, {"FLAG_WINDOW_UNDECORATED", 8},
            {"FLAG_WINDOW_TRANSPARENT", 16}, {"FLAG_MSAA_4X_HINT", 32},
            {"FLAG_WINDOW_HIGHDPI", 0x2000}, {"FLAG_WINDOW_MAXIMIZED", 0x400},
            {"FLAG_WINDOW_HIDDEN", 0x80}, {"FLAG_WINDOW_MINIMIZED", 0x200},
            {"FLAG_WINDOW_UNFOCUSED", 0x800}, {"FLAG_WINDOW_TOPMOST", 0x1000},
            {"FLAG_WINDOW_ALWAYS_RUN", 0x100}, {"FLAG_WINDOW_MOUSE_PASSTHROUGH", 0x4000},
            {"FLAG_BORDERLESS_WINDOWED_MODE", 0x8000}, {"FLAG_INTERLACED_HINT", 0x10000},

            {"KEY_LEFT_BRACKET", 91}, {"KEY_BACKSLASH", 92}, {"KEY_RIGHT_BRACKET", 93},
            {"KEY_GRAVE", 96},
            {"KEY_CAPS_LOCK", 280}, {"KEY_SCROLL_LOCK", 281}, {"KEY_NUM_LOCK", 282},
            {"KEY_PRINT_SCREEN", 283}, {"KEY_PAUSE", 284},
            {"KEY_F1", 290}, {"KEY_F2", 291}, {"KEY_F3", 292}, {"KEY_F4", 293},
            {"KEY_F5", 294}, {"KEY_F6", 295}, {"KEY_F7", 296}, {"KEY_F8", 297},
            {"KEY_F9", 298}, {"KEY_F10", 299}, {"KEY_F11", 300}, {"KEY_F12", 301},
            {"KEY_LEFT_SHIFT", 340}, {"KEY_LEFT_CONTROL", 341}, {"KEY_LEFT_ALT", 342}, {"KEY_LEFT_SUPER", 343},
            {"KEY_RIGHT_SHIFT", 344}, {"KEY_RIGHT_CONTROL", 345}, {"KEY_RIGHT_ALT", 346}, {"KEY_RIGHT_SUPER", 347},
            {"KEY_KB_MENU", 348},
            {"KEY_KP_0", 320}, {"KEY_KP_1", 321}, {"KEY_KP_2", 322}, {"KEY_KP_3", 323},
            {"KEY_KP_4", 324}, {"KEY_KP_5", 325}, {"KEY_KP_6", 326}, {"KEY_KP_7", 327},
            {"KEY_KP_8", 328}, {"KEY_KP_9", 329},
            {"KEY_KP_DECIMAL", 330}, {"KEY_KP_DIVIDE", 331}, {"KEY_KP_MULTIPLY", 332},
            {"KEY_KP_SUBTRACT", 333}, {"KEY_KP_ADD", 334}, {"KEY_KP_ENTER", 335}, {"KEY_KP_EQUAL", 336},
            {"KEY_BACK", 4}, {"KEY_MENU", 5}, {"KEY_VOLUME_UP", 24}, {"KEY_VOLUME_DOWN", 25},

            {"MOUSE_BUTTON_FORWARD", 5}, {"MOUSE_BUTTON_BACK", 6},

            {"MOUSE_CURSOR_DEFAULT", 0}, {"MOUSE_CURSOR_ARROW", 1},
            {"MOUSE_CURSOR_IBEAM", 2}, {"MOUSE_CURSOR_CROSSHAIR", 3},
            {"MOUSE_CURSOR_POINTING_HAND", 4}, {"MOUSE_CURSOR_RESIZE_EW", 5},
            {"MOUSE_CURSOR_RESIZE_NS", 6}, {"MOUSE_CURSOR_RESIZE_NWSE", 7},
            {"MOUSE_CURSOR_RESIZE_NESW", 8}, {"MOUSE_CURSOR_RESIZE_ALL", 9},
            {"MOUSE_CURSOR_NOT_ALLOWED", 10},

            {"CAMERA_CUSTOM", 0}, {"CAMERA_FREE", 1}, {"CAMERA_ORBITAL", 2},
            {"CAMERA_FIRST_PERSON", 3}, {"CAMERA_THIRD_PERSON", 4},
            {"CAMERA_PERSPECTIVE", 0}, {"CAMERA_ORTHOGRAPHIC", 1},

            {"SHADER_UNIFORM_FLOAT", 0}, {"SHADER_UNIFORM_VEC2", 1},
            {"SHADER_UNIFORM_VEC3", 2}, {"SHADER_UNIFORM_VEC4", 3},
            {"SHADER_UNIFORM_INT", 4}, {"SHADER_UNIFORM_IVEC2", 5},
            {"SHADER_UNIFORM_IVEC3", 6}, {"SHADER_UNIFORM_IVEC4", 7},
            {"SHADER_UNIFORM_SAMPLER2D", 8},

            {"TEXTURE_FILTER_POINT", 0}, {"TEXTURE_FILTER_BILINEAR", 1},
            {"TEXTURE_FILTER_TRILINEAR", 2}, {"TEXTURE_FILTER_ANISOTROPIC_4X", 3},
            {"TEXTURE_FILTER_ANISOTROPIC_8X", 4}, {"TEXTURE_FILTER_ANISOTROPIC_16X", 5},
            {"TEXTURE_WRAP_REPEAT", 0}, {"TEXTURE_WRAP_CLAMP", 1},
            {"TEXTURE_WRAP_MIRROR_REPEAT", 2}, {"TEXTURE_WRAP_MIRROR_CLAMP", 3},

            {"BLEND_ALPHA", 0}, {"BLEND_ADDITIVE", 1}, {"BLEND_MULTIPLIED", 2},
            {"BLEND_ADD_COLORS", 3}, {"BLEND_SUBTRACT_COLORS", 4},
            {"BLEND_ALPHA_PREMULTIPLY", 5}, {"BLEND_CUSTOM", 6}, {"BLEND_CUSTOM_SEPARATE", 7},
        });
    }

}
}
