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

    Value native_GetMousePosition(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "GetMousePosition expects 0 args."); return Value(); }
        Vector2 v = GetMousePosition();
        return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
    }

    Value native_GetMouseDelta(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "GetMouseDelta expects 0 args."); return Value(); }
        Vector2 v = GetMouseDelta();
        return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
    }

    Value native_GetMouseWheelMoveV(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "GetMouseWheelMoveV expects 0 args."); return Value(); }
        Vector2 v = GetMouseWheelMoveV();
        return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
    }

    void register_input(VM& vm, MapType& exports) {
        add_fn(vm, exports, "IsKeyDown", pyle::FreeFnDeducer<w_IsKeyDown>::wrap);
        add_fn(vm, exports, "IsKeyPressed", pyle::FreeFnDeducer<w_IsKeyPressed>::wrap);
        add_fn(vm, exports, "IsKeyReleased", pyle::FreeFnDeducer<w_IsKeyReleased>::wrap);
        add_fn(vm, exports, "IsKeyUp", pyle::FreeFnDeducer<w_IsKeyUp>::wrap);
        add_fn(vm, exports, "IsKeyPressedRepeat", pyle::FreeFnDeducer<w_IsKeyPressedRepeat>::wrap);
        add_fn(vm, exports, "GetKeyPressed", pyle::FreeFnDeducer<w_GetKeyPressed>::wrap);
        add_fn(vm, exports, "GetCharPressed", pyle::FreeFnDeducer<w_GetCharPressed>::wrap);
        add_fn(vm, exports, "GetMouseX", pyle::FreeFnDeducer<w_GetMouseX>::wrap);
        add_fn(vm, exports, "GetMouseY", pyle::FreeFnDeducer<w_GetMouseY>::wrap);
        add_fn(vm, exports, "IsMouseButtonPressed", pyle::FreeFnDeducer<w_IsMouseButtonPressed>::wrap);
        add_fn(vm, exports, "IsMouseButtonDown", pyle::FreeFnDeducer<w_IsMouseButtonDown>::wrap);
        add_fn(vm, exports, "IsMouseButtonReleased", pyle::FreeFnDeducer<w_IsMouseButtonReleased>::wrap);
        add_fn(vm, exports, "IsMouseButtonUp", pyle::FreeFnDeducer<w_IsMouseButtonUp>::wrap);
        add_fn(vm, exports, "GetMousePosition", native_GetMousePosition);
        add_fn(vm, exports, "GetMouseDelta", native_GetMouseDelta);
        add_fn(vm, exports, "GetMouseWheelMove", pyle::FreeFnDeducer<w_GetMouseWheelMove>::wrap);
        add_fn(vm, exports, "GetMouseWheelMoveV", native_GetMouseWheelMoveV);
        add_fn(vm, exports, "SetMouseCursor", pyle::FreeFnDeducer<w_SetMouseCursor>::wrap);
        add_fn(vm, exports, "ShowCursor", pyle::FreeFnDeducer<w_ShowCursor>::wrap);
        add_fn(vm, exports, "HideCursor", pyle::FreeFnDeducer<w_HideCursor>::wrap);
        add_fn(vm, exports, "EnableCursor", pyle::FreeFnDeducer<w_EnableCursor>::wrap);
        add_fn(vm, exports, "DisableCursor", pyle::FreeFnDeducer<w_DisableCursor>::wrap);
        add_fn(vm, exports, "IsCursorHidden", pyle::FreeFnDeducer<w_IsCursorHidden>::wrap);
    }

}
}
