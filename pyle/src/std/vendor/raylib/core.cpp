#include "raylib_binding.hpp"

namespace pyle {
namespace raylib_binding {

    void register_core(VM& vm, MapType& exports) {
        add_fn(vm, exports, "FileExists", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "FileExists expects 1 string."); return Value(); }
            return to_value(vm, FileExists(from_value<std::string>(vm, args[0]).c_str()));
        });
        add_fn(vm, exports, "DirectoryExists", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "DirectoryExists expects 1 string."); return Value(); }
            return to_value(vm, DirectoryExists(from_value<std::string>(vm, args[0]).c_str()));
        });
        add_fn(vm, exports, "IsFileExtension", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "IsFileExtension expects (fileName, ext)."); return Value(); }
            return to_value(vm, IsFileExtension(from_value<std::string>(vm, args[0]).c_str(), from_value<std::string>(vm, args[1]).c_str()));
        });
        add_fn(vm, exports, "GetFileExtension", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetFileExtension expects 1 string."); return Value(); }
            return to_value(vm, std::string(GetFileExtension(from_value<std::string>(vm, args[0]).c_str())));
        });
        add_fn(vm, exports, "GetFileName", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetFileName expects 1 string."); return Value(); }
            return to_value(vm, std::string(GetFileName(from_value<std::string>(vm, args[0]).c_str())));
        });
        add_fn(vm, exports, "GetFileNameWithoutExt", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetFileNameWithoutExt expects 1 string."); return Value(); }
            return to_value(vm, std::string(GetFileNameWithoutExt(from_value<std::string>(vm, args[0]).c_str())));
        });
        add_fn(vm, exports, "GetDirectoryPath", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetDirectoryPath expects 1 string."); return Value(); }
            return to_value(vm, std::string(GetDirectoryPath(from_value<std::string>(vm, args[0]).c_str())));
        });
        add_fn(vm, exports, "GetWorkingDirectory", +[](VM& vm, ArgView) -> Value {
            return to_value(vm, std::string(GetWorkingDirectory()));
        });
        add_fn(vm, exports, "GetApplicationDirectory", +[](VM& vm, ArgView) -> Value {
            return to_value(vm, std::string(GetApplicationDirectory()));
        });
        add_fn(vm, exports, "GetMonitorCount", +[](VM& vm, ArgView) -> Value {
            return to_value(vm, GetMonitorCount());
        });
        add_fn(vm, exports, "GetCurrentMonitor", +[](VM& vm, ArgView) -> Value {
            return to_value(vm, GetCurrentMonitor());
        });
        add_fn(vm, exports, "GetMonitorWidth", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMonitorWidth expects monitor."); return Value(); }
            return to_value(vm, GetMonitorWidth(from_value<int64_t>(vm, args[0])));
        });
        add_fn(vm, exports, "GetMonitorHeight", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMonitorHeight expects monitor."); return Value(); }
            return to_value(vm, GetMonitorHeight(from_value<int64_t>(vm, args[0])));
        });
        add_fn(vm, exports, "GetMonitorRefreshRate", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "GetMonitorRefreshRate expects monitor."); return Value(); }
            return to_value(vm, GetMonitorRefreshRate(from_value<int64_t>(vm, args[0])));
        });
        add_fn(vm, exports, "SetWindowTitle", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowTitle expects 1 string."); return Value(); }
            SetWindowTitle(from_value<std::string>(vm, args[0]).c_str());
            return Value();
        });
        add_fn(vm, exports, "SetWindowPosition", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowPosition expects (x, y)."); return Value(); }
            SetWindowPosition(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]));
            return Value();
        });
        add_fn(vm, exports, "SetWindowSize", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowSize expects (width, height)."); return Value(); }
            SetWindowSize(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]));
            return Value();
        });
        add_fn(vm, exports, "SetWindowMinSize", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowMinSize expects (width, height)."); return Value(); }
            SetWindowMinSize(from_value<int64_t>(vm, args[0]), from_value<int64_t>(vm, args[1]));
            return Value();
        });
        add_fn(vm, exports, "SetWindowOpacity", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowOpacity expects 1 float."); return Value(); }
            SetWindowOpacity(from_value<float>(vm, args[0]));
            return Value();
        });
        add_fn(vm, exports, "SetWindowState", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "SetWindowState expects flags."); return Value(); }
            SetWindowState(static_cast<unsigned int>(from_value<int64_t>(vm, args[0])));
            return Value();
        });
        add_fn(vm, exports, "ClearWindowState", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "ClearWindowState expects flags."); return Value(); }
            ClearWindowState(static_cast<unsigned int>(from_value<int64_t>(vm, args[0])));
            return Value();
        });
        add_fn(vm, exports, "IsWindowState", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "IsWindowState expects flags."); return Value(); }
            return to_value(vm, IsWindowState(static_cast<unsigned int>(from_value<int64_t>(vm, args[0]))));
        });
        add_fn(vm, exports, "OpenURL", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "OpenURL expects 1 string."); return Value(); }
            OpenURL(from_value<std::string>(vm, args[0]).c_str());
            return Value();
        });
        add_fn(vm, exports, "SetClipboardText", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "SetClipboardText expects 1 string."); return Value(); }
            SetClipboardText(from_value<std::string>(vm, args[0]).c_str());
            return Value();
        });
        add_fn(vm, exports, "GetClipboardText", +[](VM& vm, ArgView) -> Value {
            return to_value(vm, std::string(GetClipboardText()));
        });
        add_fn(vm, exports, "TakeScreenshot", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "TakeScreenshot expects 1 string."); return Value(); }
            TakeScreenshot(from_value<std::string>(vm, args[0]).c_str());
            return Value();
        });
        add_fn(vm, exports, "SetRandomSeed", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "SetRandomSeed expects 1 int."); return Value(); }
            SetRandomSeed(static_cast<unsigned int>(from_value<int64_t>(vm, args[0])));
            return Value();
        });
        add_fn(vm, exports, "LoadFileData", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "LoadFileData expects 1 string."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            int bytesRead = 0;
            unsigned char* data = LoadFileData(path.c_str(), &bytesRead);
            if (!data) return Value();
            BytesType buffer(data, data + bytesRead);
            UnloadFileData(data);
            HeapIdx idx = vm.alloc(Object(std::move(buffer)));
            return Value(Value::Tag::BytesRef, idx);
        });
        add_fn(vm, exports, "SaveFileData", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "SaveFileData expects (fileName, data)."); return Value(); }
            const std::string& path = from_value<std::string>(vm, args[0]);
            if (args[1].tag != Value::Tag::BytesRef) {
                vm.runtime_error(RuntimeError::Type, "Expected bytes for data.");
                return Value();
            }
            const auto& buffer = std::get<BytesType>(vm.get_heap_object(args[1].as_ref).data);
            return to_value(vm, SaveFileData(path.c_str(), const_cast<void*>(static_cast<const void*>(buffer.data())), static_cast<int>(buffer.size())));
        });
    }

}
}
