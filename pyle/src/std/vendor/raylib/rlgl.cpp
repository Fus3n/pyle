#include "raylib_binding.hpp"
#include "rlgl.h"

namespace pyle {
namespace raylib_binding {

    void register_rlgl(NativeModule& mod) {
        mod.raw_function("rlSetTexture", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "rlSetTexture expects (id)."); return Value(); }
            rlSetTexture(from_value<int64_t>(vm, args[0]));
            return Value();
        });
        
        mod.raw_function("rlBegin", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 1) { vm.runtime_error(RuntimeError::ArgumentError, "rlBegin expects (mode)."); return Value(); }
            rlBegin(from_value<int64_t>(vm, args[0]));
            return Value();
        });
        
        mod.function<rlEnd>("rlEnd");
        
        mod.raw_function("rlTexCoord2f", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 2) { vm.runtime_error(RuntimeError::ArgumentError, "rlTexCoord2f expects (u, v)."); return Value(); }
            rlTexCoord2f(from_value<float>(vm, args[0]), from_value<float>(vm, args[1]));
            return Value();
        });
        
        mod.raw_function("rlVertex3f", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 3) { vm.runtime_error(RuntimeError::ArgumentError, "rlVertex3f expects (x, y, z)."); return Value(); }
            rlVertex3f(from_value<float>(vm, args[0]), from_value<float>(vm, args[1]), from_value<float>(vm, args[2]));
            return Value();
        });

        mod.raw_function("rlColor4ub", +[](VM& vm, ArgView args) -> Value {
            if (args.size() != 4) { vm.runtime_error(RuntimeError::ArgumentError, "rlColor4ub expects (r, g, b, a)."); return Value(); }
            rlColor4ub(
                static_cast<unsigned char>(from_value<int64_t>(vm, args[0])),
                static_cast<unsigned char>(from_value<int64_t>(vm, args[1])),
                static_cast<unsigned char>(from_value<int64_t>(vm, args[2])),
                static_cast<unsigned char>(from_value<int64_t>(vm, args[3]))
            );
            return Value();
        });
    }

}
}