
#pragma once
#include "pyle/value.hpp"

namespace pyle {
    void register_core_natives(VM& vm, bool load_core_modules = true);

    Value native_print(VM& vm, ArgView args);
    Value native_printf(VM& vm, ArgView args);
    Value native_format(VM& vm, ArgView args);
}