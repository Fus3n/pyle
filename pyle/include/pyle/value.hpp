#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <cassert>
#include <stdexcept>
#include <fmt/format.h>
#include "ankerl/unordered_dense.h"
#include "pyle/bytecode.hpp"
#include <array>       
#include <optional>   

namespace pyle {

    class VM;
    using HeapIdx = size_t;
    struct Coroutine;

    struct CallFrame {
        HeapIdx closure;
        size_t ip;
        size_t stack_base;
    };

    struct Value {
        enum class Tag {
            Int, Float, Bool, None, StringRef, ArrayRef, StructRef,
            NativeFuncRef, FuncRef, IteratorRef, RangeRef, ClosureRef, UpvalueRef,
            StructTypeRef, MapRef, NativeObjectRef, CoroutineRef,
        } tag;
        union {
            int64_t as_int;
            double as_float;
            bool as_bool;
            HeapIdx as_ref;
        };

        Value() : tag(Tag::None), as_ref(0) {}

        explicit Value(int64_t val) : tag(Tag::Int), as_int(val) {}

        explicit Value(double val) : tag(Tag::Float), as_float(val) {}

        explicit Value(bool val) : tag(Tag::Bool), as_bool(val) {}

        Value(Tag t, HeapIdx ref) : tag(t), as_ref(ref) {
            assert(t == Tag::StringRef ||
                   t == Tag::ArrayRef ||
                   t == Tag::StructRef ||
                   t == Tag::StructTypeRef ||
                   t == Tag::NativeFuncRef ||
                   t == Tag::FuncRef ||
                   t == Tag::IteratorRef ||
                   t == Tag::RangeRef ||
                   t == Tag::ClosureRef ||
                   t == Tag::MapRef ||
                   t == Tag::NativeObjectRef ||
                   t == Tag::CoroutineRef);
        }

        std::string tag_to_string() const {
            switch (tag) {
                case Tag::Int: return "int";
                case Tag::Float: return "float";
                case Tag::Bool: return "bool";
                case Tag::None: return "none";
                case Tag::StringRef: return "string";
                case Tag::ArrayRef: return "array";
                case Tag::StructRef: return "struct";
                case Tag::NativeFuncRef: return "native_function";
                case Tag::FuncRef: return "function";
                case Tag::IteratorRef: return "iterator";
                case Tag::RangeRef: return "range";
                case Tag::MapRef: return "map";
                case Tag::NativeObjectRef: return "native_object"; 
                case Tag::CoroutineRef: return "coro";
                default:
                    return fmt::format("HeapRef({})", as_ref);
            }
        }

        bool operator==(const Value& other) const {
            if (tag != other.tag) {
                // Promote and compare if we are comparing an Int against a Float
                if ((tag == Tag::Int || tag == Tag::Float) &&
                    (other.tag == Tag::Int || other.tag == Tag::Float)) {
                    double da = (tag == Tag::Int) ? static_cast<double>(as_int) : as_float;
                    double db = (other.tag == Tag::Int) ? static_cast<double>(other.as_int) : other.as_float;
                    return da == db;
                }
                return false;
            }
            switch (tag) {
                case Tag::None: return true; 
                case Tag::Bool: return as_bool == other.as_bool;
                case Tag::Int: return as_int == other.as_int;
                case Tag::Float: return as_float == other.as_float;
                default: return as_ref == other.as_ref; 
            }
        }

        bool operator!=(const Value& other) const {
            return !(*this == other);
        }
    };

    struct ValueEqual {
        bool operator()(const Value& a, const Value& b) const {
            return a == b; 
        }
    };

      struct ValueHash {
        uint64_t operator()(const Value& v) const {
            uint64_t hash = static_cast<uint64_t>(v.tag);
            switch (v.tag) {
                case Value::Tag::Int: hash ^= ankerl::unordered_dense::hash<int64_t>{}(v.as_int); break;
                case Value::Tag::Float: hash ^= ankerl::unordered_dense::hash<double>{}(v.as_float); break;
                case Value::Tag::Bool: hash ^= ankerl::unordered_dense::hash<bool>{}(v.as_bool); break;
                case Value::Tag::None: break;
                default: hash ^= ankerl::unordered_dense::hash<size_t>{}(v.as_ref); break;
            }
            return hash;
        }
    };

    struct ArgView {
        const Value* data_ptr;
        size_t count;

        [[nodiscard]] const Value* begin() const { return data_ptr; }
        [[nodiscard]] const Value* end() const { return data_ptr + count; }
        [[nodiscard]] size_t size() const { return count; }

        const Value& operator[](size_t index) const {
            assert(index < count && "ArgView index out of bounds.");
            return data_ptr[index];
        }

        [[nodiscard]] const Value& at(size_t index) const {
            if (index >= count) throw std::out_of_range("ArgView index out of bounds.");
            return data_ptr[index];
        }
    };

    using ArrayType = std::vector<Value>;
    using NativeFn = Value (*)(VM& vm, ArgView args);
    using NativeMethodFn = Value (*)(VM& vm, HeapIdx obj_idx, ArgView args); // native function definition signature
    using MapType = ankerl::unordered_dense::map<Value, Value, ValueHash, ValueEqual>;


    enum class SpecialMethod : size_t {
        Init,  
        Str,    
        Add,    
        Sub,
        Mul,
        Div,
        Eq,   
        Count 
    };

    inline std::optional<SpecialMethod> get_special_method_enum(std::string_view name) {
        if (name == "_init") return SpecialMethod::Init;
        if (name == "_str")  return SpecialMethod::Str;
        if (name == "_add")  return SpecialMethod::Add;
        if (name == "_sub")  return SpecialMethod::Sub;
        if (name == "_mul")  return SpecialMethod::Mul;
        if (name == "_div")  return SpecialMethod::Div;
        if (name == "_eq")   return SpecialMethod::Eq;
        return std::nullopt;
    }


    struct StructType {
        std::vector<HeapIdx> field_names; 
        ankerl::unordered_dense::map<HeapIdx, size_t> field_map; 
        ankerl::unordered_dense::map<HeapIdx, HeapIdx> methods; 
        
        std::array<HeapIdx, static_cast<size_t>(SpecialMethod::Count)> special_methods{};

        ankerl::unordered_dense::map<HeapIdx, HeapIdx> setters;

        size_t get_offset(HeapIdx field_id) const {
            if (field_names.size() <= 8) {
                for (size_t i = 0; i < field_names.size(); ++i) {
                    if (field_names[i] == field_id) return i;
                }
                return size_t(-1);
            }
            auto it = field_map.find(field_id);
            if (it != field_map.end()) return it->second;
            return size_t(-1);
        }
    };

    struct Struct {
        HeapIdx type_idx;         
        std::vector<Value> fields; 
    };

    struct Upvalue {
        Value* location = nullptr; 
        Value closed;              
    };

    struct Function {
        std::string name;
        int arity = 0;
        Chunk chunk;

        struct UpvalueInfo {
            uint8_t index;
            bool is_local;
        };

        std::vector<UpvalueInfo> upvalues;
    };

    struct Iterator {
        Value container; 
        size_t index = 0;
    };

    struct Range {
        int64_t start;
        int64_t end;
    };

    struct Closure {
        HeapIdx function; 
        std::vector<HeapIdx> upvalues; 
    };

    struct NativeObject {
        void* ptr = nullptr;
        void (*deleter)(void*) = nullptr; 
        HeapIdx type_idx = 0;
    };

    struct NativeMethod {
        NativeMethodFn fn = nullptr;
    };
    
    struct Coroutine {
        Value* stack = nullptr;
        Value* sp = nullptr;
        size_t stack_capacity = 0;

        CallFrame* frames = nullptr;
        size_t frame_count = 0;
        size_t frame_capacity = 0;

        HeapIdx closure_idx = 0;       
        HeapIdx caller_idx = 0;   
        HeapIdx self_idx = 0;          

        enum class State {
            Suspended,
            Running,
            Dead
        } state = State::Suspended;

        bool is_main = false;
        bool started = false; 

        Coroutine() = default;

        ~Coroutine() {
            cleanup();
        }

        Coroutine(const Coroutine& other) = delete;
        Coroutine& operator=(const Coroutine& other) = delete;

        Coroutine(Coroutine&& other) noexcept {
            move_from(std::move(other));
        }

        Coroutine& operator=(Coroutine&& other) noexcept {
            if (this != &other) {
                cleanup();
                move_from(std::move(other));
            }
            return *this;
        }

    private:
        void cleanup() {
            if (!is_main) {
                delete[] stack;
                delete[] frames;
            }
            stack = nullptr;
            frames = nullptr;
            sp = nullptr;
            stack_capacity = 0;
            frame_capacity = 0;
            frame_count = 0;
        }


        void move_from(Coroutine&& other) noexcept {
            is_main = other.is_main;
            state = other.state;
            closure_idx = other.closure_idx;
            caller_idx = other.caller_idx;
            self_idx = other.self_idx;
            stack_capacity = other.stack_capacity;
            frame_capacity = other.frame_capacity;
            frame_count = other.frame_count;
            started = other.started;

            stack = other.stack;
            sp = other.sp;
            frames = other.frames;

            other.stack = nullptr;
            other.sp = nullptr;
            other.frames = nullptr;
            other.stack_capacity = 0;
            other.frame_capacity = 0;
            other.frame_count = 0;
        }
    };

    struct Object {
        bool gc_marked = false;
        std::variant<
            std::monostate,
            std::string,
            ArrayType,
            StructType,
            Struct,
            NativeFn,
            Function,
            Iterator,
            Range,
            Closure,
            Upvalue,
            MapType,
            NativeObject,
            NativeMethod,
            Coroutine
        > data;

        Object() = default;
        explicit Object(std::string str) : data(std::move(str)) {}
        explicit Object(ArrayType arr)   : data(std::move(arr)) {}
        explicit Object(NativeFn fn)   : data(fn) {}
        explicit Object(Function func): data(std::move(func)) {}
        explicit Object(Iterator iter): data(iter) {}
        explicit Object(Range r): data(r) {}
        explicit Object(Closure clos) : data(clos) {} 
        explicit Object(Upvalue uv)   : data(uv) {}
        explicit Object(StructType strt) : data(std::move(strt)) {} 
        explicit Object(Struct strc)     : data(std::move(strc)) {}
        explicit Object(MapType m): data(std::move(m)) {}
        explicit Object(NativeObject u) : data(u) {}
        explicit Object(NativeMethod nm) : data(nm) {}           
        explicit Object(Coroutine coro) : data(std::move(coro)) {} 
    };

}
