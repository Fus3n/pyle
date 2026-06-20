#pragma once
#include "pyle/vm.hpp"

namespace pyle {
    class Pyle {
    public:
        VM vm;
    public:
        bool execute(std::string_view source, bool disassamble = true);
    };
}