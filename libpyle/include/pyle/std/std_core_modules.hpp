
#pragma once
#include "pyle/value.hpp"

namespace pyle {
    class NativeModule;
    namespace proc {
        void bind_to_os(VM& vm, NativeModule& mod);
    }
    void register_os_module(VM& vm);
    void register_path_module(VM& vm);
    void register_fs_module(VM& vm);
    void register_color_module(VM& vm);
    void register_math_module(VM& vm);
    void register_proc_module(VM& vm);
    void register_core_modules(VM& vm);
    Value math_module_factory(VM& vm);
    Value path_module_factory(VM& vm);
    Value fs_module_factory(VM& vm);
}