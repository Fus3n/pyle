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

    std::pair<Value, std::shared_ptr<Future>> Future::create(VM& vm) {
        auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
        auto copy = *sp;
        return { to_value_owned(vm, sp), copy };
    }

    void Future::gc_mark(VM& vm) {
        vm.mark_value(raw_value);
    }

    void register_core_future(VM& vm) {
        SharedClassBinder<Future>(vm, "Future")
            .custom_constructor([](VM& vm, ArgView args) -> Value {
                auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
                return to_value_owned(vm, sp);
            })
            .getter<&Future::is_done>("is_done")
            .getter<&Future::has_failed>("has_failed")
            .getter<&Future::get_data>("data")
            .getter<&Future::get_error>("error")
            .method<&Future::resolve>("resolve")
            .method<&Future::reject>("reject")
            .register_globally();
    }
}