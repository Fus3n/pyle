#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_drawing(NativeModule& mod) {
        mod.raw_function("ClearBackground", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ClearBackground expects 1 Color."); return Value(); }
            Color* c = as_native<Color>(vm, args[0], "Color");
            if (!c) return Value();
            ClearBackground(*c);
            return Value();
        });
        
        mod.function<BeginDrawing>("BeginDrawing")
           .function<EndDrawing>("EndDrawing")
           .raw_function("BeginBlendMode", +[](VM& vm, ArgView args) -> Value {
               if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginBlendMode expects (mode)."); return Value(); }
               BeginBlendMode(from_value<int64_t>(vm, args[0]));
               return Value();
           })
           .function<EndBlendMode>("EndBlendMode")
           .raw_function("BeginScissorMode", +[](VM& vm, ArgView args) -> Value {
               if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "BeginScissorMode expects (x, y, width, height)."); return Value(); }
               BeginScissorMode(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]), from_value<int64_t>(vm, args[2]), from_value<int64_t>(vm, args[3]));
               return Value();
           })
           .function<EndScissorMode>("EndScissorMode");

        mod.raw_function("BeginMode2D", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginMode2D expects 1 Camera2D."); return Value(); }
            Camera2D* c = as_native<Camera2D>(vm, args[0], "Camera2D");
            if (!c) return Value();
            BeginMode2D(*c);
            return Value();
        });

        mod.function<EndMode2D>("EndMode2D");

        mod.raw_function("DrawPixel", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPixel expects (posX, posY, color)."); return Value(); }
            int x = from_value<int64_t>(vm, args[0]);
            int y = from_value<int64_t>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!c) return Value();
            DrawPixel(x, y, *c);
            return Value();
        });

        mod.raw_function("DrawPixelV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPixelV expects (position, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Color* c = as_native<Color>(vm, args[1], "Color");
            if (!p || !c) return Value();
            DrawPixelV(*p, *c);
            return Value();
        });

        mod.raw_function("DrawLine", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLine expects (startPosX, startPosY, endPosX, endPosY, color)."); return Value(); }
            int sx = from_value<int64_t>(vm, args[0]);
            int sy = from_value<int64_t>(vm, args[1]);
            int ex = from_value<int64_t>(vm, args[2]);
            int ey = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawLine(sx, sy, ex, ey, *c);
            return Value();
        });

        mod.raw_function("DrawLineV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLineV expects (startPos, endPos, color)."); return Value(); }
            Vector2* s = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!s || !e || !c) return Value();
            DrawLineV(*s, *e, *c);
            return Value();
        });

        mod.raw_function("DrawLineEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLineEx expects (startPos, endPos, thick, color)."); return Value(); }
            Vector2* s = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e = as_native<Vector2>(vm, args[1], "Vector2");
            float t = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!s || !e || !c) return Value();
            DrawLineEx(*s, *e, t, *c);
            return Value();
        });

        mod.raw_function("DrawLineBezier", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawLineBezier expects (startPos, endPos, thick, color)."); return Value(); }
            Vector2* s = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e = as_native<Vector2>(vm, args[1], "Vector2");
            float t = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!s || !e || !c) return Value();
            DrawLineBezier(*s, *e, t, *c);
            return Value();
        });

        mod.raw_function("DrawCircle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircle expects (centerX, centerY, radius, color)."); return Value(); }
            int cx = from_value<int64_t>(vm, args[0]);
            int cy = from_value<int64_t>(vm, args[1]);
            float r = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!c) return Value();
            DrawCircle(cx, cy, r, *c);
            return Value();
        });

        mod.raw_function("DrawCircleV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleV expects (center, radius, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            float r = from_value<float>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!center || !c) return Value();
            DrawCircleV(*center, r, *c);
            return Value();
        });

        mod.raw_function("DrawCircleLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleLines expects (centerX, centerY, radius, color)."); return Value(); }
            int cx = from_value<int64_t>(vm, args[0]);
            int cy = from_value<int64_t>(vm, args[1]);
            float r = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!c) return Value();
            DrawCircleLines(cx, cy, r, *c);
            return Value();
        });

        mod.raw_function("DrawCircleLinesV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawCircleLinesV expects (center, radius, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            float r = from_value<float>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!center || !c) return Value();
            DrawCircleLinesV(*center, r, *c);
            return Value();
        });

        mod.raw_function("DrawEllipse", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawEllipse expects (centerX, centerY, radiusH, radiusV, color)."); return Value(); }
            int cx = from_value<int64_t>(vm, args[0]);
            int cy = from_value<int64_t>(vm, args[1]);
            float rh = from_value<float>(vm, args[2]);
            float rv = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawEllipse(cx, cy, rh, rv, *c);
            return Value();
        });

        mod.raw_function("DrawEllipseLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawEllipseLines expects (centerX, centerY, radiusH, radiusV, color)."); return Value(); }
            int cx = from_value<int64_t>(vm, args[0]);
            int cy = from_value<int64_t>(vm, args[1]);
            float rh = from_value<float>(vm, args[2]);
            float rv = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawEllipseLines(cx, cy, rh, rv, *c);
            return Value();
        });

        mod.raw_function("DrawRing", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 7) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRing expects (center, innerRadius, outerRadius, startAngle, endAngle, segments, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            float in_r = from_value<float>(vm, args[1]);
            float out_r = from_value<float>(vm, args[2]);
            float start = from_value<float>(vm, args[3]);
            float end = from_value<float>(vm, args[4]);
            int segs = from_value<int64_t>(vm, args[5]);
            Color* c = as_native<Color>(vm, args[6], "Color");
            if (!center || !c) return Value();
            DrawRing(*center, in_r, out_r, start, end, segs, *c);
            return Value();
        });

        mod.raw_function("DrawRingLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 7) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRingLines expects (center, innerRadius, outerRadius, startAngle, endAngle, segments, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            float in_r = from_value<float>(vm, args[1]);
            float out_r = from_value<float>(vm, args[2]);
            float start = from_value<float>(vm, args[3]);
            float end = from_value<float>(vm, args[4]);
            int segs = from_value<int64_t>(vm, args[5]);
            Color* c = as_native<Color>(vm, args[6], "Color");
            if (!center || !c) return Value();
            DrawRingLines(*center, in_r, out_r, start, end, segs, *c);
            return Value();
        });

        mod.raw_function("DrawRectangle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangle expects (posX, posY, width, height, color)."); return Value(); }
            int x = from_value<int64_t>(vm, args[0]);
            int y = from_value<int64_t>(vm, args[1]);
            int w = from_value<int64_t>(vm, args[2]);
            int h = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawRectangle(x, y, w, h, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleV expects (position, size, color)."); return Value(); }
            Vector2* p = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* s = as_native<Vector2>(vm, args[1], "Vector2");
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!p || !s || !c) return Value();
            DrawRectangleV(*p, *s, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRec expects (rec, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            Color* c = as_native<Color>(vm, args[1], "Color");
            if (!r || !c) return Value();
            DrawRectangleRec(*r, *c);
            return Value();
        });

        mod.raw_function("DrawRectanglePro", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectanglePro expects (rec, origin, rotation, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            Vector2* o = as_native<Vector2>(vm, args[1], "Vector2");
            float rot = from_value<float>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!r || !o || !c) return Value();
            DrawRectanglePro(*r, *o, rot, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleGradientV", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleGradientV expects (posX, posY, width, height, color1, color2)."); return Value(); }
            int x = from_value<int64_t>(vm, args[0]);
            int y = from_value<int64_t>(vm, args[1]);
            int w = from_value<int64_t>(vm, args[2]);
            int h = from_value<int64_t>(vm, args[3]);
            Color* c1 = as_native<Color>(vm, args[4], "Color");
            Color* c2 = as_native<Color>(vm, args[5], "Color");
            if (!c1 || !c2) return Value();
            DrawRectangleGradientV(x, y, w, h, *c1, *c2);
            return Value();
        });

        mod.raw_function("DrawRectangleGradientH", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleGradientH expects (posX, posY, width, height, color1, color2)."); return Value(); }
            int x = from_value<int64_t>(vm, args[0]);
            int y = from_value<int64_t>(vm, args[1]);
            int w = from_value<int64_t>(vm, args[2]);
            int h = from_value<int64_t>(vm, args[3]);
            Color* c1 = as_native<Color>(vm, args[4], "Color");
            Color* c2 = as_native<Color>(vm, args[5], "Color");
            if (!c1 || !c2) return Value();
            DrawRectangleGradientH(x, y, w, h, *c1, *c2);
            return Value();
        });

        mod.raw_function("DrawRectangleGradientEx", +[](VM& vm, ArgView args) -> Value {
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

        mod.raw_function("DrawRectangleLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleLines expects (posX, posY, width, height, color)."); return Value(); }
            int x = from_value<int64_t>(vm, args[0]);
            int y = from_value<int64_t>(vm, args[1]);
            int w = from_value<int64_t>(vm, args[2]);
            int h = from_value<int64_t>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!c) return Value();
            DrawRectangleLines(x, y, w, h, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleLinesEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleLinesEx expects (rec, lineThick, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            float t = from_value<float>(vm, args[1]);
            Color* c = as_native<Color>(vm, args[2], "Color");
            if (!r || !c) return Value();
            DrawRectangleLinesEx(*r, t, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleRounded", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRounded expects (rec, roundness, segments, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            float roundness = from_value<float>(vm, args[1]);
            int segments = from_value<int64_t>(vm, args[2]);
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!r || !c) return Value();
            DrawRectangleRounded(*r, roundness, segments, *c);
            return Value();
        });

        mod.raw_function("DrawRectangleRoundedLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRoundedLines expects (rec, roundness, segments, lineThick, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            float roundness = from_value<float>(vm, args[1]);
            int segments = from_value<int64_t>(vm, args[2]);
            float lineThick = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!r || !c) return Value();
            DrawRectangleRoundedLinesEx(*r, roundness, segments, lineThick, *c);
            return Value();
        });
        mod.raw_function("DrawRectangleRoundedLinesEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawRectangleRoundedLinesEx expects (rec, roundness, segments, lineThick, color)."); return Value(); }
            Rectangle* r = as_native<Rectangle>(vm, args[0], "Rectangle");
            float roundness = from_value<float>(vm, args[1]);
            int segments = from_value<int64_t>(vm, args[2]);
            float lineThick = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!r || !c) return Value();
            DrawRectangleRoundedLinesEx(*r, roundness, segments, lineThick, *c);
            return Value();
        });

        mod.raw_function("DrawTriangle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTriangle expects (v1, v2, v3, color)."); return Value(); }
            Vector2* v1 = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* v2 = as_native<Vector2>(vm, args[1], "Vector2");
            Vector2* v3 = as_native<Vector2>(vm, args[2], "Vector2");
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!v1 || !v2 || !v3 || !c) return Value();
            DrawTriangle(*v1, *v2, *v3, *c);
            return Value();
        });

        mod.raw_function("DrawTriangleLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "DrawTriangleLines expects (v1, v2, v3, color)."); return Value(); }
            Vector2* v1 = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* v2 = as_native<Vector2>(vm, args[1], "Vector2");
            Vector2* v3 = as_native<Vector2>(vm, args[2], "Vector2");
            Color* c = as_native<Color>(vm, args[3], "Color");
            if (!v1 || !v2 || !v3 || !c) return Value();
            DrawTriangleLines(*v1, *v2, *v3, *c);
            return Value();
        });

        mod.raw_function("DrawPoly", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPoly expects (center, sides, radius, rotation, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            int sides = from_value<int64_t>(vm, args[1]);
            float radius = from_value<float>(vm, args[2]);
            float rotation = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!center || !c) return Value();
            DrawPoly(*center, sides, radius, rotation, *c);
            return Value();
        });

        mod.raw_function("DrawPolyLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 5) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPolyLines expects (center, sides, radius, rotation, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            int sides = from_value<int64_t>(vm, args[1]);
            float radius = from_value<float>(vm, args[2]);
            float rotation = from_value<float>(vm, args[3]);
            Color* c = as_native<Color>(vm, args[4], "Color");
            if (!center || !c) return Value();
            DrawPolyLines(*center, sides, radius, rotation, *c);
            return Value();
        });

        mod.raw_function("DrawFPS", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "DrawFPS expects (posX, posY)."); return Value(); }
            DrawFPS(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]));
            return Value();
        });
        mod.raw_function("DrawPolyLinesEx", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "DrawPolyLinesEx expects (center, sides, radius, rotation, lineThick, color)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            int sides = from_value<int64_t>(vm, args[1]);
            float radius = from_value<float>(vm, args[2]);
            float rotation = from_value<float>(vm, args[3]);
            float lineThick = from_value<float>(vm, args[4]);
            Color* c = as_native<Color>(vm, args[5], "Color");
            if (!center || !c) return Value();
            DrawPolyLinesEx(*center, sides, radius, rotation, lineThick, *c);
            return Value();
        });

        mod.raw_function("CheckCollisionRecs", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionRecs expects (rec1, rec2)."); return Value(); }
            Rectangle* r1 = as_native<Rectangle>(vm, args[0], "Rectangle");
            Rectangle* r2 = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!r1 || !r2) return Value();
            return to_value(vm, CheckCollisionRecs(*r1, *r2));
        });

        mod.raw_function("CheckCollisionCircles", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionCircles expects (center1, radius1, center2, radius2)."); return Value(); }
            Vector2* c1 = as_native<Vector2>(vm, args[0], "Vector2");
            float r1 = from_value<float>(vm, args[1]);
            Vector2* c2 = as_native<Vector2>(vm, args[2], "Vector2");
            float r2 = from_value<float>(vm, args[3]);
            if (!c1 || !c2) return Value();
            return to_value(vm, CheckCollisionCircles(*c1, r1, *c2, r2));
        });

        mod.raw_function("CheckCollisionCircleRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionCircleRec expects (center, radius, rec)."); return Value(); }
            Vector2* center = as_native<Vector2>(vm, args[0], "Vector2");
            float radius = from_value<float>(vm, args[1]);
            Rectangle* r = as_native<Rectangle>(vm, args[2], "Rectangle");
            if (!center || !r) return Value();
            return to_value(vm, CheckCollisionCircleRec(*center, radius, *r));
        });

        mod.raw_function("CheckCollisionPointRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointRec expects (point, rec)."); return Value(); }
            Vector2* point = as_native<Vector2>(vm, args[0], "Vector2");
            Rectangle* r = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!point || !r) return Value();
            return to_value(vm, CheckCollisionPointRec(*point, *r));
        });

        mod.raw_function("CheckCollisionPointCircle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointCircle expects (point, center, radius)."); return Value(); }
            Vector2* point = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* center = as_native<Vector2>(vm, args[1], "Vector2");
            float radius = from_value<float>(vm, args[2]);
            if (!point || !center) return Value();
            return to_value(vm, CheckCollisionPointCircle(*point, *center, radius));
        });

        mod.raw_function("CheckCollisionPointTriangle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointTriangle expects (point, p1, p2, p3)."); return Value(); }
            Vector2* point = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* p1 = as_native<Vector2>(vm, args[1], "Vector2");
            Vector2* p2 = as_native<Vector2>(vm, args[2], "Vector2");
            Vector2* p3 = as_native<Vector2>(vm, args[3], "Vector2");
            if (!point || !p1 || !p2 || !p3) return Value();
            return to_value(vm, CheckCollisionPointTriangle(*point, *p1, *p2, *p3));
        });

        mod.raw_function("CheckCollisionPointPoly", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointPoly expects (point, points, pointCount)."); return Value(); }
            Vector2* point = as_native<Vector2>(vm, args[0], "Vector2");
            
            const Value& list_val = args[1];
            if (list_val.tag != Value::Tag::ArrayRef) {
                vm.runtime_error(RuntimeError::Type, "Expected list for points.");
                return Value();
            }
            Object& list_obj = vm.get_heap_object(list_val.as_ref);
            auto& items = std::get<ArrayType>(list_obj.data);
            int count = static_cast<int>(items.size());

            Vector2* points = new Vector2[count];
            for (int i = 0; i < count; ++i) {
                Vector2* p = as_native<Vector2>(vm, items[i], "Vector2");
                if (p) points[i] = *p;
                else points[i] = {0, 0};
            }

            bool result = CheckCollisionPointPoly(*point, points, count);
            delete[] points;
            return to_value(vm, result);
        });

        mod.raw_function("CheckCollisionLines", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionLines expects (startPos1, endPos1, startPos2, endPos2)."); return Value(); }
            Vector2* s1 = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* e1 = as_native<Vector2>(vm, args[1], "Vector2");
            Vector2* s2 = as_native<Vector2>(vm, args[2], "Vector2");
            Vector2* e2 = as_native<Vector2>(vm, args[3], "Vector2");
            if (!s1 || !e1 || !s2 || !e2) return Value();
            
            Vector2 collisionPoint;
            bool hit = CheckCollisionLines(*s1, *e1, *s2, *e2, &collisionPoint);
            
            MapType result;
            result[to_value(vm, "hit")] = to_value(vm, hit);
            result[to_value(vm, "point")] = to_value_owned<Vector2>(vm, new Vector2{ collisionPoint.x, collisionPoint.y });
            return Value(Value::Tag::MapRef, vm.alloc(Object(std::move(result))));
        });

        mod.raw_function("CheckCollisionPointLine", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "CheckCollisionPointLine expects (point, p1, p2, threshold)."); return Value(); }
            Vector2* point = as_native<Vector2>(vm, args[0], "Vector2");
            Vector2* p1 = as_native<Vector2>(vm, args[1], "Vector2");
            Vector2* p2 = as_native<Vector2>(vm, args[2], "Vector2");
            int threshold = from_value<int64_t>(vm, args[3]);
            if (!point || !p1 || !p2) return Value();
            return to_value(vm, CheckCollisionPointLine(*point, *p1, *p2, threshold));
        });

        mod.raw_function("GetCollisionRec", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetCollisionRec expects (rec1, rec2)."); return Value(); }
            Rectangle* r1 = as_native<Rectangle>(vm, args[0], "Rectangle");
            Rectangle* r2 = as_native<Rectangle>(vm, args[1], "Rectangle");
            if (!r1 || !r2) return Value();
            Rectangle result = GetCollisionRec(*r1, *r2);
            return to_value_owned<Rectangle>(vm, new Rectangle{ result.x, result.y, result.width, result.height });
        });
    }

}
}
