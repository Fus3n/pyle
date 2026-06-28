#include "pyle/std/std_coro.hpp"
#include "pyle/vm.hpp"
#include <fmt/format.h>

namespace pyle::CoroMethods {

    Value resume(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() > 1) {
            vm.runtime_error(RuntimeError::ArgumentError, "resume() takes at most 1 argument.");
            return Value();
        }
        
        Value resume_arg = (args.size() == 1) ? args[0] : Value();
        
        Coroutine& target = std::get<Coroutine>(vm.get_heap_object(obj_idx).data);
        if (target.state == Coroutine::State::Running) {
            vm.runtime_error(RuntimeError::Runtime, "Cannot resume an already running coro.");
            return Value();
        }
        if (target.state == Coroutine::State::Dead) {
            vm.runtime_error(RuntimeError::Runtime, "Cannot resume a dead coro.");
            return Value();
        }

        Coroutine& current = std::get<Coroutine>(vm.get_heap_object(vm.active_coroutine_idx).data);
        
        // Save current state
        vm.save_coroutine_state(current);
        current.state = Coroutine::State::Suspended;

        target.caller_idx = vm.active_coroutine_idx;
        target.state = Coroutine::State::Running;

        vm.load_coroutine_state(target);
        
        if (!target.started) {
            target.started = true; 
        } else {
            if (vm.sp == vm.stack_end) {
                vm.grow_stack();
            }
            *(vm.sp++) = resume_arg;
        }

        vm.active_coroutine_idx = obj_idx;
        vm.coro_switched = true; 

        return Value(); 
    }

    Value state(VM& vm, HeapIdx obj_idx, ArgView args) {
        if (args.size() != 0) {
            vm.runtime_error(RuntimeError::ArgumentError, "state() takes 0 arguments.");
            return Value();
        }
        
        Coroutine& target = std::get<Coroutine>(vm.get_heap_object(obj_idx).data);
        std::string state_str = "suspended";
        if (target.state == Coroutine::State::Running) state_str = "running";
        else if (target.state == Coroutine::State::Dead) state_str = "dead";
        
        HeapIdx str_idx = vm.intern_string(state_str);
        return Value(Value::Tag::StringRef, str_idx);
    }

    Value dispatch(VM& vm, HeapIdx obj_idx, const std::string& name, ArgView args) {
        if (name == "resume") {
            return resume(vm, obj_idx, args);
        } else if (name == "state") {
            return state(vm, obj_idx, args);
        }

        vm.runtime_error(RuntimeError::Name, fmt::format("coro has no method '{}'", name));
        return Value();
    }
}
