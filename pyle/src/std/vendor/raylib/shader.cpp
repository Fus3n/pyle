#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_shader(VM& vm, MapType& exports) {
        ClassBinder<Shader> shader(vm, "Shader");
        shader.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Shader>(vm, new Shader{});
        });
        add_class(vm, exports, "Shader", shader.get_constructor());

        add_fn(vm, exports, "LoadShader", +[](VM& vm, ArgView args) -> Value {
            std::string vs, fs;
            const char* vs_cstr = nullptr;
            const char* fs_cstr = nullptr;
            if (args.size() == 2) {
                if (args[0].tag != Value::Tag::None) {
                    vs = from_value<std::string>(vm, args[0]);
                    vs_cstr = vs.c_str();
                }
                if (args[1].tag != Value::Tag::None) {
                    fs = from_value<std::string>(vm, args[1]);
                    fs_cstr = fs.c_str();
                }
            } else if (args.size() == 1) {
                if (args[0].tag != Value::Tag::None) {
                    fs = from_value<std::string>(vm, args[0]);
                    fs_cstr = fs.c_str();
                }
            } else {
                vm.runtime_error(RuntimeError::ArgumentError, "LoadShader expects 1 or 2 strings.");
                return Value();
            }
            Shader s = LoadShader(vs_cstr, fs_cstr);
            return to_value_owned<Shader>(vm, new Shader{ s.id, s.locs });
        });
        add_fn(vm, exports, "LoadShaderFromMemory", +[](VM& vm, ArgView args) -> Value {
            std::string vs, fs;
            const char* vs_cstr = nullptr;
            const char* fs_cstr = nullptr;
            if (args.size() == 2) {
                if (args[0].tag != Value::Tag::None) {
                    vs = from_value<std::string>(vm, args[0]);
                    vs_cstr = vs.c_str();
                }
                if (args[1].tag != Value::Tag::None) {
                    fs = from_value<std::string>(vm, args[1]);
                    fs_cstr = fs.c_str();
                }
            } else {
                vm.runtime_error(RuntimeError::ArgumentError, "LoadShaderFromMemory expects 2 strings.");
                return Value();
            }
            Shader s = LoadShaderFromMemory(vs_cstr, fs_cstr);
            return to_value_owned<Shader>(vm, new Shader{ s.id, s.locs });
        });
        add_fn(vm, exports, "IsShaderValid", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsShaderValid expects 1 Shader."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            if (!s) return Value();
            return to_value(vm, IsShaderValid(*s));
        });
        add_fn(vm, exports, "UnloadShader", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadShader expects 1 Shader."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            if (!s) return Value();
            UnloadShader(*s);
            return Value();
        });
        add_fn(vm, exports, "BeginShaderMode", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "BeginShaderMode expects 1 Shader."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            if (!s) return Value();
            BeginShaderMode(*s);
            return Value();
        });
        add_fn(vm, exports, "EndShaderMode", +[](VM&, ArgView) -> Value {
            EndShaderMode();
            return Value();
        });
        add_fn(vm, exports, "GetShaderLocation", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetShaderLocation expects (shader, uniformName)."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            const std::string& name = from_value<std::string>(vm, args[1]);
            if (!s) return Value();
            return to_value(vm, GetShaderLocation(*s, name.c_str()));
        });
        add_fn(vm, exports, "GetShaderLocationAttrib", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "GetShaderLocationAttrib expects (shader, attribName)."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            const std::string& name = from_value<std::string>(vm, args[1]);
            if (!s) return Value();
            return to_value(vm, GetShaderLocationAttrib(*s, name.c_str()));
        });
        add_fn(vm, exports, "SetShaderValue", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "SetShaderValue expects (shader, locIndex, value, uniformType)."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            int loc = from_value<int64_t>(vm, args[1]);
            int type = from_value<int64_t>(vm, args[3]);
            if (!s) return Value();
            const auto& val = args[2];
            switch (type) {
                case 0: {
                    float f = from_value<float>(vm, val);
                    SetShaderValue(*s, loc, &f, type);
                    break;
                }
                case 1: {
                    Vector2* v = as_native<Vector2>(vm, val, "Vector2");
                    if (v) SetShaderValue(*s, loc, v, type);
                    break;
                }
                case 2: {
                    Vector3* v = as_native<Vector3>(vm, val, "Vector3");
                    if (v) SetShaderValue(*s, loc, v, type);
                    break;
                }
                case 4: {
                    int i = from_value<int64_t>(vm, val);
                    SetShaderValue(*s, loc, &i, type);
                    break;
                }
                default:
                    vm.runtime_error(RuntimeError::ArgumentError, "Unsupported uniform type for SetShaderValue.");
                    return Value();
            }
            return Value();
        });
        add_fn(vm, exports, "SetShaderValueMatrix", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "SetShaderValueMatrix expects (shader, locIndex, matrix)."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            int loc = from_value<int64_t>(vm, args[1]);
            Matrix* m = as_native<Matrix>(vm, args[2], "Matrix");
            if (!s || !m) return Value();
            SetShaderValueMatrix(*s, loc, *m);
            return Value();
        });
        add_fn(vm, exports, "SetShaderValueTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "SetShaderValueTexture expects (shader, locIndex, texture)."); return Value(); }
            Shader* s = as_native<Shader>(vm, args[0], "Shader");
            int loc = from_value<int64_t>(vm, args[1]);
            Texture2D* t = as_native<Texture2D>(vm, args[2], "Texture");
            if (!s || !t) return Value();
            SetShaderValueTexture(*s, loc, *t);
            return Value();
        });
    }

}
}
