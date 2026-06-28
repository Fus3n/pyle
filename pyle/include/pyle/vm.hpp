#pragma once
#include "pyle/bytecode.hpp"
#include "pyle/value.hpp"
#include <ankerl/unordered_dense.h>
#include <vector>
#include "unordered_set"
#include "pyle/platform.hpp"
#include "pyle/error_reporter.hpp"

namespace pyle {

    using ModuleFactory = std::function<Value(VM& vm)>;

    struct VMConfig {
        size_t stack_capacity = 8192;
        size_t frame_capacity = 2048;
        bool gc_enabled = true;
    };

    class VM {
    public:
        std::string_view source_code;
        std::string_view script_name = "main.pyl";
        

        Value* stack = nullptr;
        Value* sp = nullptr;
        Value* stack_end = nullptr;
        size_t stack_capacity = 0;
        void grow_stack();

        CallFrame* frames = nullptr;
        size_t frame_count = 0;
        size_t frame_capacity = 0;

        std::vector<HeapIdx> open_upvalues; 

        HeapIdx capture_upvalue(size_t stack_index);
        void close_upvalues(Value* limit);

        std::vector<Value> global_slots;
        ankerl::unordered_dense::map<HeapIdx, int> global_slot_map;

        HeapIdx alloc(Object obj);
        HeapIdx intern_string(std::string_view str);

        void gc_collect_now() { gc_collect(); } 
        void set_gc_enabled(bool enabled) { gc_enabled = enabled; }
        bool is_gc_enabled() const { return gc_enabled; }
        bool is_panicked() const { return panicked; }

        Object& get_heap_object(const HeapIdx idx) { return heap[idx]; }

        void execute(Chunk in_chunk);
        std::string value_to_string(const Value& val);
        void define_native(const std::string& name, NativeFn function);
        void runtime_error(const RuntimeError& err, const std::string& msg);

        const auto& get_interned_strings() const { return interned_strings; }

        int declare_global(HeapIdx name_idx);

        bool is_truthy(const Value& v);

        inline bool is_hashable(const Value& v) const {
            return v.tag != Value::Tag::ArrayRef &&
                v.tag != Value::Tag::MapRef &&
                v.tag != Value::Tag::StructRef;
        }

        explicit VM(const VMConfig& config = VMConfig()) {
            stack_capacity = config.stack_capacity;
            stack = new Value[stack_capacity];
            sp = stack;
            stack_end = stack + stack_capacity;

            frame_capacity = config.frame_capacity;
            frames = new CallFrame[frame_capacity];
            frame_count = 0;
            
            set_gc_enabled(config.gc_enabled);
        }

        ~VM() {
            delete[] stack;
            delete [] frames;

            for (auto& obj : heap) {
                if (auto* ud = std::get_if<NativeObject>(&obj.data)) {
                    if (ud->deleter && ud->ptr) {
                        ud->deleter(ud->ptr);
                    }
                }
            }
        }

        ankerl::unordered_dense::map<HeapIdx, ModuleFactory> module_registry;
        ankerl::unordered_dense::map<HeapIdx, Value> loaded_modules;

        size_t builtin_count = 0;
        bool builtins_finalized = false;
        std::vector<std::vector<Value>> saved_globals_stack;

        ankerl::unordered_dense::map<HeapIdx, int> builtin_slot_map;
        std::vector<ankerl::unordered_dense::map<HeapIdx, int>> saved_slot_maps_stack;


        HeapIdx active_coroutine_idx = 0;
        HeapIdx main_coroutine_idx = 0;
        bool coro_switched = false;

        inline void save_coroutine_state(Coroutine& coro) {
            coro.stack = this->stack;
            coro.sp = this->sp;
            coro.stack_capacity = this->stack_capacity;
            coro.frames = this->frames;
            coro.frame_count = this->frame_count;
            coro.frame_capacity = this->frame_capacity;
        }

        inline void load_coroutine_state(Coroutine& coro) {
            this->stack = coro.stack;
            this->sp = coro.sp;
            this->stack_capacity = coro.stack_capacity;
            this->frames = coro.frames;
            this->frame_count = coro.frame_count;
            this->frame_capacity = coro.frame_capacity;
            this->stack_end = this->stack + this->stack_capacity;
        }

        void init_root_coroutine();

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

        inline void push(Value value) { 
            if (sp == stack_end) {
                grow_stack();
            }
            *sp++ = value;
        }

        PYLE_FORCEINLINE Value pop() { return *(--sp); }
        PYLE_FORCEINLINE size_t stack_size() const { return sp - stack; }
        PYLE_FORCEINLINE Value peek(size_t distance = 1) const { return *(sp - distance); }
        PYLE_FORCEINLINE void set_top(Value val) { *(sp - 1) = val;}

        void value_to_string_helper(const Value& val, std::unordered_set<HeapIdx>& visited, std::stringstream& ss);
        
        PYLE_FORCEINLINE Function& get_func_from_frame(const CallFrame& frame) {
            Closure& closure = std::get<Closure>(heap[frame.closure].data);
            return std::get<Function>(
                heap[closure.function].data
            );
        }

        HeapIdx build_closure_for_call(HeapIdx fn_idx, CallFrame* caller_frame);
        PYLE_FORCEINLINE bool instantiate_struct(HeapIdx struct_type_idx, int arg_count, CallFrame* current_frame);
    };
}
