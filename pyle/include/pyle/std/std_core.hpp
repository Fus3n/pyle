
#pragma once
#include "pyle/vm.hpp"

namespace pyle {
    void register_core_natives(VM& vm);

    Value native_print(VM& vm, ArgView args);
    Value native_printf(VM& vm, ArgView args);
    Value native_format(VM& vm, ArgView args);
    Value native_clock(VM& vm, ArgView args);
}