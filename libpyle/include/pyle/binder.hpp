#pragma once
#include <type_traits>
#include <utility>
#include <string>
#include <vector>
#include <memory.h>
#include "pyle/vm.hpp"
#include "pyle/value.hpp"

namespace pyle {

    // Global metadata registry mapping for class templates
    template <typename T>
    struct BindRegistry {
        static inline HeapIdx type_idx = 0;
        static inline std::string class_name = "";
    };

    template <typename T>
    struct is_vector : std::false_type {};

    template <typename T, typename Alloc>
    struct is_vector<std::vector<T, Alloc>> : std::true_type {};

    template <typename T>
    inline constexpr bool is_vector_v = is_vector<T>::value;

    template <typename T>
    auto from_value(VM& vm, const Value& val) {
        using DecayedT = std::remove_cv_t<std::remove_reference_t<T>>;

        if constexpr (std::is_same_v<DecayedT, Value>) {
            return val;
        } else if constexpr (std::is_same_v<DecayedT, int64_t> || std::is_integral_v<DecayedT>) {
            if (val.tag != Value::Tag::Int) {
                vm.runtime_error(RuntimeError::Type, "Expected integer.");
                return static_cast<DecayedT>(0);
            }
            return static_cast<DecayedT>(val.as_int);
        } else if constexpr (std::is_floating_point_v<DecayedT>) {
            if (val.tag == Value::Tag::Float) return static_cast<DecayedT>(val.as_float);
            if (val.tag == Value::Tag::Int) return static_cast<DecayedT>(val.as_int);
            vm.runtime_error(RuntimeError::Type, "Expected number.");
            return static_cast<DecayedT>(0.0);
        } else if constexpr (std::is_same_v<DecayedT, std::string> || std::is_same_v<DecayedT, std::string_view>) {
            if (val.tag != Value::Tag::StringRef) {
                vm.runtime_error(RuntimeError::Type, "Expected string.");
                return std::string("");
            }
            return std::get<std::string>(vm.get_heap_object(val.as_ref).data);
        } else if constexpr (std::is_same_v<DecayedT, bool>) {
            return vm.is_truthy(val);
        } else if constexpr (std::is_pointer_v<DecayedT>) {
            if (val.tag != Value::Tag::NativeObjectRef) {
                vm.runtime_error(RuntimeError::Type, "Expected native object.");
                return static_cast<DecayedT>(nullptr);
            }
            auto& ud = std::get<NativeObject>(vm.get_heap_object(val.as_ref).data);
            return static_cast<DecayedT>(ud.ptr);
        } else if constexpr (is_vector_v<DecayedT>) {
            if (val.tag != Value::Tag::ArrayRef) {
                vm.runtime_error(RuntimeError::Type, "Expected array.");
                return DecayedT();
            }
            auto& arr = std::get<ArrayType>(vm.get_heap_object(val.as_ref).data);
            using ElementType = typename DecayedT::value_type;
            DecayedT result;
            result.reserve(arr.size());
            for (const auto& elem : arr) {
                result.push_back(from_value<ElementType>(vm, elem));
            }
            return result;
        } else {
            static_assert(sizeof(T) == 0, "Unsupported binding conversion type.");
        }
    }

    template <typename T>
    Value to_value(VM& vm, T val) {
        if constexpr (std::is_same_v<T, Value>) {
            return val;
        } else if constexpr (std::is_same_v<T, void>) {
            return Value();
        } else if constexpr (std::is_same_v<T, bool>) {
            return Value(val);
        } else if constexpr (std::is_same_v<T, int64_t> || std::is_integral_v<T>) {
            return Value(static_cast<int64_t>(val));
        } else if constexpr (std::is_floating_point_v<T>) {
            return Value(static_cast<double>(val));
        } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
            HeapIdx idx = vm.intern_string(val);
            return Value(Value::Tag::StringRef, idx);
        } else if constexpr (std::is_pointer_v<T>) {
            NativeObject ud;
            ud.ptr = const_cast<std::remove_const_t<std::remove_pointer_t<T>>*>(val);
            ud.deleter = nullptr;
            ud.type_idx = BindRegistry<std::remove_const_t<std::remove_pointer_t<T>>>::type_idx;
            HeapIdx idx = vm.alloc(Object(ud));
            return Value(Value::Tag::NativeObjectRef, idx);
        } else if constexpr (is_vector_v<T>) {
            using ElementType = typename T::value_type;
            ArrayType arr;
            arr.reserve(val.size());
            for (const auto& elem : val) {
                arr.push_back(to_value(vm, elem));
            }
            HeapIdx idx = vm.alloc(Object(std::move(arr)));
            return Value(Value::Tag::ArrayRef, idx);
        } else {
            static_assert(sizeof(T) == 0, "Unsupported return type.");
        }
    }

    template <typename T, typename = void>
    struct has_gc_mark : std::false_type {};

    template <typename T>
    struct has_gc_mark<T, std::void_t<decltype(std::declval<T>().gc_mark(std::declval<VM&>()))>> : std::true_type {};


    // recursive helper to unpack smart pointers at compile-time
    template <typename T, typename = void>
    struct gc_marker_helper {
        static void mark(void* p, VM& vm) {
            if constexpr (has_gc_mark<T>::value) {
                static_cast<T*>(p)->gc_mark(vm);
            }
        }
        static constexpr bool can_mark = has_gc_mark<T>::value;
    };

    // specialization for std::shared_ptr<U>
    template <typename U>
    struct gc_marker_helper<std::shared_ptr<U>> {
        static void mark(void* p, VM& vm) {
            auto* sp = static_cast<std::shared_ptr<U>*>(p);
            if (sp && *sp) {
                if constexpr (has_gc_mark<U>::value) {
                    (*sp)->gc_mark(vm); 
                }
            }
        }
        static constexpr bool can_mark = has_gc_mark<U>::value;
    };

    template <typename U>
    struct gc_marker_helper<std::unique_ptr<U>> {
        static void mark(void* p, VM& vm) {
            auto* up = static_cast<std::unique_ptr<U>*>(p);
            if (up && *up) {
                if constexpr (has_gc_mark<U>::value) {
                    (*up)->gc_mark(vm);
                }
            }
        }
        static constexpr bool can_mark = has_gc_mark<U>::value;
    };


    template <typename T>
    Value to_value_owned(VM& vm, T* val, void (*custom_marker)(void*, VM&) = nullptr) {
        NativeObject ud;
        ud.ptr = const_cast<std::remove_const_t<T>*>(val);
        
        ud.deleter = [](void* p) { delete static_cast<T*>(p); };
        
        if (custom_marker) {
            ud.marker = custom_marker;
        } else if constexpr (gc_marker_helper<T>::can_mark) {
            ud.marker = &gc_marker_helper<T>::mark; 
        } else {
            ud.marker = nullptr;
        }
        
        ud.type_idx = BindRegistry<std::remove_const_t<T>>::type_idx;
        
        HeapIdx idx = vm.alloc(Object(ud));
        return Value(Value::Tag::NativeObjectRef, idx);
    }

    template <auto MemFn, typename T = decltype(MemFn)>
    struct MethodDeducer;

    template <auto MemFn, typename Class, typename Ret, typename... Args>
    struct MethodDeducer<MemFn, Ret (Class::*)(Args...)> {
        template <size_t... Is>
        static Value invoke(VM& vm, Class* instance, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    (instance->*MemFn)(from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = (instance->*MemFn)(from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, HeapIdx obj_idx, ArgView args) {
            Object& obj = vm.get_heap_object(obj_idx);
            auto* instance = static_cast<Class*>(std::get<NativeObject>(obj.data).ptr);
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, instance, args, std::make_index_sequence<param_count>{});
        }
    };

    template <auto MemFn, typename Class, typename Ret, typename... Args>
    struct MethodDeducer<MemFn, Ret (Class::*)(VM&, Args...)> {
        template <size_t... Is>
        static Value invoke(VM& vm, Class* instance, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    (instance->*MemFn)(vm, from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = (instance->*MemFn)(vm, from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, HeapIdx obj_idx, ArgView args) {
            Object& obj = vm.get_heap_object(obj_idx);
            auto* instance = static_cast<Class*>(std::get<NativeObject>(obj.data).ptr);
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, instance, args, std::make_index_sequence<param_count>{});
        }
    };

    template <auto MemFn, typename Class, typename Ret, typename... Args>
    struct MethodDeducer<MemFn, Ret (Class::*)(Args...) const> {
        template <size_t... Is>
        static Value invoke(VM& vm, const Class* instance, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    (instance->*MemFn)(from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = (instance->*MemFn)(from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, HeapIdx obj_idx, ArgView args) {
            Object& obj = vm.get_heap_object(obj_idx);
            const auto* instance = static_cast<const Class*>(std::get<NativeObject>(obj.data).ptr);
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, instance, args, std::make_index_sequence<param_count>{});
        }
    };

    template <auto MemFn, typename Class, typename Ret, typename... Args>
    struct MethodDeducer<MemFn, Ret (Class::*)(VM&, Args...) const> {
        template <size_t... Is>
        static Value invoke(VM& vm, const Class* instance, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    (instance->*MemFn)(vm, from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = (instance->*MemFn)(vm, from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, HeapIdx obj_idx, ArgView args) {
            Object& obj = vm.get_heap_object(obj_idx);
            const auto* instance = static_cast<const Class*>(std::get<NativeObject>(obj.data).ptr);
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, instance, args, std::make_index_sequence<param_count>{});
        }
    };

    template <auto Fn, typename T = decltype(Fn)>
    struct FreeFnDeducer;

    template <auto Fn, typename Ret, typename... Args>
    struct FreeFnDeducer<Fn, Ret (*)(Args...)> {
        template <size_t... Is>
        static Value invoke(VM& vm, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    Fn(from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = Fn(from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, ArgView args) {
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, args, std::make_index_sequence<param_count>{});
        }
    };

    template <auto Fn, typename Ret, typename... Args>
    struct FreeFnDeducer<Fn, Ret (*)(VM&, Args...)> {
        template <size_t... Is>
        static Value invoke(VM& vm, ArgView args, std::index_sequence<Is...>) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    Fn(vm, from_value<Args>(vm, args[Is])...);
                    return Value();
                } else {
                    auto result = Fn(vm, from_value<Args>(vm, args[Is])...);
                    return to_value(vm, result);
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }

        static Value wrap(VM& vm, ArgView args) {
            constexpr size_t param_count = sizeof...(Args);
            if (args.size() != param_count) {
                vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", param_count, args.size()));
                return Value();
            }
            return invoke(vm, args, std::make_index_sequence<param_count>{});
        }
    };

    template <typename T, typename... Args, size_t... Is>
    T* invoke_constructor_helper(VM& vm, ArgView args, std::index_sequence<Is...>) {
        try {
            return new T(from_value<Args>(vm, args[Is])...);
        } catch (const std::exception& e) {
            vm.runtime_error(RuntimeError::Runtime, e.what());
            return nullptr;
        }
    }

    template <auto Fn, typename Ret>
    struct FreeFnDeducer<Fn, Ret (*)(VM&, ArgView)> {
        static Value wrap(VM& vm, ArgView args) {
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    Fn(vm, args);
                    return Value();
                } else {
                    return to_value(vm, Fn(vm, args));
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }
    };

    template <auto MemFn, typename Class, typename Ret>
    struct MethodDeducer<MemFn, Ret (Class::*)(VM&, ArgView)> {
        static Value wrap(VM& vm, HeapIdx obj_idx, ArgView args) {
            Object& obj = vm.get_heap_object(obj_idx);
            if (!std::holds_alternative<NativeObject>(obj.data)) {
                vm.runtime_error(RuntimeError::Type, "Expected native object instance.");
                return Value();
            }
            auto* instance = static_cast<Class*>(std::get<NativeObject>(obj.data).ptr);
            try {
                if constexpr (std::is_same_v<Ret, void>) {
                    (instance->*MemFn)(vm, args);
                    return Value();
                } else {
                    return to_value(vm, (instance->*MemFn)(vm, args));
                }
            } catch (const std::exception& e) {
                vm.runtime_error(RuntimeError::Runtime, e.what());
                return Value();
            }
        }
    };

    template <typename T>
    class ClassBinder {
        VM& vm;
        Value type_val; 
        bool was_gc_enabled = true;

    public:
        ClassBinder(VM& vm, const std::string& name) : vm(vm) {
            was_gc_enabled = vm.is_gc_enabled();
            vm.set_gc_enabled(false); 
            BindRegistry<T>::class_name = name;
            
            StructType type_meta;
            BindRegistry<T>::type_idx = vm.alloc(Object(type_meta));
            
            type_val = Value(Value::Tag::StructTypeRef, BindRegistry<T>::type_idx);
        }

        ~ClassBinder() {
            vm.set_gc_enabled(was_gc_enabled); 
        }
    

        template <typename... Args>
        ClassBinder& constructor() {
            NativeFn ctor_wrapper = [](VM& vm, ArgView args) -> Value {
                constexpr size_t param_count = sizeof...(Args);
                if (args.size() != param_count) {
                    vm.runtime_error(RuntimeError::ArgumentError, fmt::format("Constructor expected {} arguments, got {}.", param_count, args.size()));
                    return Value();
                }

                T* instance = invoke_constructor_helper<T, Args...>(vm, args, std::make_index_sequence<param_count>{});
                if (!instance) return Value();

                return to_value_owned(vm, instance);
            };

            HeapIdx ctor_idx = vm.alloc(Object(ctor_wrapper));
            
            auto& meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            meta.native_constructor_idx = ctor_idx;

            return *this;
        }

        template <auto Fn>
        ClassBinder& static_method(const std::string& name) {
            NativeMethodFn wrapped = [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
                return FreeFnDeducer<Fn>::wrap(vm, args);
            };

            HeapIdx method_idx = vm.alloc(Object(NativeMethod{wrapped}));
            HeapIdx name_id = vm.intern_string(name);

            auto& meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            meta.methods[name_id] = method_idx;

            return *this;
        }

        ClassBinder& register_globally() {
            int slot = vm.declare_global(vm.intern_string(BindRegistry<T>::class_name));
            vm.global_slots[slot] = type_val; 
            return *this;
        }

        Value get_constructor() const { return type_val; }

        template <auto MemFn>
        ClassBinder& method(const std::string& name) {
            NativeMethodFn wrapped = MethodDeducer<MemFn>::wrap;
            
            HeapIdx method_idx = vm.alloc(Object(NativeMethod{wrapped}));
            HeapIdx name_id = vm.intern_string(name);
            
            auto& registered_meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            registered_meta.methods[name_id] = method_idx;
            
            return *this;
        }

        template <typename FieldType, FieldType T::*FieldPtr>
        ClassBinder& member(const std::string& name) {

            NativeMethodFn getter = [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
                auto* inst = static_cast<T*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
                return to_value(vm, inst->*FieldPtr);
            };

            NativeMethodFn setter = [](VM& vm, HeapIdx obj_idx, ArgView args) -> Value {
                auto* inst = static_cast<T*>(std::get<NativeObject>(vm.get_heap_object(obj_idx).data).ptr);
                inst->*FieldPtr = from_value<FieldType>(vm, args[0]);
                return args[0];
            };

            HeapIdx getter_idx = vm.alloc(Object(NativeMethod{getter}));
            HeapIdx setter_idx = vm.alloc(Object(NativeMethod{setter}));
            HeapIdx name_id = vm.intern_string(name);

            auto& registered_meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            
            registered_meta.getters[name_id] = getter_idx; 
            registered_meta.setters[name_id] = setter_idx;

            return *this;
        }

        ClassBinder& custom_constructor(NativeFn custom_ctor) {
            HeapIdx ctor_idx = vm.alloc(Object(custom_ctor));
            
            auto& meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            meta.native_constructor_idx = ctor_idx;

            return *this;
        }

        ClassBinder& custom_method(const std::string& name, NativeMethodFn custom_fn) {
            HeapIdx method_idx = vm.alloc(Object(NativeMethod{custom_fn}));
            HeapIdx name_id = vm.intern_string(name);

            auto& registered_meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            registered_meta.methods[name_id] = method_idx;

            return *this;
        }

        ClassBinder& custom_getter(const std::string& name, NativeMethodFn getter_fn) {
            HeapIdx getter_idx = vm.alloc(Object(NativeMethod{getter_fn}));
            HeapIdx name_id = vm.intern_string(name);

            auto& registered_meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            registered_meta.getters[name_id] = getter_idx;

            return *this;
        }

        ClassBinder& custom_setter(const std::string& name, NativeMethodFn setter_fn) {
            HeapIdx setter_idx = vm.alloc(Object(NativeMethod{setter_fn}));
            HeapIdx name_id = vm.intern_string(name);

            auto& registered_meta = std::get<StructType>(vm.get_heap_object(BindRegistry<T>::type_idx).data);
            registered_meta.setters[name_id] = setter_idx;

            return *this;
        }
    };

    template <auto Fn>
    void bind_function(VM& vm, const std::string& name) {
        NativeFn wrapped = FreeFnDeducer<Fn>::wrap;
        vm.define_native(name, wrapped);
    }

    class NativeModule {
        VM& vm;
        std::string name;
        MapType exports;

    public:
        NativeModule(VM& vm, const std::string& name) : vm(vm), name(name) {}

        NativeModule& raw_function(const std::string& func_name, NativeFn raw_fn) {
            HeapIdx fn_idx = vm.alloc(Object(raw_fn));
            Value key(Value::Tag::StringRef, vm.intern_string(func_name));
            exports[key] = Value(Value::Tag::NativeFuncRef, fn_idx);
            return *this;
        }

        template <auto Fn>
        NativeModule& function(const std::string& func_name) {
            NativeFn wrapped = FreeFnDeducer<Fn>::wrap;
            HeapIdx fn_idx = vm.alloc(Object(wrapped));
            Value key(Value::Tag::StringRef, vm.intern_string(func_name));
            exports[key] = Value(Value::Tag::NativeFuncRef, fn_idx);
            return *this;
        }

        template <typename T>
        NativeModule& class_binder(ClassBinder<T>& binder) {
            Value ctor = binder.get_constructor();
            Value key(Value::Tag::StringRef, vm.intern_string(BindRegistry<T>::class_name));
            exports[key] = ctor;
            return *this;
        }

        Value build() {
            HeapIdx map_idx = vm.alloc(Object(std::move(exports)));
            return Value(Value::Tag::MapRef, map_idx);
        }
    };

    inline void register_module(VM& vm, const std::string& name, ModuleFactory factory) {
        HeapIdx name_id = vm.intern_string(name);
        vm.module_registry[name_id] = factory;
    }
}