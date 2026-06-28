#pragma once
#include "pyle/value.hpp"

namespace pyle::MapMethods {
    bool has_method(const std::string& name);
    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args);
}
