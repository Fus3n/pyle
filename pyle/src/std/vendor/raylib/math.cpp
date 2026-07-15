#include "raylib_binding.hpp"
#include <cmath>

namespace pyle {
namespace raylib_binding {

    Value ctor_vec2(VM& vm, ArgView args) {
        float x = args.size() > 0 ? from_value<float>(vm, args[0]) : 0.0f;
        float y = args.size() > 1 ? from_value<float>(vm, args[1]) : 0.0f;
        return to_value_owned<Vector2>(vm, new Vector2{ x, y });
    }

    Value ctor_rect(VM& vm, ArgView args) {
        float x = args.size() > 0 ? from_value<float>(vm, args[0]) : 0.0f;
        float y = args.size() > 1 ? from_value<float>(vm, args[1]) : 0.0f;
        float w = args.size() > 2 ? from_value<float>(vm, args[2]) : 0.0f;
        float h = args.size() > 3 ? from_value<float>(vm, args[3]) : 0.0f;
        return to_value_owned<Rectangle>(vm, new Rectangle{ x, y, w, h });
    }

    Value ctor_color(VM& vm, ArgView args) {
        unsigned char r = args.size() > 0 ? static_cast<unsigned char>(from_value<int64_t>(vm, args[0])) : 0;
        unsigned char g = args.size() > 1 ? static_cast<unsigned char>(from_value<int64_t>(vm, args[1])) : 0;
        unsigned char b = args.size() > 2 ? static_cast<unsigned char>(from_value<int64_t>(vm, args[2])) : 0;
        unsigned char a = args.size() > 3 ? static_cast<unsigned char>(from_value<int64_t>(vm, args[3])) : 255;
        return to_value_owned<Color>(vm, new Color{ r, g, b, a });
    }

    Value ctor_camera(VM& vm, ArgView) {
        return to_value_owned<Camera2D>(vm, new Camera2D{});
    }

    Value ctor_texture(VM& vm, ArgView) {
        return to_value_owned<Texture2D>(vm, new Texture2D{});
    }

    Value native_Vector2Add(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Add expects (v1, v2)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        Vector2* b = as_native<Vector2>(vm, args[1], "Vector2");
        if (!a || !b) return Value();
        return to_value_owned<Vector2>(vm, new Vector2{ a->x + b->x, a->y + b->y });
    }

    Value native_Vector2Subtract(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Subtract expects (v1, v2)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        Vector2* b = as_native<Vector2>(vm, args[1], "Vector2");
        if (!a || !b) return Value();
        return to_value_owned<Vector2>(vm, new Vector2{ a->x - b->x, a->y - b->y });
    }

    Value native_Vector2Scale(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Scale expects (v, scale)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        if (!a) return Value();
        float s = from_value<float>(vm, args[1]);
        return to_value_owned<Vector2>(vm, new Vector2{ a->x * s, a->y * s });
    }

    Value native_Vector2Length(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Length expects (v)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        if (!a) return Value();
        return to_value(vm, static_cast<double>(std::sqrt(a->x * a->x + a->y * a->y)));
    }

    Value native_Vector2Distance(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Distance expects (v1, v2)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        Vector2* b = as_native<Vector2>(vm, args[1], "Vector2");
        if (!a || !b) return Value();
        float dx = a->x - b->x, dy = a->y - b->y;
        return to_value(vm, static_cast<double>(std::sqrt(dx * dx + dy * dy)));
    }

    Value native_Vector2Normalize(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Normalize expects (v)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        if (!a) return Value();
        float len = std::sqrt(a->x * a->x + a->y * a->y);
        if (len > 0.0f) return to_value_owned<Vector2>(vm, new Vector2{ a->x / len, a->y / len });
        return to_value_owned<Vector2>(vm, new Vector2{ 0.0f, 0.0f });
    }

    Value native_Vector2Zero(VM& vm, ArgView) {
        return to_value_owned<Vector2>(vm, new Vector2{ 0.0f, 0.0f });
    }

    Value native_Vector2Angle(VM& vm, ArgView args) {
        if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector2Angle expects (v1, v2)."); return Value(); }
        Vector2* a = as_native<Vector2>(vm, args[0], "Vector2");
        Vector2* b = as_native<Vector2>(vm, args[1], "Vector2");
        if (!a || !b) return Value();
        float dot = a->x * b->x + a->y * b->y;
        float det = a->x * b->y - a->y * b->x;
        return to_value(vm, static_cast<double>(std::atan2(det, dot)));
    }

    void register_math(VM& vm, MapType& exports) {
        ClassBinder<Vector2> vec2(vm, "Vector2");
        vec2.custom_constructor(ctor_vec2)
            .member<float, &Vector2::x>("x")
            .member<float, &Vector2::y>("y");
        add_class(vm, exports, "Vector2", vec2.get_constructor());

        ClassBinder<Rectangle> rect(vm, "Rectangle");
        rect.custom_constructor(ctor_rect)
            .member<float, &Rectangle::x>("x")
            .member<float, &Rectangle::y>("y")
            .member<float, &Rectangle::width>("width")
            .member<float, &Rectangle::height>("height");
        add_class(vm, exports, "Rectangle", rect.get_constructor());

        ClassBinder<Color> color(vm, "Color");
        color.custom_constructor(ctor_color)
            .member<unsigned char, &Color::r>("r")
            .member<unsigned char, &Color::g>("g")
            .member<unsigned char, &Color::b>("b")
            .member<unsigned char, &Color::a>("a");
        add_class(vm, exports, "Color", color.get_constructor());

        ClassBinder<Camera2D> cam(vm, "Camera2D");
        cam.custom_constructor(ctor_camera)
            .custom_getter("offset", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* c = native_at<Camera2D>(vm, o, "Camera2D");
                if (!c) return Value();
                return to_value_owned<Vector2>(vm, new Vector2{ c->offset.x, c->offset.y });
            })
            .custom_setter("offset", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* c = native_at<Camera2D>(vm, o, "Camera2D");
                auto* v = as_native<Vector2>(vm, a[0], "Vector2");
                if (!c || !v) return Value();
                c->offset = *v;
                return a[0];
            })
            .custom_getter("target", +[](VM& vm, HeapIdx o, ArgView) -> Value {
                auto* c = native_at<Camera2D>(vm, o, "Camera2D");
                if (!c) return Value();
                return to_value_owned<Vector2>(vm, new Vector2{ c->target.x, c->target.y });
            })
            .custom_setter("target", +[](VM& vm, HeapIdx o, ArgView a) -> Value {
                auto* c = native_at<Camera2D>(vm, o, "Camera2D");
                auto* v = as_native<Vector2>(vm, a[0], "Vector2");
                if (!c || !v) return Value();
                c->target = *v;
                return a[0];
            })
            .member<float, &Camera2D::rotation>("rotation")
            .member<float, &Camera2D::zoom>("zoom");
        add_class(vm, exports, "Camera2D", cam.get_constructor());

        add_fn(vm, exports, "Vector2Add", native_Vector2Add);
        add_fn(vm, exports, "Vector2Subtract", native_Vector2Subtract);
        add_fn(vm, exports, "Vector2Scale", native_Vector2Scale);
        add_fn(vm, exports, "Vector2Length", native_Vector2Length);
        add_fn(vm, exports, "Vector2Distance", native_Vector2Distance);
        add_fn(vm, exports, "Vector2Normalize", native_Vector2Normalize);
        add_fn(vm, exports, "Vector2Zero", native_Vector2Zero);
        add_fn(vm, exports, "Vector2Angle", native_Vector2Angle);
    }

}
}
