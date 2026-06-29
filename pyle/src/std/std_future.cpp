#include "pyle/std/std_future.hpp"
#include "pyle/vm.hpp"
#include "pyle/binder.hpp"
#include <memory>

namespace pyle {

    Value Future::get_data(VM& vm) {
        if (!finished.load()) {
            return Value(); // Returns none if not finished
        }

        // Recreate the Pyle Value on the fly to avoid caching a heap reference
        if (!raw_string.empty()) {
            HeapIdx idx = vm.intern_string(raw_string);
            return Value(Value::Tag::StringRef, idx);
        } else if (!raw_bytes.empty()) {
            ArrayType arr;
            arr.reserve(raw_bytes.size());
            for (uint8_t b : raw_bytes) {
                arr.push_back(Value(static_cast<int64_t>(b)));
            }
            HeapIdx idx = vm.alloc(Object(std::move(arr)));
            return Value(Value::Tag::ArrayRef, idx);
        }
        return Value();
    }

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
            return (*sp)->get_data(vm); // Renamed from get_data to data
        });

        bind_getter("error", [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
            auto* sp = static_cast<std::shared_ptr<Future>*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
            return Value(Value::Tag::StringRef, vm.intern_string((*sp)->get_error())); // Renamed from get_error to error
        });

        auto ctor_wrapper = [](VM& vm, ArgView args) -> Value {
            auto* sp = new std::shared_ptr<Future>(std::make_shared<Future>());
            NativeObject ud;
            ud.ptr = sp;
            ud.deleter = [](void* p) { 
                delete static_cast<std::shared_ptr<Future>*>(p); 
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