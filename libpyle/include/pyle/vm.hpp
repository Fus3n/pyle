#pragma once
#include <ankerl/unordered_dense.h>
#include <cstdlib>
#include <vector>
#include <deque>
#include <mutex> 
#include "pyle/bytecode.hpp"
#include "pyle/value.hpp"
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
        std::string executable_path;
        bool is_worker = false;

        Value* stack = nullptr;
        Value* sp = nullptr;
        Value* stack_end = nullptr;
        size_t stack_capacity = 0;
        /// Grows the value stack when full.
        void grow_stack();

        CallFrame* frames = nullptr;
        size_t frame_count = 0;
        size_t frame_capacity = 0;

        std::vector<HeapIdx> open_upvalues;

        /// Captures a stack local as an upvalue for a closure.
        HeapIdx capture_upvalue(size_t stack_index);
        /// Closes all open upvalues at or above the given stack limit.
        void close_upvalues(Value* limit);

        // Root (top-level script) global storage. `global_slots` is a pointer to
        // the *currently active* global storage: either this root, or a module's
        // own persistent storage while a module function is running.
        std::vector<Value> root_globals;
        std::vector<Value>* global_slots = nullptr;
        HeapIdx globals_idx = HeapIdx(-1);
        ankerl::unordered_dense::map<HeapIdx, int> global_slot_map;

        /// Allocates a new heap object and returns its index.
        HeapIdx alloc(Object obj);
        /// Allocates a permanent heap object that is never collected by GC.
        HeapIdx alloc_permanent(Object obj);
        /// Returns the canonical interned index for a string, creating it if needed.
        HeapIdx intern_string(std::string_view str);

        /// Forces an immediate garbage collection cycle.
        void gc_collect_now() { gc_collect(); }
        /// Enables or disables automatic garbage collection.
        void set_gc_enabled(bool enabled) { gc_enabled = enabled; }
        bool is_gc_enabled() const { return gc_enabled; }
        /// When enabled, alloc() skips the VM mutex. Only safe in single-threaded
        /// native code that does not re-enter the VM (see BulkAlloc).
        void set_bulk_alloc(bool enabled) { bulk_alloc = enabled; }
        /// Returns true if a runtime error has occurred.
        bool is_panicked() const { return panicked; }
        /// Sets the panic flag, halting execution on the next dispatch.
        void set_panicked(bool v = true) { panicked = v; }

        /// Returns a mutable reference to the heap object at the given index.
        Object& get_heap_object(const HeapIdx idx) { return heap[idx]; }
        /// Returns true if the index is within the current heap bounds.
        bool heap_valid(const HeapIdx idx) const { return idx < heap.size(); }

        /// Returns a typed reference to the heap object at the given index.
        template<typename T>
        T& get_heap_object(const HeapIdx idx) {
            return std::get<T>(heap[idx].data);
        }

        /// Executes a compiled chunk on the VM.
        void execute(Chunk in_chunk);
        /// Converts a value to its string representation.
        std::string value_to_string(const Value& val);
        /// Registers a native function as a global callable by name.
        void define_native(const std::string& name, NativeFn function);
        /// Raises a runtime error with the given type and message.
        void runtime_error(const RuntimeError& err, const std::string& msg);

        const auto& get_interned_strings() const { return interned_strings; }

        /// Declares a new global variable and returns its slot index.
        int declare_global(HeapIdx name_idx);

        /// Returns true if the value is truthy (non-zero, non-empty, non-none).
        bool is_truthy(const Value& v);

        /// Returns the canonical map key for a value (interns strings).
        Value canonicalize_map_key(const Value& key);

        /// Returns true if the value can be used as a Pyle map key.
        inline bool is_hashable(const Value& v) const {
            return v.tag != Value::Tag::ArrayRef &&
                v.tag != Value::Tag::MapRef &&
                v.tag != Value::Tag::StructRef;
        }

        /// Creates a new VM with the given configuration.
        explicit VM(const VMConfig& config = VMConfig()) {
            stack_capacity = config.stack_capacity;
            stack = new Value[stack_capacity];
            sp = stack;
            stack_end = stack + stack_capacity;

            frame_capacity = config.frame_capacity;
            frames = new CallFrame[frame_capacity];
            frame_count = 0;

            global_slots = &root_globals;
            globals_idx = HeapIdx(-1);

            const char* prof = std::getenv("PYLE_PROFILE");
            profile_ops = prof && prof[0] != '\0' && prof[0] != '0';

            set_gc_enabled(config.gc_enabled);
        }

        ~VM() {
            delete[] stack;
            delete [] frames;

            if (profile_ops) dump_op_profile();

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
        ankerl::unordered_dense::map<std::string, std::string> source_cache;
        ankerl::unordered_dense::map<HeapIdx, HeapIdx> closure_memo;

        size_t builtin_count = 0;
        bool builtins_finalized = false;
        std::vector<HeapIdx> saved_globals_stack;

        ankerl::unordered_dense::map<HeapIdx, int> builtin_slot_map;
        std::vector<ankerl::unordered_dense::map<HeapIdx, int>> saved_slot_maps_stack;


        HeapIdx active_coroutine_idx = 0;
        HeapIdx main_coroutine_idx = 0;
        bool coro_switched = false;

        /// Saves the current coroutine's execution state.
        inline void save_coroutine_state(Coroutine& coro) {
            coro.stack = this->stack;
            coro.sp = this->sp;
            coro.stack_capacity = this->stack_capacity;
            coro.frames = this->frames;
            coro.frame_count = this->frame_count;
            coro.frame_capacity = this->frame_capacity;
            coro.saved_globals_idx = this->globals_idx;
        }

        /// Restores a coroutine's previously saved execution state.
        inline void load_coroutine_state(Coroutine& coro) {
            this->stack = coro.stack;
            this->sp = coro.sp;
            this->stack_capacity = coro.stack_capacity;
            this->frames = coro.frames;
            this->frame_count = coro.frame_count;
            this->frame_capacity = coro.frame_capacity;
            this->stack_end = this->stack + this->stack_capacity;
            this->globals_idx = coro.saved_globals_idx;
            this->global_slots = (this->globals_idx == HeapIdx(-1))
                ? &this->root_globals
                : &std::get<ArrayType>(this->heap[this->globals_idx].data);
        }

        void init_root_coroutine();
        std::vector<std::string> import_paths = {"./"};

        /// Adds a directory to the module search path for import().
        void add_import_path(std::string path) {
            if (!path.empty() && path.back() != '/' && path.back() != '\\') {
                path += "/";
            }
            import_paths.push_back(std::move(path));
        }


        /// Roots a value on the GC stack until the matching gc_root_pop.
        void gc_root_push(HeapIdx idx, Value::Tag tag) {
            gc_roots.push_back(Value(tag, idx));
        }

        /// Pops the most recently pushed GC root.
        void gc_root_pop() {
            gc_roots.pop_back();
        }

        /// Marks a value and everything it references as reachable.
        void mark_value(const Value& val);
        /// Returns the VM mutex. Only needed for thread-safe native callbacks.
        std::recursive_mutex& get_mutex() { return vm_mutex; }

        /// Looks up a global variable by name. Returns none if not found.
        pyle::Value get_global(const std::string& name);

        /// Calls a callable with a vector of arguments (any count).
        /// Allocates a synthetic chunk per call, prefer call_func for hot paths.
        pyle::Value call_func_raw(pyle::Value callee, const std::vector<pyle::Value>& args);
        /// Calls a callable with one argument. Zero-allocation fast path.
        pyle::Value call_func1(pyle::Value callee, pyle::Value arg);
        /// Calls a callable with any number of arguments. Zero-allocation fast path.
        pyle::Value call_func_n(pyle::Value callee, const pyle::Value* args, size_t count);

        /// Calls a callable with any number of arguments. Automatically converts
        /// each argument and dispatches to the matching fast path.
        template <typename... Args>
        Value call_func(Value callee, Args&&... args);

        /// Calls a Pyle function under a catch checkpoint. Returns {ok, value}
        /// on success or {ok, error} with a {type, message, trace} map on panic.
        pyle::Value pcall_invoke(pyle::Value callee, const pyle::Value* args, size_t count);
        /// Prints a pcall-shaped error map using the runtime error format.
        void print_trace(pyle::Value err);
        /// Re-raises an error map (or message string) with its trace preserved.
        void raise_error(pyle::Value err);

    private:
        Value last_result;
        void run_loop();
        void ensure_call_trampoline();
        HeapIdx call_trampoline_idx = HeapIdx(-1);
        Coroutine* active_coro();
        TraceFrame describe_frame(CallFrame& frame);
        void snapshot_trace(Coroutine& coro);
        void clear_pending();
        void print_error_report(std::string_view type_str, std::string_view msg,
            const std::vector<TraceFrame>& trace, const std::string& hint);
        pyle::Value build_error_value(Coroutine& coro);
        pyle::Value make_pcall_result(bool ok, Value payload);
        bool deliver_task_failure();
        bool parse_error_map(Value v, std::string& type, std::string& msg,
            std::vector<TraceFrame>& trace);
        std::string hint_for(const std::string& type, const std::string& msg);

        std::recursive_mutex vm_mutex; 
        bool gc_enabled = true;

        // MEMORY
        std::vector<HeapIdx> gc_worklist;
        void gc_sweep();
        void gc_mark();
        void gc_collect();
        
        std::deque<Object> heap;
        std::vector<HeapIdx> free_list;

        std::vector<Value> gc_roots;

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
        int execute_depth = 0;
        bool bulk_alloc = false;
        uint64_t op_counts[256] = {};
        bool profile_ops = false;
        void dump_op_profile();

        inline void push(Value value) { 
            if (sp == stack_end) {
                grow_stack();
            }
            *sp++ = value;
        }

        PYLE_FORCEINLINE Value pop() { 
            #ifndef NDEBUG
                if (sp <= stack) {
                    runtime_error(RuntimeError::StackUnderflow, "Internal VM Error: Stack underflow.");
                    return Value(); 
                }
            #endif
            return *(--sp);
        }
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

    /// RAII guard that roots a value on the GC stack for its lifetime.
    struct GCRoot {
        VM& vm;
        GCRoot(VM& vm, HeapIdx idx, Value::Tag tag) : vm(vm) {
            vm.gc_root_push(idx, tag);
        }
        ~GCRoot() {
            vm.gc_root_pop();
        }
    };

    /// RAII guard for bulk native-side allocation. Disables GC and skips the
    /// VM mutex for the duration. Only safe when no Pyle code re-enters the VM.
    struct BulkAlloc {
        VM& vm;
        explicit BulkAlloc(VM& v) : vm(v) { vm.set_gc_enabled(false); vm.set_bulk_alloc(true); }
        ~BulkAlloc() { vm.set_bulk_alloc(false); vm.set_gc_enabled(true); }
    };
}


