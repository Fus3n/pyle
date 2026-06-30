#include "pyle/std/std_future.hpp"
#include "pyle/vm.hpp"
#include "pyle/binder.hpp"
#include <memory>

namespace pyle {
    void register_core_future(VM& vm) {
        StructType type_meta;
        HeapIdx type_idx = vm.alloc(Object(type_meta));
        BindRegistry<std::shared_ptr<Future>>::type_idx = type_idx;

        auto bind_getter = [&](const std::string& name, NativeMethodFn fn) {
            HeapIdx getter_idx = vm.alloc(Object(NativeMethod{fn}));
            HeapIdx name_id = vm.intern_string(name);
            auto& meta = std::get<StructType>(vm.get_heap_object(type_idx).data);
            
            meta.getters[name_id] = getter_idx; 
        };

        auto bind_method = [&](const std::string& name, NativeMethodFn fn) {
            HeapIdx method_idx = vm.alloc(Object(NativeMethod{fn}));
            HeapIdx name_id = vm.intern_string(name);
            auto& meta = std::get<StructType>(vm.get_heap_object(type_idx).data);
            meta.methods[name_id] = method_idx; 
        };

        bind_getter("is_done", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return Value((*sp)->is_done());
        });

        bind_getter("has_failed", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return Value((*sp)->has_failed());
        });

        bind_getter("data", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return (*sp)->get_data(vm);
        });

        bind_getter("error", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return Value(Value::Tag::StringRef, vm.intern_string((*sp)->get_error())); // Renamed from get_error to error
        });

        bind_method("resolve", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            (*sp)->raw_value = args[0];
            (*sp)->finished.store(true);
            return Value();
        });

        bind_method("reject", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            (*sp)->error = pyle::from_value<std::string>(vm, args[0]);
            (*sp)->failed = true;
            (*sp)->finished.store(true);
            return Value();
        });

        auto ctor_wrapper = [](VM& vm, ArgView args) -> Value {
            auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
            NativeObject ud;
            ud.ptr = sp;
            
            ud.deleter = [](void* p) { 
                delete static_cast<std::shared_ptr<Future>*>(p); 
            };
            
            ud.marker = [](void* p, VM& vm) {
                auto* sp = static_cast<std::shared_ptr<Future>*>(p);
                vm.mark_value((*sp)->raw_value); 
            };
            
            ud.type_idx = BindRegistry<std::shared_ptr<Future>>::type_idx;
            
            HeapIdx idx = vm.alloc(Object(ud));
            return Value(Value::Tag::NativeObjectRef, idx);
        };

        HeapIdx ctor_idx = vm.alloc(Object(pyle::NativeFn(ctor_wrapper)));
        Value ctor_val(Value::Tag::NativeFuncRef, ctor_idx);

        int slot = vm.declare_global(vm.intern_string("Future"));
        vm.global_slots[slot] = ctor_val;
    }
}