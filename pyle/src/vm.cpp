#include "pyle/vm.hpp"
#include <fmt/printf.h>
#include <sstream>
#include <string>
#include <variant>
#include "pyle/bytecode.hpp"
#include "pyle/std/std_array.hpp"
#include "pyle/std/std_string.hpp"
#include "pyle/value.hpp"

namespace pyle {
    constexpr size_t INITIAL_THRESHOLD = 256;
    size_t gc_threshold = INITIAL_THRESHOLD;

    PYLE_FORCEINLINE void sync_frame_cache(VM::CallFrame* f, Function& fn,
                                const uint32_t*& instr_data, const uint32_t*& ip,
                                const uint32_t*& ip_end, const Value*& const_pool,
                                size_t& const_pool_size) {
        instr_data = fn.chunk.instr.data();
        ip = instr_data + f->ip;
        ip_end = instr_data + fn.chunk.instr.size();
        const_pool = fn.chunk.const_pool.data();
        const_pool_size = fn.chunk.const_pool.size();
    }

    void VM::grow_stack() {
        size_t new_capacity = stack_capacity * 2;
        Value* new_stack = new Value[new_capacity];

        size_t current_size = sp - stack;

        for (size_t i = 0; i < current_size; ++i) {
            new_stack[i] = stack[i];
        }

        delete [] stack;
        stack = new_stack;
        sp = stack + current_size;
        stack_end = stack + new_capacity;
        stack_capacity = new_capacity;
    }

    bool VM::is_truthy(const Value& v) {
        switch (v.tag) {
            case Value::Tag::Null:  return false;
            case Value::Tag::Bool:  return v.as_bool;
            case Value::Tag::Int:   return v.as_int != 0;
            case Value::Tag::Float: return v.as_float != 0.0;
            default:                return true;
        }
    }

    HeapIdx VM::intern_string(std::string_view str) {
        auto it = interned_strings.find(str);
        if (it != interned_strings.end()) {
            return it->second;
        }

        HeapIdx idx = alloc(Object(std::string(str)));
        auto& stored = std::get<std::string>(heap[idx].data);
        interned_strings[stored] = idx;
        return idx;
    }

    HeapIdx VM::alloc(Object obj) {
        if (gc_enabled && free_list.empty() && heap.size() >= gc_threshold) {
            gc_collect();

            const size_t live_objects = heap.size() - free_list.size();
            gc_threshold = std::max(INITIAL_THRESHOLD, live_objects * 2);
        }

        if (!free_list.empty()) {
            const HeapIdx idx = free_list.back();
            free_list.pop_back();
            heap[idx] = std::move(obj);
            return idx;
        }

        const HeapIdx idx = heap.size();
        heap.push_back(std::move(obj));
        return idx;
    }

    void VM::gc_sweep() {
        for (HeapIdx i = 0; i < heap.size(); ++i) {
            if (std::holds_alternative<std::monostate>(heap[i].data))
                continue;

            if (!heap[i].gc_marked) {
                heap[i].data = std::monostate{};
                free_list.push_back(i);
            } else {
                heap[i].gc_marked = false;
            }
        }

        for (auto it = interned_strings.begin(); it != interned_strings.end();) {
            if (std::holds_alternative<std::monostate>(heap[it->second].data)) {
                it = interned_strings.erase(it);
            } else {
                ++it;
            }
        }
    }

    void VM::gc_mark() {
        std::vector<HeapIdx> worklist;

        auto mark_value = [&](const Value &val) {
            switch (val.tag) {
                case Value::Tag::StringRef:
                case Value::Tag::ArrayRef:
                case Value::Tag::StructRef:
                case Value::Tag::NativeFuncRef:
                case Value::Tag::FuncRef: {
                    HeapIdx idx = val.as_ref;
                    if (!heap[idx].gc_marked) {
                        heap[idx].gc_marked = true;
                        worklist.push_back(idx);
                    }
                    break;
                }
                default: break;
            }
        };

        for (Value* ptr = stack; ptr < sp; ++ptr) {
            mark_value(*ptr);
        }

        for (const Value& val : global_slots) {
            mark_value(val);
        }

        for (size_t i = 0; i < frame_count; ++i) {
            const auto& frame = frames[i];
            mark_value(Value(Value::Tag::FuncRef, frame.function));
            for (const Value &val: get_function(frame).chunk.const_pool) {
                mark_value(val);
            }
        }

        while (!worklist.empty()) {
            HeapIdx current = worklist.back();
            worklist.pop_back();

            if (const auto *array_ptr = std::get_if<ArrayType>(&heap[current].data)) {
                for (const Value &val: *array_ptr) {
                    mark_value(val);
                }
            } else if (const auto *struct_ptr = std::get_if<StructType>(&heap[current].data)) {
                for (const auto& [key, val] : *struct_ptr) {
                    mark_value(val);
                }
            }
        }
    }

    void VM::gc_collect() {
        gc_mark();
        gc_sweep();
    }


    std::string VM::value_to_string(const Value &val) {
        std::unordered_set<HeapIdx> visited;
        std::stringstream ss;
        value_to_string_helper(val, visited, ss);
        return ss.str();
    }

    void VM::value_to_string_helper(const Value &val, std::unordered_set<HeapIdx> &visited, std::stringstream &ss) {
        switch (val.tag) {
            case Value::Tag::Int: ss << val.as_int; break;
            case Value::Tag::Float: ss << val.as_float; break;
            case Value::Tag::Bool: ss << (val.as_bool ? "true": "false"); break;
            case Value::Tag::Null: ss << "null"; break;
            case Value::Tag::NativeFuncRef: ss << "<native_function>"; break;
            case Value::Tag::StringRef: {
                ss << std::get<std::string>(heap[val.as_ref].data);
                break;
            }
            case Value::Tag::ArrayRef: {
                HeapIdx idx = val.as_ref;
                if (visited.count(idx)) {
                    ss << "[" << idx << "]";
                    return;
                }
                visited.insert(idx);
                const auto& vec = std::get<ArrayType>(heap[idx].data);
                ss << "[";
                for (size_t i = 0; i < vec.size(); ++i) {
                    value_to_string_helper(vec[i], visited, ss);
                    if (i < vec.size() - 1)
                        ss << ", ";
                }
                ss << "]";

                visited.erase(idx);
                break;
            }
            case Value::Tag::StructRef: {
                HeapIdx idx = val.as_ref;
                if (visited.count(idx)) {
                    ss << "{...}";
                    return;
                }
                visited.insert(idx);

                const auto& fields = std::get<StructType>(heap[idx].data);
                ss << "{";
                size_t i = 0;

                for (const auto& [key, field_val]: fields) {
                    ss << key << ": ";
                    value_to_string_helper(field_val, visited, ss);
                    if (++i < fields.size()) {
                        ss << ", ";
                    }
                    ss << "}";
                    visited.erase(idx);
                    break;
                }
            }
            default:
                ss << "HeapRef(" << val.as_ref << ")";
        }
    }

    void VM::runtime_error(const RuntimeError &type, const std::string &msg) {
        panicked = true;
        size_t line = 0;
        CallFrame& frame = frames[frame_count - 1];
        Function& func = get_function(frame);
        if (frame.ip > 0 && frame.ip <= func.chunk.lines.size()) {
            line = func.chunk.lines[frame.ip - 1];
        }
        fmt::print(stderr, "{}: {}\n", err_to_string(type), msg);
        if (line > 0) {
            fmt::print(stderr, " at line {}\n", line + 1);
        }
    }

#define BINARY_OP(op, sync_expr) \
    do { \
        if (stack_size() < 2) { \
            sync_expr; \
            runtime_error(RuntimeError::StackUnderflow, "Not enough values in the stack to apply binary operation"); \
            return; \
        } \
        Value b = pop(); \
        Value a = peek();\
        if (a.tag == Value::Tag::Int && b.tag == Value::Tag::Int) { \
            set_top(Value(a.as_int op b.as_int)); \
        } else if ((a.tag == Value::Tag::Int || a.tag == Value::Tag::Float) && \
            (b.tag == Value::Tag::Int || b.tag == Value::Tag::Float)) { \
            double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float; \
            double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float; \
            set_top(Value(da op db));  \
        } else { \
            std::string msg = fmt::format("Unsupported operand types. a.tag={}, b.tag={}", \
            static_cast<int>(a.tag), static_cast<int>(b.tag)); \
            sync_expr; \
            runtime_error(RuntimeError::Type, msg); \
            return; \
        } \
    } while (false)

bool VM::values_equal(const Value& a, const Value& b) {
    if ((a.tag == Value::Tag::Int || a.tag == Value::Tag::Float) &&
        (b.tag == Value::Tag::Int || b.tag == Value::Tag::Float)) {
        double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float;
        double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float;
        return da == db;
    }
    if (a.tag != b.tag) return false;
    switch (a.tag) {
        case Value::Tag::Null: return true;
        case Value::Tag::Bool: return a.as_bool == b.as_bool;
        case Value::Tag::StringRef:
        case Value::Tag::ArrayRef:
        case Value::Tag::StructRef:
        case Value::Tag::NativeFuncRef:
            return a.as_ref == b.as_ref;
        default: return false;
    }
}

#define COMPARISON_OP(op, sync_expr) \
    do { \
        if (stack_size() < 2) { \
            sync_expr; \
            runtime_error(RuntimeError::StackUnderflow, "Not enough values in the stack to apply comparison operation"); \
            return; \
        } \
        Value b = pop(); \
        Value a = peek(); \
        if (a.tag == Value::Tag::Int && b.tag == Value::Tag::Int) { \
            set_top(Value(a.as_int op b.as_int)); \
        } else if ((a.tag == Value::Tag::Int || a.tag == Value::Tag::Float) && \
            (b.tag == Value::Tag::Int || b.tag == Value::Tag::Float)) { \
            double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float; \
            double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float; \
            set_top(Value(da op db)); \
        } else { \
            std::string msg = fmt::format("Unsupported operand types for comparison. a.tag={}, b.tag={}", \
            a.tag_to_string(), b.tag_to_string()); \
            sync_expr; \
            runtime_error(RuntimeError::Type, msg); \
            return; \
        } \
    } while (false)


#if defined(__clang__) || defined(__GNUC__)
    #define PYLE_USE_COMPUTED_GOTO
#endif

#ifdef PYLE_USE_COMPUTED_GOTO
    #define OP(name) op_##name:
    #define ARG get_operand(*(ip - 1))
    #define DISPATCH() \
        do { \
            if (panicked) return; \
            uint32_t instruction = *ip++; \
            goto *dispatch_table[static_cast<uint8_t>(get_op(instruction))]; \
        } while (false)
#else
    #define OP(name) case OpCode::name:
    #define ARG arg
    #define DISPATCH() break
#endif

    void VM::execute(Chunk in_chunk) {
        frame_count = 0;
        sp = stack;
        panicked = false;

        Function main_fn;
        main_fn.name = "main";
        main_fn.chunk = std::move(in_chunk);
        
        HeapIdx main_idx = alloc(Object(std::move(main_fn)));

        CallFrame root_frame;
        root_frame.function = main_idx;  
        root_frame.ip = 0;
        root_frame.stack_base = 0;

        frames[frame_count++] = root_frame;

        CallFrame* frame = &frames[frame_count - 1];
        
        Function& fn = get_function(*frame);
        const uint32_t* instr_data;
        const uint32_t* ip;
        const uint32_t* ip_end;
        const Value* const_pool;
        size_t const_pool_size;
        sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                
        auto sync_ip = [&]() {
            frame->ip = ip - instr_data;
        };

        auto runtime_error = [&](const RuntimeError &type, const std::string &msg) {
            sync_ip();
            this->runtime_error(type, msg);
        };

#ifdef PYLE_USE_COMPUTED_GOTO
        static const void* dispatch_table[] = {
            &&op_LOAD_CONST,
            &&op_LOAD_LOCAL,
            &&op_SET_LOCAL,
            &&op_LOAD_GLOBAL_SLOT,
            &&op_SET_GLOBAL_SLOT,
            &&op_DEFINE_GLOBAL_SLOT,
            &&op_SET_LOCAL_POP,
            &&op_SET_GLOBAL_SLOT_POP,
            &&op_ADD,
            &&op_SUB,
            &&op_MUL,
            &&op_DIV,
            &&op_MOD,
            &&op_NEG,
            &&op_EQ,
            &&op_NEQ,
            &&op_LT,
            &&op_LTE,
            &&op_GT,
            &&op_GTE,
            &&op_NOT,
            &&op_JUMP,
            &&op_JUMP_IF_FALSE,
            &&op_JUMP_IF_TRUE,
            &&op_LOOP,
            &&op_CALL,
            &&op_CALL_METHOD,
            &&op_RETURN,
            &&op_POP,
            &&op_NEW_STRUCT,
            &&op_GET_FIELD,
            &&op_SET_FIELD,
            &&op_NEW_ARRAY,
            &&op_GET_INDEX,
            &&op_SET_INDEX,
            &&op_HALT
        };
        DISPATCH(); // Kick off the computed goto execution!
#else
        while (true) {
            if (panicked) return;

            if (ip >= ip_end) { 
                sync_ip();
                runtime_error(RuntimeError::OutOfBounds, "Instruction pointer out of bounds.");
                return;
            }
            uint32_t instruction = *ip++;

            OpCode op = get_op(instruction);
            uint32_t arg = get_operand(instruction);

            switch (op) {
#endif

                OP(LOAD_CONST) {
                    #ifndef NDEBUG
                    if (ARG >= const_pool_size) {
                        runtime_error(RuntimeError::OutOfBounds, "Load constant index out of bounds.");
                        return;
                    }
                    #endif
                    push(const_pool[ARG]);
                }
                DISPATCH();

                OP(POP) {
                    pop(); 
                }
                DISPATCH();

                OP(ADD) {
                    BINARY_OP(+, sync_ip()); 
                }
                DISPATCH();

                OP(SUB) {
                    BINARY_OP(-, sync_ip()); 
                }
                DISPATCH();

                OP(MUL) {
                    BINARY_OP(*, sync_ip()); 
                }
                DISPATCH();

                OP(DIV) {
                    if (stack_size() < 2) {
                        runtime_error(RuntimeError::StackUnderflow, "Stack underflow in division\n");
                        return;
                    }
                    Value b = peek();
                    if ((b.tag == Value::Tag::Int && b.as_int == 0) ||
                        (b.tag == Value::Tag::Float && b.as_float == 0.0)) {
                        runtime_error(RuntimeError::ZeroDivision, "Division by zero.");
                        return;
                    }
                    BINARY_OP(/, sync_ip());
                }
                DISPATCH();

                OP(MOD) {
                    if (stack_size() < 2) {
                        runtime_error(RuntimeError::StackUnderflow, "Not enough values in the stack to apply modulo");
                        return;
                    }
                    
                    Value b = pop();  
                    Value a = peek(); 
                    
                    if ((b.tag == Value::Tag::Int && b.as_int == 0) ||
                        (b.tag == Value::Tag::Float && b.as_float == 0.0)) {
                        runtime_error(RuntimeError::ZeroDivision, "Modulo by zero.");
                        return;
                    }

                    if (a.tag == Value::Tag::Int && b.tag == Value::Tag::Int) {
                        set_top(Value(a.as_int % b.as_int)); // Overwrite in-place
                    } else if ((a.tag == Value::Tag::Int || a.tag == Value::Tag::Float) &&
                            (b.tag == Value::Tag::Int || b.tag == Value::Tag::Float)) {
                        double da = (a.tag == Value::Tag::Int) ? static_cast<double>(a.as_int) : a.as_float;
                        double db = (b.tag == Value::Tag::Int) ? static_cast<double>(b.as_int) : b.as_float;
                        set_top(Value(std::fmod(da, db)));   // Overwrite in-place
                    } else {
                        std::string msg = fmt::format("Unsupported operand types for modulo. a.tag={}, b.tag={}", 
                        static_cast<int>(a.tag), static_cast<int>(b.tag));
                        runtime_error(RuntimeError::Type, msg);
                        return;
                    }
                }
                DISPATCH();

                OP(EQ) {
                    Value b = pop();
                    Value a = peek();
                    set_top(Value(values_equal(a, b)));
                }   
                DISPATCH();

                OP(NEQ) {
                    Value b = pop();
                    Value a = peek();
                    set_top(Value(!values_equal(a, b)));
                }
                DISPATCH();

                OP(LT) {
                    COMPARISON_OP(<, sync_ip()); 
                }
                DISPATCH();

                OP(LTE) {
                    COMPARISON_OP(<=, sync_ip()); 
                }
                DISPATCH();

                OP(GT) {
                    COMPARISON_OP(>, sync_ip()); 
                }
                DISPATCH();

                OP(GTE) {
                    COMPARISON_OP(>=, sync_ip()); 
                }
                DISPATCH();

                OP(LOAD_LOCAL) {
                    push(stack[frame->stack_base + ARG]);
                }
                DISPATCH();

                OP(SET_LOCAL) {
                    stack[frame->stack_base + ARG] = peek();
                }
                DISPATCH();

                OP(LOAD_GLOBAL_SLOT) {
                    #ifndef NDEBUG
                    if (ARG >= global_slots.size()) {
                        runtime_error(RuntimeError::OutOfBounds, "Global slot out of bounds.");
                        return;
                    }
                    #endif
                    push(global_slots[ARG]);
                } 
                DISPATCH();

                OP(SET_GLOBAL_SLOT) {
                    #ifndef NDEBUG
                    if (ARG >= global_slots.size()) {
                        runtime_error(RuntimeError::OutOfBounds, "Global slot out of bounds.");
                        return;
                    }
                    #endif
                    global_slots[ARG] = peek();
                } 
                DISPATCH();

                OP(DEFINE_GLOBAL_SLOT) {
                    while (ARG >= global_slots.size()) global_slots.push_back(Value());
                    global_slots[ARG] = pop();
                }
                DISPATCH();

                OP(SET_LOCAL_POP) {
                    stack[frame->stack_base + ARG] = pop();
                }
                DISPATCH();

                OP(SET_GLOBAL_SLOT_POP) {
                    global_slots[ARG] = pop();
                }
                DISPATCH();

                OP(CALL) {
                    int arg_count = ARG;
                    if (static_cast<size_t>(sp - stack) < static_cast<size_t>(arg_count + 1)) {
                        runtime_error(RuntimeError::StackUnderflow, "Not enough values on stack for function call.");
                        return;
                    }

                    Value callee = peek(arg_count + 1);

                    if (callee.tag == Value::Tag::NativeFuncRef) {
                        NativeFn native = std::get<NativeFn>(heap[callee.as_ref].data);

                        const Value* args_ptr = arg_count > 0 ? (sp - arg_count) : nullptr;
                        ArgView args_view{args_ptr, static_cast<size_t>(arg_count)};

                        sync_ip();
                        Value result = native(*this, args_view);
                        sp -= arg_count; 
                        set_top(result);
                    } else if (callee.tag == Value::Tag::FuncRef) {
                        Function& fn = std::get<Function>(heap[callee.as_ref].data);
                        
                        if (fn.arity != arg_count) {
                            runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", fn.arity, arg_count));
                            return;
                        }

                        sync_ip(); 
                        CallFrame new_frame;
                        new_frame.function = callee.as_ref;
                        new_frame.ip = 0;
                        new_frame.stack_base = stack_size() - arg_count;
                        
                        if (frame_count == frame_capacity) [[unlikely]] {
                            runtime_error(RuntimeError::Runtime, "Stack overflow (maximum recursion depth exceeded).");
                            return;
                        }

                        frames[frame_count++] = new_frame;
                        frame = &frames[frame_count - 1];

                        sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                    } 
                    else {
                        runtime_error(RuntimeError::Type, fmt::format("Can only call functions. Grabbed Tag={}, StackSize={}",
                                      static_cast<int>(callee.tag), stack_size()));
                        return;
                    }
                }
                DISPATCH();

                OP(RETURN) {
                    Value ret_val = peek();
                    size_t stack_base = frame->stack_base; 
                    frame_count--;
                    if (frame_count == 0) return;

                    stack[stack_base - 1] = ret_val;
                    sp = stack + stack_base; 

                    frame = &frames[frame_count - 1];

                    Function& fn = get_function(*frame);
                    sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                }
                DISPATCH();

                OP(CALL_METHOD) {
                    int arg_count = ARG;

                    if (stack_size() < arg_count + 1) {
                        runtime_error(RuntimeError::StackUnderflow, "Not enough values on stack for method call.");
                        return;
                    }

                    Value name_val = peek(arg_count + 1);
                    Value callee = peek(arg_count + 2);

                    if (name_val.tag != Value::Tag::StringRef) {
                        runtime_error(RuntimeError::Type, "Expected string for method name.");
                        return;
                    }

                    std::string method_name = std::get<std::string>(heap[name_val.as_ref].data);

                    const Value *args_ptr = arg_count > 0 ? (sp - arg_count) : nullptr;
                    ArgView args_view{args_ptr, static_cast<size_t>(arg_count)};

                    sync_ip();
                    Value result;
                    if (callee.tag == Value::Tag::ArrayRef) {
                        result = ArrayMethods::dispatch(*this, callee.as_ref, method_name, args_view);
                    } else if (callee.tag == Value::Tag::StringRef) {
                        result = StringMethods::dispatch(*this, callee.as_ref, method_name, args_view);
                    } else {
                        runtime_error(RuntimeError::Type, fmt::format("Expected object with method, got {} instead", callee.tag_to_string()));
                        return;
                    }

                    if (panicked) return;

                    sp -= (arg_count + 1);
                    set_top(result);
                }
                DISPATCH();

                OP(NEW_ARRAY) {
                    int element_count = static_cast<int>(ARG);
                    ArrayType elements(element_count);

                    Value* base = sp - element_count;
                    for (int i = 0; i < element_count; i++) {
                        elements[i] = base[i];
                    }

                    HeapIdx idx = alloc(Object(std::move(elements)));
                    sp = base;
                    push(Value(Value::Tag::ArrayRef, idx));
                }
                DISPATCH();

                OP(GET_INDEX) {
                    if (stack_size() < 2) {
                        runtime_error(RuntimeError::StackUnderflow, "Stack underflow in GET_INDEX.");
                        return;
                    }

                    Value index = pop();
                    Value container = peek();

                    if (index.tag != Value::Tag::Int) {
                        runtime_error(RuntimeError::Type, fmt::format("Array index must be an integer, got '{}'.", index.tag_to_string()));
                        return;
                    }
                    
                    switch (container.tag) {
                        case Value::Tag::ArrayRef: { 
                            auto& vec = std::get<ArrayType>(heap[container.as_ref].data);
                            if (index.as_int < 0 || index.as_int >= static_cast<int64_t>(vec.size())) {
                                runtime_error(RuntimeError::Index, fmt::format("Array index {} out of bounds for size {}.", index.as_int, vec.size()));
                                return;
                            }
                            set_top(vec[index.as_int]);
                            break;
                        }
                        case Value::Tag::StringRef: {
                            const auto& str = std::get<std::string>(heap[container.as_ref].data);
                            if (index.as_int < 0 || index.as_int >= static_cast<int64_t>(str.size())) {
                                runtime_error(RuntimeError::Index, fmt::format("String index {} out of bounds for length {}.", index.as_int, str.size()));
                                return;
                            }
                            std::string char_str(1, str[index.as_int]);
                            HeapIdx char_idx = intern_string(char_str);
                            set_top(Value(Value::Tag::StringRef, char_idx));
                            break;
                        }
                        default: {
                            runtime_error(RuntimeError::Type, fmt::format("Cannot set index on type '{}'.", container.tag_to_string()));
                            return;
                        }
                    }
                }
                DISPATCH();

                OP(SET_INDEX) {
                    if (stack_size() < 3) {
                        runtime_error(RuntimeError::StackUnderflow, "Stack underflow in SET_INDEX.");
                        return;
                    }

                    Value value = peek(1);
                    Value index = peek(2);
                    Value array_val = peek(3);

                    if (array_val.tag != Value::Tag::ArrayRef) {
                        runtime_error(RuntimeError::Type, fmt::format("Cannot set index on non-array type '{}'.", array_val.tag_to_string()));
                        return;
                    }

                    if (index.tag != Value::Tag::Int) {
                        runtime_error(RuntimeError::Type, fmt::format("Array index must be an integer, got '{}'.", index.tag_to_string()));
                        return;
                    }

                    auto& vec = std::get<ArrayType>(heap[array_val.as_ref].data);
                    if (index.as_int < 0 || index.as_int >= static_cast<int64_t>(vec.size())) {
                        runtime_error(RuntimeError::Index, fmt::format("Array index {} out of bounds for size {}.", index.as_int, vec.size()));
                        return;
                    }

                    vec[index.as_int] = value;
                    sp -= 2;
                    set_top(value);
                }
                DISPATCH();

                OP(JUMP_IF_FALSE) {
                    if (!is_truthy(peek())) {
                        ip += ARG;
                    }
                }
                DISPATCH();

                OP(JUMP_IF_TRUE) {
                    if (is_truthy(peek()))  {
                        ip += ARG;
                    }
                }
                DISPATCH();

                OP(NOT) {
                    Value val = pop();
                    push(Value(!is_truthy(val)));
                }
                DISPATCH();

                OP(NEG) {
                    if (sp == stack) {
                        runtime_error(RuntimeError::StackUnderflow, "Stack underflow in unary minus.");
                        return;
                    }   
                    Value val = peek();
                    switch (val.tag) {
                        case Value::Tag::Int: set_top(Value(-val.as_int)); break;
                        case Value::Tag::Float: set_top(Value(-val.as_float)); break;
                        default:
                            runtime_error(RuntimeError::Type, fmt::format("Cannot negate type '{}'.", val.tag_to_string()));
                            return;
                    }
                }
                DISPATCH();

                OP(JUMP) {
                    ip += ARG;
                }
                DISPATCH();

                OP(LOOP) {
                    ip -= ARG;
                }
                DISPATCH();

                OP(NEW_STRUCT)
                OP(GET_FIELD)
                OP(SET_FIELD) {
                    runtime_error(RuntimeError::Runtime, "Unimplemented opcode called.");
                    return;
                }

                OP(HALT) {
                    return;
                }

#ifndef PYLE_USE_COMPUTED_GOTO
                default:
                    sync_ip();
                    runtime_error(RuntimeError::Runtime, "Unsupported opcode.");
                    return;
            }
        }
#endif
    }

    
    int VM::declare_global(const std::string& name) {
        auto it = global_slot_map.find(name);
        if (it != global_slot_map.end()) return it->second;

        int slot = static_cast<int>(global_slot_map.size());
        global_slots.push_back(Value());
        global_slot_map[name] = slot;
        return slot;
    }

    void VM::define_native(const std::string &name, NativeFn function) {
        HeapIdx name_idx = intern_string(name);
        HeapIdx fn_idx = alloc(Object(function));

        Value fn_val(Value::Tag::NativeFuncRef, fn_idx);
        int slot = declare_global(name);

        global_slots[slot] = fn_val;
    }


#undef BINARY_OP
}