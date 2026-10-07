
#pragma once
#include "pyle/value.hpp"

namespace pyle {
    class NativeModule;
    namespace proc {
        void bind_to_os(VM& vm, NativeModule& mod);
    }
    void register_core_modules(VM& vm);
    Value math_module_factory(VM& vm);
}