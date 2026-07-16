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

    void register_window(NativeModule& mod) {
        mod.function<w_InitWindow>("InitWindow")
           .function<w_WindowShouldClose>("WindowShouldClose")
           .function<w_CloseWindow>("CloseWindow")
           .function<w_SetTargetFPS>("SetTargetFPS")
           .function<w_GetFrameTime>("GetFrameTime")
           .function<w_GetTime>("GetTime")
           .function<w_GetRandomValue>("GetRandomValue")
           .function<w_SetConfigFlags>("SetConfigFlags")
           .function<w_SetExitKey>("SetExitKey")
           .function<w_GetScreenWidth>("GetScreenWidth")
           .function<w_GetScreenHeight>("GetScreenHeight")
           .function<w_GetFPS>("GetFPS")
           .function<w_IsWindowReady>("IsWindowReady")
           .function<w_IsWindowFullscreen>("IsWindowFullscreen")
           .function<w_IsWindowMinimized>("IsWindowMinimized")
           .function<w_IsWindowMaximized>("IsWindowMaximized")
           .function<w_IsWindowFocused>("IsWindowFocused")
           .function<w_IsWindowResized>("IsWindowResized")
           .function<w_ToggleFullscreen>("ToggleFullscreen")
           .function<w_MaximizeWindow>("MaximizeWindow")
           .function<w_MinimizeWindow>("MinimizeWindow")
           .function<w_RestoreWindow>("RestoreWindow")
           .function<w_GetRenderWidth>("GetRenderWidth")
           .function<w_GetRenderHeight>("GetRenderHeight");
    }

}
}
