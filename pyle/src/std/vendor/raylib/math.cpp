#include "raylib_binding.hpp"
#include <cmath>
#include <raymath.h>

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

        ClassBinder<Vector3> vec3(vm, "Vector3");
        vec3.custom_constructor(+[](VM& vm, ArgView args) -> Value {
            float x = args.size() > 0 ? from_value<float>(vm, args[0]) : 0.0f;
            float y = args.size() > 1 ? from_value<float>(vm, args[1]) : 0.0f;
            float z = args.size() > 2 ? from_value<float>(vm, args[2]) : 0.0f;
            return to_value_owned<Vector3>(vm, new Vector3{ x, y, z });
        })
            .member<float, &Vector3::x>("x")
            .member<float, &Vector3::y>("y")
            .member<float, &Vector3::z>("z");
        add_class(vm, exports, "Vector3", vec3.get_constructor());

        ClassBinder<Matrix> mat(vm, "Matrix");
        mat.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Matrix>(vm, new Matrix{ 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 });
        })
            .member<float, &Matrix::m0>("m0").member<float, &Matrix::m4>("m4")
            .member<float, &Matrix::m8>("m8").member<float, &Matrix::m12>("m12")
            .member<float, &Matrix::m1>("m1").member<float, &Matrix::m5>("m5")
            .member<float, &Matrix::m9>("m9").member<float, &Matrix::m13>("m13")
            .member<float, &Matrix::m2>("m2").member<float, &Matrix::m6>("m6")
            .member<float, &Matrix::m10>("m10").member<float, &Matrix::m14>("m14")
            .member<float, &Matrix::m3>("m3").member<float, &Matrix::m7>("m7")
            .member<float, &Matrix::m11>("m11").member<float, &Matrix::m15>("m15");
        add_class(vm, exports, "Matrix", mat.get_constructor());

        add_fn(vm, exports, "Vector3Add", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Add expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            return to_value_owned<Vector3>(vm, new Vector3{ a->x + b->x, a->y + b->y, a->z + b->z });
        });
        add_fn(vm, exports, "Vector3Subtract", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Subtract expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            return to_value_owned<Vector3>(vm, new Vector3{ a->x - b->x, a->y - b->y, a->z - b->z });
        });
        add_fn(vm, exports, "Vector3Scale", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Scale expects (v, scale)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            if (!a) return Value();
            float s = from_value<float>(vm, args[1]);
            return to_value_owned<Vector3>(vm, new Vector3{ a->x * s, a->y * s, a->z * s });
        });
        add_fn(vm, exports, "Vector3Length", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Length expects (v)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            if (!a) return Value();
            return to_value(vm, static_cast<double>(std::sqrt(a->x * a->x + a->y * a->y + a->z * a->z)));
        });
        add_fn(vm, exports, "Vector3Distance", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Distance expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
            return to_value(vm, static_cast<double>(std::sqrt(dx * dx + dy * dy + dz * dz)));
        });
        add_fn(vm, exports, "Vector3Normalize", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Normalize expects (v)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            if (!a) return Value();
            float len = std::sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
            if (len > 0.0f) return to_value_owned<Vector3>(vm, new Vector3{ a->x / len, a->y / len, a->z / len });
            return to_value_owned<Vector3>(vm, new Vector3{ 0, 0, 0 });
        });
        add_fn(vm, exports, "Vector3Zero", +[](VM& vm, ArgView) -> Value {
            return to_value_owned<Vector3>(vm, new Vector3{ 0, 0, 0 });
        });
        add_fn(vm, exports, "Vector3DotProduct", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3DotProduct expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            return to_value(vm, static_cast<double>(a->x * b->x + a->y * b->y + a->z * b->z));
        });
        add_fn(vm, exports, "Vector3CrossProduct", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3CrossProduct expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            return to_value_owned<Vector3>(vm, new Vector3{
                a->y * b->z - a->z * b->y,
                a->z * b->x - a->x * b->z,
                a->x * b->y - a->y * b->x
            });
        });
        add_fn(vm, exports, "Vector3Lerp", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Lerp expects (v1, v2, t)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            float t = from_value<float>(vm, args[2]);
            if (!a || !b) return Value();
            return to_value_owned<Vector3>(vm, new Vector3{
                a->x + (b->x - a->x) * t,
                a->y + (b->y - a->y) * t,
                a->z + (b->z - a->z) * t
            });
        });
        add_fn(vm, exports, "Vector3Angle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Angle expects (v1, v2)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* b = as_native<Vector3>(vm, args[1], "Vector3");
            if (!a || !b) return Value();
            float dot = a->x * b->x + a->y * b->y + a->z * b->z;
            float la = std::sqrt(a->x * a->x + a->y * a->y + a->z * a->z);
            float lb = std::sqrt(b->x * b->x + b->y * b->y + b->z * b->z);
            return to_value(vm, static_cast<double>(std::acos(dot / (la * lb))));
        });
        add_fn(vm, exports, "Vector3Negate", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "Vector3Negate expects (v)."); return Value(); }
            Vector3* a = as_native<Vector3>(vm, args[0], "Vector3");
            if (!a) return Value();
            return to_value_owned<Vector3>(vm, new Vector3{ -a->x, -a->y, -a->z });
        });

        add_fn(vm, exports, "MatrixIdentity", +[](VM& vm, ArgView) -> Value {
            Matrix m = MatrixIdentity();
            return to_value_owned<Matrix>(vm, new Matrix(m));
        });
        add_fn(vm, exports, "MatrixMultiply", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixMultiply expects (a, b)."); return Value(); }
            Matrix* a = as_native<Matrix>(vm, args[0], "Matrix");
            Matrix* b = as_native<Matrix>(vm, args[1], "Matrix");
            if (!a || !b) return Value();
            return to_value_owned<Matrix>(vm, new Matrix(MatrixMultiply(*a, *b)));
        });
        add_fn(vm, exports, "MatrixTranslate", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixTranslate expects (x, y, z)."); return Value(); }
            float x = from_value<float>(vm, args[0]);
            float y = from_value<float>(vm, args[1]);
            float z = from_value<float>(vm, args[2]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixTranslate(x, y, z)));
        });
        add_fn(vm, exports, "MatrixRotateX", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixRotateX expects (angle)."); return Value(); }
            float a = from_value<float>(vm, args[0]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixRotateX(a)));
        });
        add_fn(vm, exports, "MatrixRotateY", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixRotateY expects (angle)."); return Value(); }
            float a = from_value<float>(vm, args[0]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixRotateY(a)));
        });
        add_fn(vm, exports, "MatrixRotateZ", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixRotateZ expects (angle)."); return Value(); }
            float a = from_value<float>(vm, args[0]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixRotateZ(a)));
        });
        add_fn(vm, exports, "MatrixScale", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixScale expects (x, y, z)."); return Value(); }
            float x = from_value<float>(vm, args[0]);
            float y = from_value<float>(vm, args[1]);
            float z = from_value<float>(vm, args[2]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixScale(x, y, z)));
        });
        add_fn(vm, exports, "MatrixOrtho", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 6) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixOrtho expects (l, r, b, t, n, f)."); return Value(); }
            float l = from_value<float>(vm, args[0]), r = from_value<float>(vm, args[1]);
            float b = from_value<float>(vm, args[2]), t = from_value<float>(vm, args[3]);
            float n = from_value<float>(vm, args[4]), f = from_value<float>(vm, args[5]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixOrtho(l, r, b, t, n, f)));
        });
        add_fn(vm, exports, "MatrixPerspective", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixPerspective expects (fovY, aspect, near, far)."); return Value(); }
            float fov = from_value<float>(vm, args[0]), aspect = from_value<float>(vm, args[1]);
            float n = from_value<float>(vm, args[2]), f = from_value<float>(vm, args[3]);
            return to_value_owned<Matrix>(vm, new Matrix(MatrixPerspective(fov, aspect, n, f)));
        });
        add_fn(vm, exports, "MatrixLookAt", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "MatrixLookAt expects (eye, target, up)."); return Value(); }
            Vector3* eye = as_native<Vector3>(vm, args[0], "Vector3");
            Vector3* target = as_native<Vector3>(vm, args[1], "Vector3");
            Vector3* up = as_native<Vector3>(vm, args[2], "Vector3");
            if (!eye || !target || !up) return Value();
            return to_value_owned<Matrix>(vm, new Matrix(MatrixLookAt(*eye, *target, *up)));
        });
    }

}
}
