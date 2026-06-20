#pragma once
#include "pyle/bytecode.hpp"
#include "pyle/value.hpp"
#include <ankerl/unordered_dense.h>
#include <vector>
#include "unordered_set"

namespace pyle {

    enum class RuntimeError {
        Type, Name, Index, ZeroDivision, StackUnderflow, OutOfBounds, ArgumentError, Runtime
    };

    inline std::string_view err_to_string(const RuntimeError& err) {
        switch (err) {
            case RuntimeError::Type: return "TypeError";
            case RuntimeError::Name: return "NameError";
            case RuntimeError::Index: return "IndexError";
            case RuntimeError::StackUnderflow: return "StackUnderFlowError";
            case RuntimeError::ArgumentError: return "ArgumentError";
            case RuntimeError::OutOfBounds: return "OutOfBoundsError";
            default: return "RuntimeError";
        }
    }

    using MethodFn = Value (*)(VM& vm, HeapIdx obj_idx, ArgView args);

    class VM {
    public:
        struct CallFrame {
            HeapIdx function;
            size_t ip;
            size_t stack_base;
        };

        std::vector<Value> eval_stack;
        std::vector<CallFrame> frames;

        std::vector<Value> global_slots;
        ankerl::unordered_dense::map<std::string, int> global_slot_map;

        HeapIdx alloc(Object obj);
        HeapIdx intern_string(std::string_view str);

        void gc_collect_now() { gc_collect(); } 
        void set_gc_enabled(bool enabled) { gc_enabled = enabled; }
        bool is_gc_enabled() const { return gc_enabled; }
        bool is_panicked() const { return panicked; }

        Object& get_heap_object(const HeapIdx idx) { return heap[idx]; }

        void execute(Chunk in_chunk);
        bool values_equal(const Value& a, const Value& b);
        std::string value_to_string(const Value& val);
        void define_native(const std::string& name, NativeFn function);
        void runtime_error(const RuntimeError& err, const std::string& msg);

        const auto& get_interned_strings() const { return interned_strings; }

        int declare_global(const std::string& name);

        bool is_truthy(const Value& v);

        VM() {
            eval_stack.reserve(8192);
            frames.reserve(1024);
        }

    private:
        bool gc_enabled = true;

        // MEMORY
        void gc_sweep();
        void gc_mark();
        void gc_collect();
        
        std::vector<Object> heap;
        std::vector<HeapIdx> free_list;



        struct StringHash {
            using is_transparent = void;
            auto operator()(std::string_view str) const noexcept -> uint64_t {
                return ankerl::unordered_dense::hash<std::string_view>{}(str);
            }
        };
        ankerl::unordered_dense::map<std::string, HeapIdx, StringHash, std::equal_to<>> interned_strings;

        static constexpr size_t INITIAL_THRESHOLD = 256;
        size_t gc_threshold = INITIAL_THRESHOLD;

        bool panicked = false;

        inline void push(Value value) { eval_stack.push_back(value); }
        inline Value pop() {
            Value val = eval_stack.back();
            eval_stack.pop_back();
            return val;
        }
        void value_to_string_helper(const Value& val, std::unordered_set<HeapIdx>& visited, std::stringstream& ss);
   
        Function& get_function(const CallFrame& frame) {
            return std::get<Function>(heap[frame.function].data);
        }
    };
}
