
#pragma once
#include "pyle/value.hpp"

namespace pyle {
    void register_core_natives(VM& vm, bool load_core_modules = true);
}