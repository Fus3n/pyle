#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    namespace {
        void w_InitWindow(VM&, int w, int h, std::string t) { InitWindow(w, h, t.c_str()); }
        bool w_WindowShouldClose(VM&) { return WindowShouldClose(); }
        void w_CloseWindow(VM&) { CloseWindow(); }
        void w_SetTargetFPS(VM&, int fps) { SetTargetFPS(fps); }
        float w_GetFrameTime(VM&) { return GetFrameTime(); }
        double w_GetTime(VM&) { return GetTime(); }
        int w_GetRandomValue(VM&, int min, int max) { return GetRandomValue(min, max); }
        void w_SetConfigFlags(VM&, int flags) { SetConfigFlags((unsigned int)flags); }
        void w_SetExitKey(VM&, int key) { SetExitKey(key); }
    }

    namespace {
        int w_GetScreenWidth(VM&) { return GetScreenWidth(); }
        int w_GetScreenHeight(VM&) { return GetScreenHeight(); }
    }

    void register_window(VM& vm, MapType& exports) {
        add_fn(vm, exports, "InitWindow", pyle::FreeFnDeducer<w_InitWindow>::wrap);
        add_fn(vm, exports, "WindowShouldClose", pyle::FreeFnDeducer<w_WindowShouldClose>::wrap);
        add_fn(vm, exports, "CloseWindow", pyle::FreeFnDeducer<w_CloseWindow>::wrap);
        add_fn(vm, exports, "SetTargetFPS", pyle::FreeFnDeducer<w_SetTargetFPS>::wrap);
        add_fn(vm, exports, "GetFrameTime", pyle::FreeFnDeducer<w_GetFrameTime>::wrap);
        add_fn(vm, exports, "GetTime", pyle::FreeFnDeducer<w_GetTime>::wrap);
        add_fn(vm, exports, "GetRandomValue", pyle::FreeFnDeducer<w_GetRandomValue>::wrap);
        add_fn(vm, exports, "SetConfigFlags", pyle::FreeFnDeducer<w_SetConfigFlags>::wrap);
        add_fn(vm, exports, "GetScreenWidth", pyle::FreeFnDeducer<w_GetScreenWidth>::wrap);
        add_fn(vm, exports, "GetScreenHeight", pyle::FreeFnDeducer<w_GetScreenHeight>::wrap);
        add_fn(vm, exports, "SetExitKey", pyle::FreeFnDeducer<w_SetExitKey>::wrap);
    }

}
}
