#pragma once
#include "pyle/value.hpp"

namespace pyle::FiberMethods {
    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args);
}
