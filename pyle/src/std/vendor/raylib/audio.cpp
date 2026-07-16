#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_audio(NativeModule& mod) {
        VM& vm = mod.get_vm();

        ClassBinder<Sound> sound(vm, "Sound");
        sound.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Sound>(vm, new Sound{});
        });
        mod.class_type(sound);

        mod.raw_function("InitAudioDevice", +[](VM& vm, ArgView args) -> Value {
            InitAudioDevice();
            return Value();
        });
        
        mod.raw_function("CloseAudioDevice", +[](VM& vm, ArgView args) -> Value {
            CloseAudioDevice();
            return Value();
        });
        
        mod.raw_function("LoadSound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadSound expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            Sound s = LoadSound(path.c_str());
            return to_value_owned<Sound>(vm, new Sound{ s.stream, s.frameCount });
        });
        
        mod.raw_function("PlaySound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "PlaySound expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            PlaySound(*s);
            return Value();
        });
        
        mod.raw_function("StopSound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "StopSound expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            StopSound(*s);
            return Value();
        });
        
        mod.raw_function("PauseSound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "PauseSound expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            PauseSound(*s);
            return Value();
        });
        
        mod.raw_function("ResumeSound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ResumeSound expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            ResumeSound(*s);
            return Value();
        });
        
        mod.raw_function("UnloadSound", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadSound expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            UnloadSound(*s);
            return Value();
        });
        
        mod.raw_function("IsSoundPlaying", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsSoundPlaying expects 1 Sound."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            if (!s) return Value();
            return to_value(vm, IsSoundPlaying(*s));
        });
        
        mod.raw_function("SetSoundVolume", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetSoundVolume expects (sound, volume)."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            float v = from_value<float>(vm, args[1]);
            if (!s) return Value();
            SetSoundVolume(*s, v);
            return Value();
        });
        
        mod.raw_function("SetSoundPitch", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetSoundPitch expects (sound, pitch)."); return Value(); }
            Sound* s = as_native<Sound>(vm, args[0], "Sound");
            float p = from_value<float>(vm, args[1]);
            if (!s) return Value();
            SetSoundPitch(*s, p);
            return Value();
        });
    }

}
}
