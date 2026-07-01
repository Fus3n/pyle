#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"
#include "pyle/binder.hpp"
#include "pyle/std/std_future.hpp"


pyle::Value fetch_data_async(pyle::VM& vm, pyle::ArgView args) {
    auto* sp = new std::shared_ptr<pyle::Future>(std::make_shared<pyle::Future>());

    std::thread([sp_copy = *sp, &vm]() {
        // Do slow database query / web request here ...

        std::string database_result = "result";
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        {
            std::lock_guard<std::recursive_mutex> lock(vm.get_mutex());
            sp_copy->raw_value = pyle::Value(pyle::Value::Tag::StringRef, vm.intern_string(database_result));
            sp_copy->finished.store(true); 
        }
    }).detach();

    return to_value_owned(vm, sp); 
}

int main() {
    pyle::Pyle interpreter;
    pyle::register_core_natives(interpreter.vm);

    pyle::bind_function<fetch_data_async>(interpreter.vm, "fetch_data");

    std::string code = R"(
        fn main() {
            print("Starting C++ async fetch...")
            let result = waitfor(fetch_data())
            printf("Finished script! Result: {}", result)
        }
        
        # Bootstrap our main coroutine onto the task scheduler!
        async.run(main)
    )";

    std::cout << "Running Pyle Async Binding\n\n";

    interpreter.execute(code, false, "class_binding.pyl");

    std::cout << "\n--- Triggering Manual Garbage Collection Sweep ---\n";
    interpreter.vm.gc_collect_now();

    std::cout << "\nClass Binding Example Finished\n";
    return 0;
}