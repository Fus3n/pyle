#pragma once
#include "pyle/value.hpp"
#include <atomic> 
#include <memory>

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

        void resolve(Value value) {
            raw_value = value;
            finished.store(true);
        }

        void reject(VM& vm, Value err_val);

        void gc_mark(VM& vm);

        static std::pair<Value, std::shared_ptr<Future>> create(VM& vm);
    };

    void register_core_future(VM& vm);
}
