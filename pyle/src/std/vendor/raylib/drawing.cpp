#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    Value native_ClearBackground(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ClearBackground expects 1 Color."); return Value(); }
        Color* c = as_native<Color>(vm, args[0], "Color");
        if (!c) return Value();
        ClearBackground(*c);
        return Value();
    }

    Value native_DrawText(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawText expects (text, x, y, size, color)."); return Value(); }
        const std::string& text = from_value<std::string>(vm, args[0]);
        int x = from_value<int64_t>(vm, args[1]);
        int y = from_value<int64_t>(vm, args[2]);
        int size = from_value<int64_t>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!c) return Value();
        DrawText(text.c_str(), x, y, size, *c);
        return Value();
    }

    Value native_DrawRectangle(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangle expects (x, y, w, h, color)."); return Value(); }
        int x = from_value<int64_t>(vm, args[0]);
        int y = from_value<int64_t>(vm, args[1]);
        int w = from_value<int64_t>(vm, args[2]);
        int h = from_value<int64_t>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!c) return Value();
        DrawRectangle(x, y, w, h, *c);
        return Value();
    }

    Value native_DrawRectangleRec(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRec expects (Rectangle, Color)."); return Value(); }
        Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
        Color* c = as_native<Color>(vm, args[1], "Color");
        if (!r || !c) return Value();
        DrawRectangleRec(*r, *c);
        return Value();
    }

    Value native_DrawRectangleLines(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleLines expects (x, y, w, h, color)."); return Value(); }
        int x = from_value<int64_t>(vm, args[0]);
        int y = from_value<int64_t>(vm, args[1]);
        int w = from_value<int64_t>(vm, args[2]);
        int h = from_value<int64_t>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!c) return Value();
        DrawRectangleLines(x, y, w, h, *c);
        return Value();
    }

    Value native_DrawCircle(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircle expects (x, y, radius, color)."); return Value(); }
        int x = from_value<int64_t>(vm, args[0]);
        int y = from_value<int64_t>(vm, args[1]);
        float r = from_value<float>(vm, args[2]);
        Color* c = as_native<Color>(vm, args[3], "Color");
        if (!c) return Value();
        DrawCircle(x, y, r, *c);
        return Value();
    }

    Value native_DrawCircleLines(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleLines expects (x, y, radius, color)."); return Value(); }
        int x = from_value<int64_t>(vm, args[0]);
        int y = from_value<int64_t>(vm, args[1]);
        float r = from_value<float>(vm, args[2]);
        Color* c = as_native<Color>(vm, args[3], "Color");
        if (!c) return Value();
        DrawCircleLines(x, y, r, *c);
        return Value();
    }

    Value native_DrawLine(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLine expects (x1, y1, x2, y2, color)."); return Value(); }
        int x1 = from_value<int64_t>(vm, args[0]);
        int y1 = from_value<int64_t>(vm, args[1]);
        int x2 = from_value<int64_t>(vm, args[2]);
        int y2 = from_value<int64_t>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!c) return Value();
        DrawLine(x1, y1, x2, y2, *c);
        return Value();
    }

    Value native_DrawTriangle(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTriangle expects (v1, v2, v3, color)."); return Value(); }
        Vector2* v1 = as_native<Vector2>(vm, args[0], "Vector2");
        Vector2* v2 = as_native<Vector2>(vm, args[1], "Vector2");
        Vector2* v3 = as_native<Vector2>(vm, args[2], "Vector2");
        Color* c = as_native<Color>(vm, args[3], "Color");
        if (!v1 || !v2 || !v3 || !c) return Value();
        DrawTriangle(*v1, *v2, *v3, *c);
        return Value();
    }

    Value native_MeasureText(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "MeasureText expects (text, size)."); return Value(); }
        const std::string& text = from_value<std::string>(vm, args[0]);
        int size = from_value<int64_t>(vm, args[1]);
        return to_value(vm, MeasureText(text.c_str(), size));
    }

    Value native_DrawRectangleRounded(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRounded expects (Rectangle, roundness, segments, color)."); return Value(); }
        Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
        float roundness = from_value<float>(vm, args[1]);
        int segments = from_value<int64_t>(vm, args[2]);
        Color* c = as_native<Color>(vm, args[3], "Color");
        if (!r || !c) return Value();
        DrawRectangleRounded(*r, roundness, segments, *c);
        return Value();
    }

    Value native_BeginMode2D(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginMode2D expects (Camera2D)."); return Value(); }
        Camera2D* cam = as_native<Camera2D>(vm, args[0], "Camera2D");
        if (!cam) return Value();
        BeginMode2D(*cam);
        return Value();
    }

    Value native_EndMode2D(VM& vm, ArgView) {
        EndMode2D();
        return Value();
    }

    // Collision checks.
    Value native_CheckCollisionRecs(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionRecs expects (r1, r2)."); return Value(); }
        Rectangle* a = as_native<Rectangle>(vm, args[0], "Rectangle");
        Rectangle* b = as_native<Rectangle>(vm, args[1], "Rectangle");
        if (!a || !b) return Value();
        return to_value(vm, CheckCollisionRecs(*a, *b));
    }

    Value native_CheckCollisionCircleRec(VM& vm, ArgView args) {
        if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionCircleRec expects (center, radius, rec)."); return Value(); }
        Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
        float radius = from_value<float>(vm, args[1]);
        Rectangle* r = as_native<Rectangle>(vm, args[2], "Rectangle");
        if (!center || !r) return Value();
        return to_value(vm, CheckCollisionCircleRec(*center, radius, *r));
    }

    Value native_CheckCollisionCircles(VM& vm, ArgView args) {
        if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionCircles expects (c1, r1, c2, r2)."); return Value(); }
        Vector2* c1 = as_native<Vector2>(vm, args[0], "Vector2");
        float r1 = from_value<float>(vm, args[1]);
        Vector2* c2 = as_native<Vector2>(vm, args[2], "Vector2");
        float r2 = from_value<float>(vm, args[3]);
        if (!c1 || !c2) return Value();
        return to_value(vm, CheckCollisionCircles(*c1, r1, *c2, r2));
    }

    Value native_CheckCollisionPointRec(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointRec expects (point, rec)."); return Value(); }
        Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
        Rectangle* r = as_native<Rectangle>(vm, args[1], "Rectangle");
        if (!p || !r) return Value();
        return to_value(vm, CheckCollisionPointRec(*p, *r));
    }

    Value native_DrawRectangleRoundedLines(VM& vm, ArgView args) {
        if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRoundedLines expects (Rectangle, roundness, segments, lineThick, color)."); return Value(); }
        Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
        float roundness = from_value<float>(vm, args[1]);
        int segments = from_value<int64_t>(vm, args[2]);
        float lineThick = from_value<float>(vm, args[3]);
        Color* c = as_native<Color>(vm, args[4], "Color");
        if (!r || !c) return Value();
        DrawRectangleRoundedLinesEx(*r, roundness, segments, lineThick, *c);
        return Value();
    }

    Value native_ColorAlpha(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ColorAlpha expects (color, alpha)."); return Value(); }
        Color* c = as_native<Color>(vm, args[0], "Color");
        float a = from_value<float>(vm, args[1]);
        if (!c) return Value();
        Color result = ColorAlpha(*c, a);
        return to_value_owned<Color>(vm, new Color{result.r, result.g, result.b, result.a});
    }

    void register_drawing(VM& vm, MapType& exports) {
        add_fn(vm, exports, "BeginDrawing", +[](VM&, ArgView) -> Value { BeginDrawing(); return Value(); });
        add_fn(vm, exports, "EndDrawing", +[](VM&, ArgView) -> Value { EndDrawing(); return Value(); });
        add_fn(vm, exports, "ClearBackground", native_ClearBackground);
        add_fn(vm, exports, "DrawText", native_DrawText);
        add_fn(vm, exports, "DrawRectangle", native_DrawRectangle);
        add_fn(vm, exports, "DrawRectangleRec", native_DrawRectangleRec);
        add_fn(vm, exports, "DrawRectangleLines", native_DrawRectangleLines);
        add_fn(vm, exports, "DrawCircle", native_DrawCircle);
        add_fn(vm, exports, "DrawCircleLines", native_DrawCircleLines);
        add_fn(vm, exports, "DrawLine", native_DrawLine);
        add_fn(vm, exports, "DrawTriangle", native_DrawTriangle);
        add_fn(vm, exports, "MeasureText", native_MeasureText);
        add_fn(vm, exports, "DrawRectangleRounded", native_DrawRectangleRounded);
        add_fn(vm, exports, "DrawRectangleRoundedLines", native_DrawRectangleRoundedLines);
        add_fn(vm, exports, "ColorAlpha", native_ColorAlpha);
        add_fn(vm, exports, "BeginMode2D", native_BeginMode2D);
        add_fn(vm, exports, "EndMode2D", native_EndMode2D);
        add_fn(vm, exports, "CheckCollisionRecs", native_CheckCollisionRecs);
        add_fn(vm, exports, "CheckCollisionCircleRec", native_CheckCollisionCircleRec);
        add_fn(vm, exports, "CheckCollisionCircles", native_CheckCollisionCircles);
        add_fn(vm, exports, "CheckCollisionPointRec", native_CheckCollisionPointRec);
    }

}
}
