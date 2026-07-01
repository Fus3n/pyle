#pragma once
#include "pyle/value.hpp"
#include <atomic> 

namespace pyle {

    struct Future {
        std::atomic<bool> finished{false};
        bool failed = false;
        std::string error;

        Value raw_value; 
        Future() = default;

        bool is_done() const { return finished.load(); }
        bool has_failed() const { return failed; }
        std::string get_error() const { return error; }
        
        Value get_data(VM& vm) {
            return raw_value; 
        }
    };

    void register_core_future(VM& vm);
}
