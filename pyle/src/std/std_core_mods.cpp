#include "pyle/std/std_core_mods.hpp"
#include "pyle/binder.hpp"
#include "pyle/value.hpp"
#include <chrono>


namespace pyle {


    // OS MODULE
    Value os_time() {
        auto now = std::chrono::high_resolution_clock::now();
        auto duration = now.time_since_epoch();
        double seconds = std::chrono::duration_cast<std::chrono::duration<double>>(duration).count();
        return Value(seconds);
    }
    
    Value os_sys(VM& vm, ArgView args) {
        if (args.size() != 1 || args[0].tag != Value::Tag::StringRef) {
            vm.runtime_error(RuntimeError::ArgumentError, "os.system expects 1 string argument.");
            return Value();
        }
        std::string cmd = std::get<std::string>(vm.get_heap_object(args[0].as_ref).data);
        int result = std::system(cmd.c_str());
        return Value(static_cast<int64_t>(result));
    }

    Value os_module_factory(VM& vm) {
        return NativeModule(vm, "os")
            .raw_function("system", os_sys)
            .function<os_time>("time")
            .build();
    }
    
    
    void register_core_modules(VM& vm) {
        pyle::register_module(vm, "os", os_module_factory);
    }
}