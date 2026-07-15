#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    namespace {
        bool w_IsKeyDown(VM&, int key) { return IsKeyDown(key); }
        bool w_IsKeyPressed(VM&, int key) { return IsKeyPressed(key); }
        bool w_IsKeyReleased(VM&, int key) { return IsKeyReleased(key); }
        int  w_GetMouseX(VM&) { return GetMouseX(); }
        int  w_GetMouseY(VM&) { return GetMouseY(); }
        bool w_IsMouseButtonPressed(VM&, int b) { return IsMouseButtonPressed(b); }
        bool w_IsMouseButtonDown(VM&, int b) { return IsMouseButtonDown(b); }
        float w_GetMouseWheelMove(VM&) { return GetMouseWheelMove(); }
    }

    Value native_GetMousePosition(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "GetMousePosition expects 0 args."); return Value(); }
        Vector2 v = GetMousePosition();
        return to_value_owned<Vector2>(vm, new Vector2{ v.x, v.y });
    }

    void register_input(VM& vm, MapType& exports) {
        add_fn(vm, exports, "IsKeyDown", pyle::FreeFnDeducer<w_IsKeyDown>::wrap);
        add_fn(vm, exports, "IsKeyPressed", pyle::FreeFnDeducer<w_IsKeyPressed>::wrap);
        add_fn(vm, exports, "IsKeyReleased", pyle::FreeFnDeducer<w_IsKeyReleased>::wrap);
        add_fn(vm, exports, "GetMouseX", pyle::FreeFnDeducer<w_GetMouseX>::wrap);
        add_fn(vm, exports, "GetMouseY", pyle::FreeFnDeducer<w_GetMouseY>::wrap);
        add_fn(vm, exports, "IsMouseButtonPressed", pyle::FreeFnDeducer<w_IsMouseButtonPressed>::wrap);
        add_fn(vm, exports, "IsMouseButtonDown", pyle::FreeFnDeducer<w_IsMouseButtonDown>::wrap);
        add_fn(vm, exports, "GetMousePosition", native_GetMousePosition);
        add_fn(vm, exports, "GetMouseWheelMove", pyle::FreeFnDeducer<w_GetMouseWheelMove>::wrap);
    }

}
}
