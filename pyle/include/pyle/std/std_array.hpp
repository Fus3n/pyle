#pragma once
#include <string>
#include "pyle/value.hpp"


namespace pyle {
    class VM;

    namespace ArrayMethods {
        Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args);
        
    }
}