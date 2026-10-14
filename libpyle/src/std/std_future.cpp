#include "pyle/std/std_future.hpp"
#include "pyle/vm.hpp"
#include "pyle/binder.hpp"
#include <memory>

namespace pyle {

    void Future::reject(VM& vm, Value err_val) {
        error = pyle::from_value<std::string>(vm, err_val);
        failed = true;
        finished.store(true);
    }

    bool Future::cancel(VM& vm) {
        std::function<void()> native;
        Value pv;
        bool has_pv = false;
        {
            std::lock_guard<std::mutex> lock(cancel_mutex);
            if (finished.load()) return false;
            if (!on_cancel && !has_pyle_cancel) return false;
            cancelled.store(true);
            native = on_cancel;
            pv = pyle_cancel;
            has_pv = has_pyle_cancel;
        }
        if (native) native();
        if (has_pv) vm.call_func_n(pv, nullptr, 0);
        return true;
    }

    std::pair<Value, std::shared_ptr<Future>> Future::create(VM& vm) {
        auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
        auto copy = *sp;
        return { to_value_owned(vm, sp), copy };
    }

    void Future::gc_mark(VM& vm) {
        vm.mark_value(raw_value);
        vm.mark_value(pyle_cancel);
    }

    void register_core_future(VM& vm) {
        SharedClassBinder<Future>(vm, "Future")
            .custom_constructor([](VM& vm, ArgView args) -> Value {
                auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
                return to_value_owned(vm, sp);
            })
            .getter<&Future::is_done>("is_done")
            .getter<&Future::has_failed>("has_failed")
            .getter<&Future::is_cancelled>("is_cancelled")
            .getter<&Future::get_data>("data")
            .getter<&Future::get_error>("error")
            .method<&Future::resolve>("resolve")
            .method<&Future::reject>("reject")
            .method<&Future::cancel>("cancel")
            .custom_method("set_cancel_hook", [](VM& vm, HeapIdx self, ArgView args) -> Value {
                if (args.size() != 1) {
                    vm.runtime_error(RuntimeError::ArgumentError, "set_cancel_hook expects 1 callable.");
                    return Value();
                }
                auto& native = std::get<NativeObject>(vm.get_heap_object(self).data);
                auto* sp = static_cast<std::shared_ptr<Future>*>(native.ptr);
                if (!sp || !*sp) return Value();
                (*sp)->set_cancel_hook(args[0]);
                return Value();
            })
            .register_globally();
    }
}