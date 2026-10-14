#pragma once
#include "pyle/value.hpp"
#include <atomic> 
#include <memory>
#include <functional>
#include <mutex>

namespace pyle {

    struct Future {
        std::atomic<bool> finished{false};
        std::atomic<bool> cancelled{false};
        bool failed = false;
        std::string error;

        Value raw_value; 
        Future() = default;

        bool is_done() const { return finished.load(); }
        bool has_failed() const { return failed; }
        bool is_cancelled() const { return cancelled.load(); }
        std::string get_error() const { return error; }
        
        Value get_data(VM& vm) {
            return raw_value; 
        }

        void resolve(Value value) {
            raw_value = value;
            finished.store(true);
        }

        void reject(VM& vm, Value err_val);

        bool cancel() {
            std::lock_guard<std::mutex> lock(cancel_mutex);
            if (finished.load()) return false;
            if (!on_cancel) return false;
            cancelled.store(true);
            on_cancel();
            return true;
        }

        void set_cancel_hook(std::function<void()> fn) {
            std::lock_guard<std::mutex> lock(cancel_mutex);
            on_cancel = std::move(fn);
        }

        void gc_mark(VM& vm);

        static std::pair<Value, std::shared_ptr<Future>> create(VM& vm);

    private:
        std::function<void()> on_cancel;
        std::mutex cancel_mutex;
    };

    void register_core_future(VM& vm);
}
