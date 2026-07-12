
#pragma once
#include "pyle/value.hpp"

namespace pyle {
    void register_core_modules(VM& vm);
    Value math_module_factory(VM& vm);
}