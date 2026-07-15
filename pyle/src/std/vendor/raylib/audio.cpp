#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    Value native_InitAudioDevice(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "InitAudioDevice takes 0 arguments."); return Value(); }
        InitAudioDevice();
        return Value();
    }

    Value native_CloseAudioDevice(VM& vm, ArgView args) {
        if (args.size() != 0) { vm.runtime_error(RuntimeError::ArgumentError, "CloseAudioDevice takes 0 arguments."); return Value(); }
        CloseAudioDevice();
        return Value();
    }

    Value ctor_sound(VM& vm, ArgView) {
        return to_value_owned<Sound>(vm, new Sound{});
    }

    Value native_LoadSound(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadSound expects 1 string."); return Value(); }
        const std::string& path = from_value<std::string>(vm, args[0]);
        Sound s = LoadSound(path.c_str());
        Sound* copy = new Sound{};
        *copy = s;
        return to_value_owned<Sound>(vm, copy);
    }

    Value native_PlaySound(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "PlaySound expects 1 Sound."); return Value(); }
        Sound* s = as_native<Sound>(vm, args[0], "Sound");
        if (!s) return Value();
        PlaySound(*s);
        return Value();
    }

    Value native_UnloadSound(VM& vm, ArgView args) {
        if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadSound expects 1 Sound."); return Value(); }
        Sound* s = as_native<Sound>(vm, args[0], "Sound");
        if (!s) return Value();
        UnloadSound(*s);
        return Value();
    }

    void register_audio(VM& vm, MapType& exports) {
        ClassBinder<Sound> sound(vm, "Sound");
        sound.custom_constructor(ctor_sound);
        add_class(vm, exports, "Sound", sound.get_constructor());

        add_fn(vm, exports, "InitAudioDevice", native_InitAudioDevice);
        add_fn(vm, exports, "CloseAudioDevice", native_CloseAudioDevice);
        add_fn(vm, exports, "LoadSound", native_LoadSound);
        add_fn(vm, exports, "PlaySound", native_PlaySound);
        add_fn(vm, exports, "UnloadSound", native_UnloadSound);
    }

}
}
