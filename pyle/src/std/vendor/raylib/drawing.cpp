#include "raylib_binding.hpp"
#include <rlgl.h>


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
        add_fn(vm, exports, "DrawFPS", +[](VM& vm, ArgView args) -> Value {
            int x = args.size() > 0 ? from_value<int64_t>(vm, args[0]) : 10;
            int y = args.size() > 1 ? from_value<int64_t>(vm, args[1]) : 10;
            DrawFPS(x, y);
            return Value();
        });
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

        add_fn(vm, exports, "Fade", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Fade expects (color, alpha)."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            float a = from_value<float>(vm, args[1]);
            if (!c) return Value();
            Color r = Fade(*c, a);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorFromHSV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "ColorFromHSV expects (h, s, v)."); return Value(); }
            float h = from_value<float>(vm, args[0]);
            float s = from_value<float>(vm, args[1]);
            float v = from_value<float>(vm, args[2]);
            Color r = ColorFromHSV(h, s, v);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorToHSV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ColorToHSV expects 1 Color."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            if (!c) return Value();
            Vector3 r = ColorToHSV(*c);
            return to_value_owned<Vector3>(vm, new Vector3{ r.x, r.y, r.z });
        });
        add_fn(vm, exports, "ColorTint", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ColorTint expects (color, tint)."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            Color* t = as_native<Color>(vm, args[1], "Color");
            if (!c || !t) return Value();
            Color r = ColorTint(*c, *t);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorBrightness", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ColorBrightness expects (color, factor)."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            float f = from_value<float>(vm, args[1]);
            if (!c) return Value();
            Color r = ColorBrightness(*c, f);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorContrast", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ColorContrast expects (color, contrast)."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            float cn = from_value<float>(vm, args[1]);
            if (!c) return Value();
            Color r = ColorContrast(*c, cn);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorLerp", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "ColorLerp expects (c1, c2, factor)."); return Value(); }
            Color* a = as_native<Color>(vm, args[0], "Color");
            Color* b = as_native<Color>(vm, args[1], "Color");
            float f = from_value<float>(vm, args[2]);
            if (!a || !b) return Value();
            Color r = ColorLerp(*a, *b, f);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "ColorIsEqual", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "ColorIsEqual expects (c1, c2)."); return Value(); }
            Color* a = as_native<Color>(vm, args[0], "Color");
            Color* b = as_native<Color>(vm, args[1], "Color");
            if (!a || !b) return Value();
            return to_value(vm, ColorIsEqual(*a, *b));
        });
        add_fn(vm, exports, "GetColor", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetColor expects hexValue."); return Value(); }
            unsigned int hex = static_cast<unsigned int>(from_value<int64_t>(vm, args[0]));
            Color r = GetColor(hex);
            return to_value_owned<Color>(vm, new Color{ r.r, r.g, r.b, r.a });
        });
        add_fn(vm, exports, "DrawCircleV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleV expects (center, radius, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            float r = from_value<float>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !c) return Value();
            DrawCircleV(*p, r, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawRectangleV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleV expects (pos, size, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* sz = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !sz || !c) return Value();
            DrawRectangleV(*p, *sz, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawRectanglePro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectanglePro expects (rec, origin, rotation, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            Vector2* o = as_native<Vector2>(vm, args[1], "Vector2");
            float rot = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!r || !o || !c) return Value();
            DrawRectanglePro(*r, *o, rot, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawRectangleGradientEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleGradientEx expects (rec, col1, col2, col3, col4)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            Color* c1 = as_native<Color>(vm, args[1], "Color");
            Color* c2 = as_native<Color>(vm, args[2], "Color");
            Color* c3 = as_native<Color>(vm, args[3], "Color");
            Color* c4 = as_native<Color>(vm, args[4], "Color");
            if (!r || !c1 || !c2 || !c3 || !c4) return Value();
            DrawRectangleGradientEx(*r, *c1, *c2, *c3, *c4);
            return Value();
        });
        add_fn(vm, exports, "DrawEllipse", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawEllipse expects (cx, cy, rh, rv, color)."); return Value(); }
            int cx = from_value<int64_t>(vm, args[0]), cy = from_value<int64_t>(vm, args[1]);
            float rh = from_value<float>(vm, args[2]), rv = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawEllipse(cx, cy, rh, rv, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawLineV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLineV expects (start, end, color)."); return Value(); }
            Vector2* s = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!s || !e || !c) return Value();
            DrawLineV(*s, *e, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawLineEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLineEx expects (start, end, thick, color)."); return Value(); }
            Vector2* s = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e = as_native<Vector2>(vm, args[1], "Vector2");
            float th = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!s || !e || !c) return Value();
            DrawLineEx(*s, *e, th, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawPoly", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPoly expects (center, sides, radius, rotation, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            int sides = from_value<int64_t>(vm, args[1]);
            float r = from_value<float>(vm, args[2]);
            float rot = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!p || !c) return Value();
            DrawPoly(*p, sides, r, rot, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawCircleSector", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleSector expects (center, radius, startAngle, endAngle, segments, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            float r = from_value<float>(vm, args[1]);
            float sa = from_value<float>(vm, args[2]), ea = from_value<float>(vm, args[3]);
            int seg = from_value<int64_t>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!p || !c) return Value();
            DrawCircleSector(*p, r, sa, ea, seg, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawRing", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 7) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRing expects (center, innerR, outerR, startA, endA, seg, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            float ir = from_value<float>(vm, args[1]), or_ = from_value<float>(vm, args[2]);
            float sa = from_value<float>(vm, args[3]), ea = from_value<float>(vm, args[4]);
            int seg = from_value<int64_t>(vm, args[5]);
            Color* c = as_native<Color>(vm, args[6], "Color");
            if (!p || !c) return Value();
            DrawRing(*p, ir, or_, sa, ea, seg, *c);
            return Value();
        });
        add_fn(vm, exports, "DrawTextPro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 8) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTextPro expects (font, text, pos, origin, rot, size, spacing, tint)."); return Value(); }
            Font* f = as_native<Font>(vm, args[0], "Font");
            const std::string& text = from_value<std::string>(vm, args[1]);
            Vector2* p = as_native<Vector2>(vm, args[2], "Vector2");
            Vector2* o = as_native<Vector2>(vm, args[3], "Vector2");
            float rot = from_value<float>(vm, args[4]);
            float sz = from_value<float>(vm, args[5]);
            float sp = from_value<float>(vm, args[6]);
            Color* t = as_native<Color>(vm, args[7], "Color");
            if (!f || !p || !o || !t) return Value();
            DrawTextPro(*f, text.c_str(), *p, *o, rot, sz, sp, *t);
            return Value();
        });
        add_fn(vm, exports, "SetShapesTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetShapesTexture expects (texture, source)."); return Value(); }
            Texture2D* t = as_native<Texture2D>(vm, args[0], "Texture");
            Rectangle* s = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!t || !s) return Value();
            SetShapesTexture(*t, *s);
            return Value();
        });
        add_fn(vm, exports, "BeginBlendMode", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginBlendMode expects 1 int."); return Value(); }
            BeginBlendMode(from_value<int64_t>(vm, args[0]));
            return Value();
        });
        add_fn(vm, exports, "EndBlendMode", +[](VM&, ArgView) -> Value {
            EndBlendMode();
            return Value();
        });
        add_fn(vm, exports, "BeginScissorMode", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "BeginScissorMode expects (x, y, w, h)."); return Value(); }
            BeginScissorMode(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]),
                from_value<int64_t>(vm, args[2]), from_value<int64_t>(vm, args[3]));
            return Value();
        });
        add_fn(vm, exports, "EndScissorMode", +[](VM&, ArgView) -> Value {
            EndScissorMode();
            return Value();
        });
        add_fn(vm, exports, "CheckCollisionPointCircle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointCircle expects (point, center, radius)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* c = as_native<Vector2>(vm, args[1], "Vector2");
            float r = from_value<float>(vm, args[2]);
            if (!p || !c) return Value();
            return to_value(vm, CheckCollisionPointCircle(*p, *c, r));
        });
        add_fn(vm, exports, "CheckCollisionCircleLine", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionCircleLine expects (center, radius, p1, p2)."); return Value(); }
            Vector2* c = as_native<Vector2>(vm, args[0], "Vector2");
            float r = from_value<float>(vm, args[1]);
            Vector2* p1 = as_native<Vector2>(vm, args[2], "Vector2");
            Vector2* p2 = as_native<Vector2>(vm, args[3], "Vector2");
            if (!c || !p1 || !p2) return Value();
            return to_value(vm, CheckCollisionCircleLine(*c, r, *p1, *p2));
        });
        add_fn(vm, exports, "GetCollisionRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetCollisionRec expects (r1, r2)."); return Value(); }
            Rectangle* a = as_native<Rectangle>(vm, args[0], "Rectangle");
            Rectangle* b = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!a || !b) return Value();
            Rectangle r = GetCollisionRec(*a, *b);
            return to_value_owned<Rectangle>(vm, new Rectangle{ r.x, r.y, r.width, r.height });
        });

            add_fn(vm, exports, "GetCollisionRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetCollisionRec expects (r1, r2)."); return Value(); }
            Rectangle* a = as_native<Rectangle>(vm, args[0], "Rectangle");
            Rectangle* b = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!a || !b) return Value();
            Rectangle r = GetCollisionRec(*a, *b);
            return to_value_owned<Rectangle>(vm, new Rectangle{ r.x, r.y, r.width, r.height });
        });
    }

}
}
