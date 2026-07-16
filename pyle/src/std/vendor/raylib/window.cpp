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
        int w_GetFPS(VM&) { return GetFPS(); }
        bool w_IsWindowReady(VM&) { return IsWindowReady(); }
        bool w_IsWindowFullscreen(VM&) { return IsWindowFullscreen(); }
        bool w_IsWindowMinimized(VM&) { return IsWindowMinimized(); }
        bool w_IsWindowMaximized(VM&) { return IsWindowMaximized(); }
        bool w_IsWindowFocused(VM&) { return IsWindowFocused(); }
        bool w_IsWindowResized(VM&) { return IsWindowResized(); }
        void w_ToggleFullscreen(VM&) { ToggleFullscreen(); }
        void w_MaximizeWindow(VM&) { MaximizeWindow(); }
        void w_MinimizeWindow(VM&) { MinimizeWindow(); }
        void w_RestoreWindow(VM&) { RestoreWindow(); }
        int w_GetRenderWidth(VM&) { return GetRenderWidth(); }
        int w_GetRenderHeight(VM&) { return GetRenderHeight(); }
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

        add_fn(vm, exports, "GetFPS", pyle::FreeFnDeducer<w_GetFPS>::wrap);
        add_fn(vm, exports, "IsWindowReady", pyle::FreeFnDeducer<w_IsWindowReady>::wrap);
        add_fn(vm, exports, "IsWindowFullscreen", pyle::FreeFnDeducer<w_IsWindowFullscreen>::wrap);
        add_fn(vm, exports, "IsWindowMinimized", pyle::FreeFnDeducer<w_IsWindowMinimized>::wrap);
        add_fn(vm, exports, "IsWindowMaximized", pyle::FreeFnDeducer<w_IsWindowMaximized>::wrap);
        add_fn(vm, exports, "IsWindowFocused", pyle::FreeFnDeducer<w_IsWindowFocused>::wrap);
        add_fn(vm, exports, "IsWindowResized", pyle::FreeFnDeducer<w_IsWindowResized>::wrap);
        add_fn(vm, exports, "ToggleFullscreen", pyle::FreeFnDeducer<w_ToggleFullscreen>::wrap);
        add_fn(vm, exports, "MaximizeWindow", pyle::FreeFnDeducer<w_MaximizeWindow>::wrap);
        add_fn(vm, exports, "MinimizeWindow", pyle::FreeFnDeducer<w_MinimizeWindow>::wrap);
        add_fn(vm, exports, "RestoreWindow", pyle::FreeFnDeducer<w_RestoreWindow>::wrap);
        add_fn(vm, exports, "GetRenderWidth", pyle::FreeFnDeducer<w_GetRenderWidth>::wrap);
        add_fn(vm, exports, "GetRenderHeight", pyle::FreeFnDeducer<w_GetRenderHeight>::wrap);
    }

}
}
