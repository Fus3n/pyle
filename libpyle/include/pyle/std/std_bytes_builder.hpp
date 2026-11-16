#pragma once

#include "pyle/value.hpp"
#include "pyle/vm.hpp"

namespace pyle::BytesBuilderMethods {

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args);
}
