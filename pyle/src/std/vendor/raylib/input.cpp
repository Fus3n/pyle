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
        bool w_IsGamepadAvailable(VM&, int g) { return IsGamepadAvailable(g); }
        bool w_IsGamepadButtonPressed(VM&, int g, int b) { return IsGamepadButtonPressed(g, b); }
        bool w_IsGamepadButtonDown(VM&, int g, int b) { return IsGamepadButtonDown(g, b); }
        bool w_IsGamepadButtonReleased(VM&, int g, int b) { return IsGamepadButtonReleased(g, b); }
        bool w_IsGamepadButtonUp(VM&, int g, int b) { return IsGamepadButtonUp(g, b); }
        float w_GetGamepadAxisMovement(VM&, int g, int a) { return GetGamepadAxisMovement(g, a); }
        int w_GetGamepadAxisCount(VM&, int g) { return GetGamepadAxisCount(g); }
        const char* w_GetGamepadName(VM&, int g) { return GetGamepadName(g); }
        void w_SetGamepadMappings(VM&, const std::string& m) { SetGamepadMappings(m.c_str()); }
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
            })
            .function<w_IsGamepadAvailable>("IsGamepadAvailable")
            .function<w_IsGamepadButtonPressed>("IsGamepadButtonPressed")
            .function<w_IsGamepadButtonDown>("IsGamepadButtonDown")
            .function<w_IsGamepadButtonReleased>("IsGamepadButtonReleased")
            .function<w_IsGamepadButtonUp>("IsGamepadButtonUp")
            .function<w_GetGamepadAxisMovement>("GetGamepadAxisMovement")
            .function<w_GetGamepadAxisCount>("GetGamepadAxisCount")
            .raw_function("GetGamepadName", +[](VM& vm, ArgView args) -> Value {
                if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetGamepadName expects (gamepad)."); return Value(); }
                int g = from_value<int64_t>(vm, args[0]);
                return to_value(vm, std::string(GetGamepadName(g)));
            })
            .function<w_SetGamepadMappings>("SetGamepadMappings");
    }

}
}
