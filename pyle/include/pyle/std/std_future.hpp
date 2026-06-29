#pragma once
#include "pyle/value.hpp"

namespace pyle {

    struct Future {
        std::atomic<bool> finished{false};
        bool failed = false;
        std::string error;

        std::string raw_string;
        std::vector<uint8_t> raw_bytes;

        Future() = default;

        bool is_done() const { return finished.load(); }
        bool has_failed() const { return failed; }
        std::string get_error() const { return error; }
        
        Value get_data(VM& vm);
    };

    void register_core_future(VM& vm);
}
