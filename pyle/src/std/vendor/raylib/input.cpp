#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    namespace {
        bool w_IsKeyDown(VM&, int key) { return IsKeyDown(key); }
        bool w_IsKeyPressed(VM&, int key) { return IsKeyPressed(key); }
        bool w_IsKeyReleased(VM&, int key) { return IsKeyReleased(key); }
        bool w_IsKeyUp(VM&, int key) { return IsKeyUp(key); }
        bool w_IsKeyPressedRepeat(VM&, int key) { return IsKeyPressedRepeat(key); }
        int  w_GetKeyPressed(VM&) { return GetKeyPressed(); }
        int  w_GetCharPressed(VM&) { return GetCharPressed(); }
        int  w_GetMouseX(VM&) { return GetMouseX(); }
        int  w_GetMouseY(VM&) { return GetMouseY(); }
        bool w_IsMouseButtonPressed(VM&, int b) { return IsMouseButtonPressed(b); }
        bool w_IsMouseButtonDown(VM&, int b) { return IsMouseButtonDown(b); }
        bool w_IsMouseButtonReleased(VM&, int b) { return IsMouseButtonReleased(b); }
        bool w_IsMouseButtonUp(VM&, int b) { return IsMouseButtonUp(b); }
        float w_GetMouseWheelMove(VM&) { return GetMouseWheelMove(); }
        void w_SetMouseCursor(VM&, int c) { SetMouseCursor(c); }
        void w_ShowCursor(VM&) { ShowCursor(); }
        void w_HideCursor(VM&) { HideCursor(); }
        void w_EnableCursor(VM&) { EnableCursor(); }
        void w_DisableCursor(VM&) { DisableCursor(); }
        bool w_IsCursorHidden(VM&) { return IsCursorHidden(); }
    }

    void register_input(NativeModule& mod) {
        mod.function<w_IsKeyDown>("IsKeyDown")
           .function<w_IsKeyPressed>("IsKeyPressed")
           .function<w_IsKeyReleased>("IsKeyReleased")
           .function<w_IsKeyUp>("IsKeyUp")
           .function<w_IsKeyPressedRepeat>("IsKeyPressedRepeat")
           .function<w_GetKeyPressed>("GetKeyPressed")
           .function<w_GetCharPressed>("GetCharPressed")
           .function<w_GetMouseX>("GetMouseX")
           .function<w_GetMouseY>("GetMouseY")
           .function<w_IsMouseButtonPressed>("IsMouseButtonPressed")
           .function<w_IsMouseButtonDown>("IsMouseButtonDown")
           .function<w_IsMouseButtonReleased>("IsMouseButtonReleased")
           .function<w_IsMouseButtonUp>("IsMouseButtonUp")
           .function<w_GetMouseWheelMove>("GetMouseWheelMove")
           .function<w_SetMouseCursor>("SetMouseCursor")
           .function<w_ShowCursor>("ShowCursor")
           .function<w_HideCursor>("HideCursor")
           .function<w_EnableCursor>("EnableCursor")
           .function<w_DisableCursor>("DisableCursor")
           .function<w_IsCursorHidden>("IsCursorHidden")
           .raw_function("GetMousePosition", +[](VM& vm, ArgView) -> Value {
               Vector2 v = GetMousePosition();
               return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
           })
           .raw_function("GetMouseDelta", +[](VM& vm, ArgView) -> Value {
               Vector2 v = GetMouseDelta();
               return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
           })
           .raw_function("GetMouseWheelMoveV", +[](VM& vm, ArgView) -> Value {
               Vector2 v = GetMouseWheelMoveV();
               return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
           });
    }

}
}
