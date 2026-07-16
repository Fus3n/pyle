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

        // ---------------------------------------------------------------- Music
        ClassBinder<Music> music(vm, "Music");
        music.custom_constructor(+[](VM& vm, ArgView) -> Value {
            return to_value_owned<Music>(vm, new Music{});
        })
            .member<bool, &Music::looping>("looping");
        mod.class_type(music);

        mod.raw_function("LoadMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadMusicStream expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            Music m = LoadMusicStream(path.c_str());
            return to_value_owned<Music>(vm, new Music{ m.stream, m.frameCount, m.looping, m.ctxType, m.ctxData });
        });

        mod.raw_function("UnloadMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UnloadMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            UnloadMusicStream(*m);
            return Value();
        });

        mod.raw_function("PlayMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "PlayMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            PlayMusicStream(*m);
            return Value();
        });

        mod.raw_function("UpdateMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "UpdateMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            UpdateMusicStream(*m);
            return Value();
        });

        mod.raw_function("StopMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "StopMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            StopMusicStream(*m);
            return Value();
        });

        mod.raw_function("PauseMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "PauseMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            PauseMusicStream(*m);
            return Value();
        });

        mod.raw_function("ResumeMusicStream", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ResumeMusicStream expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            ResumeMusicStream(*m);
            return Value();
        });

        mod.raw_function("IsMusicStreamPlaying", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsMusicStreamPlaying expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            return to_value(vm, IsMusicStreamPlaying(*m));
        });

        mod.raw_function("SetMusicVolume", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetMusicVolume expects (music, volume)."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            float v = from_value<float>(vm, args[1]);
            if (!m) return Value();
            SetMusicVolume(*m, v);
            return Value();
        });

        mod.raw_function("SetMusicPitch", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetMusicPitch expects (music, pitch)."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            float p = from_value<float>(vm, args[1]);
            if (!m) return Value();
            SetMusicPitch(*m, p);
            return Value();
        });

        mod.raw_function("SetMusicPan", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetMusicPan expects (music, pan)."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            float p = from_value<float>(vm, args[1]);
            if (!m) return Value();
            SetMusicPan(*m, p);
            return Value();
        });

        mod.raw_function("GetMusicTimeLength", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMusicTimeLength expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            return to_value(vm, GetMusicTimeLength(*m));
        });

        mod.raw_function("GetMusicTimePlayed", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMusicTimePlayed expects 1 Music."); return Value(); }
            Music* m = as_native<Music>(vm, args[0], "Music");
            if (!m) return Value();
            return to_value(vm, GetMusicTimePlayed(*m));
        });
    }

}
}
