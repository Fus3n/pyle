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
            case Value::Tag::None:  return false;
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

    HeapIdx VM::build_closure_for_call(HeapIdx fn_idx, CallFrame* caller_frame) {
        Function& fn = std::get<Function>(heap[fn_idx].data);
        Closure closure;
        closure.function = fn_idx;
        for (const auto& uv : fn.upvalues) {
            if (uv.is_local) {
                closure.upvalues.push_back(capture_upvalue(caller_frame->stack_base + uv.index));
            } else {
                Closure& parent_closure = std::get<Closure>(heap[caller_frame->closure].data);
                closure.upvalues.push_back(parent_closure.upvalues[uv.index]);
            }
        }
        return alloc(Object(closure));
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
                case Value::Tag::NativeFuncRef:
                case Value::Tag::FuncRef:
                case Value::Tag::IteratorRef: 
                case Value::Tag::RangeRef: 
                case Value::Tag::ClosureRef: 
                case Value::Tag::UpvalueRef: 
                case Value::Tag::StructRef:
                case Value::Tag::StructTypeRef: 
                case Value::Tag::MapRef: {
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
            mark_value(Value(Value::Tag::ClosureRef, frame.closure)); 
            for (const Value &val: get_func_from_frame(frame).chunk.const_pool) {
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
            } else if (const auto *struct_ptr = std::get_if<Struct>(&heap[current].data)) { 
                mark_value(Value(Value::Tag::StructTypeRef, struct_ptr->type_idx)); 
                for (const Value &val: struct_ptr->fields) {
                    mark_value(val);
                }
            } else if (const auto *type_ptr = std::get_if<StructType>(&heap[current].data)) {
                for (HeapIdx str_idx : type_ptr->field_names) {
                    mark_value(Value(Value::Tag::StringRef, str_idx)); 
                }
                
                for (auto [method_name_idx, fn_idx] : type_ptr->methods) {
                    mark_value(Value(Value::Tag::StringRef, method_name_idx));
                    mark_value(Value(Value::Tag::FuncRef, fn_idx));
                }
                for (HeapIdx fn_idx : type_ptr->special_methods) {
                    if (fn_idx != 0) {
                        mark_value(Value(Value::Tag::FuncRef, fn_idx));
                    }
                }

            } else if (const auto *iter_ptr = std::get_if<Iterator>(&heap[current].data)) {
                mark_value(iter_ptr->container);
            } else if (const auto *closure_ptr = std::get_if<Closure>(&heap[current].data)) { 
                mark_value(Value(Value::Tag::FuncRef, closure_ptr->function));
                for (HeapIdx uv_idx : closure_ptr->upvalues) { 
                    mark_value(Value(Value::Tag::UpvalueRef, uv_idx));
                }
            } else if (const auto *uv_ptr = std::get_if<Upvalue>(&heap[current].data)) { 
                if (uv_ptr->location == &uv_ptr->closed) {
                   mark_value(uv_ptr->closed); 
                }
            } else if (const auto *map_ptr = std::get_if<MapType>(&heap[current].data)) {
                for (const auto& [key, val] : *map_ptr) {
                    mark_value(key);
                    mark_value(val);
                }
            } else if (const auto *fn_ptr = std::get_if<Function>(&heap[current].data)) {
                for (const Value &val: fn_ptr->chunk.const_pool) {
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
            case Value::Tag::None: ss << "null"; break;
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
            case Value::Tag::StructTypeRef: {
                HeapIdx idx = val.as_ref;
                const auto& type = std::get<StructType>(heap[idx].data);
                ss << "struct {";
                for (size_t i = 0; i < type.field_names.size(); ++i) {
                    ss << std::get<std::string>(heap[type.field_names[i]].data);
                    if (i < type.field_names.size() - 1) ss << ", ";
                }
                ss << "}";
                break;
            }
            case Value::Tag::StructRef: {
                HeapIdx idx = val.as_ref;
                if (visited.count(idx)) {
                    ss << "{...}";
                    return;
                }
                visited.insert(idx);

                const auto& s = std::get<Struct>(heap[idx].data);
                const auto& type = std::get<StructType>(heap[s.type_idx].data);
                
                ss << "{";
                for (size_t i = 0; i < s.fields.size(); ++i) {
                    std::string field_name = std::get<std::string>(heap[type.field_names[i]].data);
                    ss << field_name << ": ";
                    value_to_string_helper(s.fields[i], visited, ss);
                    if (i < s.fields.size() - 1) {
                        ss << ", ";
                    }
                }
                ss << "}";
                visited.erase(idx);
                break;
            }
            case Value::Tag::RangeRef: {
                HeapIdx idx = val.as_ref;
                const auto& r = std::get<Range>(heap[idx].data);
                ss << r.start << ".." << r.end;
                break;
            }
            case Value::Tag::MapRef: {
                HeapIdx idx = val.as_ref;
                if (visited.count(idx)) {
                    ss << "{...}";
                    return;
                }
                visited.insert(idx);

                const auto& map = std::get<MapType>(heap[idx].data);
                ss << "{";
                size_t i = 0;

                for (const auto& [key, map_val] : map) {
                    value_to_string_helper(key, visited, ss);
                    ss << ": ";
                    value_to_string_helper(map_val, visited, ss);
                    if (++i < map.size()) {
                        ss << ", ";
                    }
                }
                ss << "}";
                visited.erase(idx);
                break;
            }
            default:
                ss << "HeapRef(" << val.as_ref << ")";
        }
    }

    void VM::runtime_error(const RuntimeError &type, const std::string &msg) {
        panicked = true;
        size_t line = 0;
        CallFrame& frame = frames[frame_count - 1];
        Function& func = get_func_from_frame(frame);
        if (frame.ip > 0 && frame.ip <= func.chunk.lines.size()) {
            line = func.chunk.lines[frame.ip - 1];
        }
        
        fmt::print(stderr, "\033[1;31m{}:\033[0m \033[1m{}\033[0m\n", err_to_string(type), msg);
        fmt::print(stderr, "   --> {}:{}: (in function '{}')\n", script_name, line + 1, func.name);
        
        if (!source_code.empty()) {
            auto get_line_of_code = [](std::string_view src, size_t target_line) -> std::string_view {
                size_t current_line = 0;
                size_t start = 0;
                for (size_t i = 0; i < src.size(); ++i) {
                    if (src[i] == '\n') {
                        if (current_line == target_line) {
                            return src.substr(start, i - start);
                        }
                        start = i + 1;
                        current_line++;
                    }
                }
                if (current_line == target_line && start < src.size()) {
                    return src.substr(start);
                }
                return "";
            };
            
            std::string_view line_text = get_line_of_code(source_code, line);
            if (!line_text.empty()) {
                fmt::print(stderr, " {:4d} | {}\n", line + 1, line_text);
                
                size_t first_non_space = 0;
                while (first_non_space < line_text.size() && (line_text[first_non_space] == ' ' || line_text[first_non_space] == '\t')) {
                    first_non_space++;
                }
                std::string carets = "        | ";
                for (size_t i = 0; i < first_non_space; ++i) {
                    if (line_text[i] == '\t') carets += '\t';
                    else carets += ' ';
                }
                carets += "\033[1;31m^^^^~\033[0m";
                fmt::print(stderr, "{}\n", carets);
            }
        }
        
        std::string hint = get_runtime_hint(type, msg);
        if (!hint.empty()) {
            fmt::print(stderr, "    \033[1;36mHint:\033[0m {}\n", hint);
        }
        fmt::print(stderr, "\n");
    }

    HeapIdx VM::capture_upvalue(size_t stack_index) {
        Value* local_ptr = &stack[stack_index];
        
        // if an upvalue exists for this variable, share it
        for (HeapIdx uv_idx : open_upvalues) {
            Upvalue& uv = std::get<Upvalue>(heap[uv_idx].data);
            if (uv.location == local_ptr) {
                return uv_idx;
            }
        }
        
        // new upvalue
        Upvalue uv;
        uv.location = local_ptr;
        HeapIdx idx = alloc(Object(uv));
        open_upvalues.push_back(idx);
        return idx;
    }

    void VM::close_upvalues(Value* limit) {
        for (auto it = open_upvalues.begin(); it != open_upvalues.end(); ) {
            Upvalue& uv = std::get<Upvalue>(heap[*it].data);
            if (uv.location >= limit) {
                uv.closed = *(uv.location);
                uv.location = &uv.closed;
                it = open_upvalues.erase(it);
            } else {
                ++it;
            }
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
            if (ip >= ip_end) { \
                frame->ip = ip - instr_data; \
                this->runtime_error(RuntimeError::OutOfBounds, "Instruction pointer out of bounds."); \
                return; \
            } \
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
        
        HeapIdx main_func_idx = alloc(Object(std::move(main_fn)));
        HeapIdx main_closure_idx = alloc(Object(Closure{main_func_idx}));

        CallFrame root_frame;
        root_frame.closure = main_closure_idx;  
        root_frame.ip = 0;
        root_frame.stack_base = 0;

        frames[frame_count++] = root_frame;

        CallFrame* frame = &frames[frame_count - 1];
        
        Function& fn = get_func_from_frame(*frame);
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
            &&op_GET_ITER,
            &&op_FOR_ITER,
            &&op_NEW_RANGE,
            &&op_CLOSURE,
            &&op_LOAD_UPVALUE,
            &&op_SET_UPVALUE,
            &&op_SET_UPVALUE_POP,
            &&op_GET_FIELD,
            &&op_SET_FIELD,
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
            &&op_NEW_ARRAY,
            &&op_NEW_MAP,
            &&op_CALL_KW,
            &&op_GET_INDEX,
            &&op_SET_INDEX,
            &&op_HALT
        };
        if (ip >= ip_end) {
            runtime_error(RuntimeError::OutOfBounds, "Instruction pointer out of bounds.");
            return;
        }
        DISPATCH(); 
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
                    set_top(Value(a == b));
                }   
                DISPATCH();

                OP(NEQ) {
                    Value b = pop();
                    Value a = peek();
                    set_top(Value(a != b));
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
                    if (stack_size() < arg_count + 1) {
                        runtime_error(RuntimeError::StackUnderflow, "Not enough values for call."); return;
                    }
                    Value callee = peek(arg_count + 1);
                    switch (callee.tag) {
                        case Value::Tag::ClosureRef: {
                            Closure& closure = std::get<Closure>(heap[callee.as_ref].data);
                            Function& fn = std::get<Function>(heap[closure.function].data);
                            if (fn.arity != arg_count) {
                                runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} args, got {}.", fn.arity, arg_count)); return;
                            }
                            sync_ip();
                            CallFrame new_frame;
                            new_frame.closure = callee.as_ref;
                            new_frame.ip = 0;
                            new_frame.stack_base = stack_size() - arg_count;
                            frames[frame_count++] = new_frame;
                            frame = &frames[frame_count - 1];
                            sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                            break;
                        }
                        case Value::Tag::NativeFuncRef: {
                            NativeFn native = std::get<NativeFn>(heap[callee.as_ref].data);
                            const Value* args_ptr = arg_count > 0 ? (sp - arg_count) : nullptr;
                            ArgView args_view{args_ptr, static_cast<size_t>(arg_count)};
                            sync_ip();
                            Value result = native(*this, args_view);
                            sp -= arg_count; 
                            set_top(result);
                            break;
                        }
                        case Value::Tag::StructTypeRef: {
                            StructType& type = std::get<StructType>(heap[callee.as_ref].data);
                            if (arg_count > type.field_names.size()) {
                                runtime_error(RuntimeError::ArgumentError, fmt::format("Too many arguments. Expected at most {}.", type.field_names.size())); 
                                return;
                            }

                            HeapIdx fn_idx = type.special_methods[static_cast<size_t>(SpecialMethod::Init)];
                            bool has_init = (fn_idx != 0);

                            Struct instance;
                            instance.type_idx = callee.as_ref;
                            instance.fields.resize(type.field_names.size());
                            for (int i = 0; i < arg_count; i++) {
                                instance.fields[i] = peek(arg_count - i);
                            }
                            for (size_t i = arg_count; i < type.field_names.size(); i++) {
                                instance.fields[i] = Value();
                            }

                            HeapIdx idx = alloc(Object(instance));
                            Value instance_val(Value::Tag::StructRef, idx);

                            if (has_init) {
                                Function& fn = std::get<Function>(heap[fn_idx].data);
                                if (fn.arity != arg_count + 1) {
                                    runtime_error(RuntimeError::ArgumentError, fmt::format("_init expects {} args, got {}.", fn.arity - 1, arg_count)); 
                                    return;
                                }

                                HeapIdx closure_idx = build_closure_for_call(fn_idx, frame);
                                Value init_clos(Value::Tag::ClosureRef, closure_idx);
                                Function& fn_post_clos = std::get<Function>(heap[fn_idx].data);

                                if (sp == stack_end) grow_stack();
                                std::copy_backward(sp - arg_count, sp, sp + 1);
                                sp++;

                                *(sp - 1 - arg_count) = instance_val;  // self
                                *(sp - 2 - arg_count) = init_clos;     // closure

                                sync_ip();
                                CallFrame new_frame;
                                new_frame.closure = closure_idx;
                                new_frame.ip = 0;
                                new_frame.stack_base = stack_size() - arg_count - 1; 
                                frames[frame_count++] = new_frame;
                                frame = &frames[frame_count - 1];
                                sync_frame_cache(frame, fn_post_clos, instr_data, ip, ip_end, const_pool, const_pool_size);
                            } else {
                                sp -= arg_count;
                                set_top(instance_val);
                            }
                            break;
                        }
                        default:
                            runtime_error(RuntimeError::Type, "Object is not callable."); return;
                    }
                }
                DISPATCH();

                OP(RETURN) {
                    Value ret_val = peek();
                    size_t stack_base = frame->stack_base; 
                    close_upvalues(&stack[stack_base]);
                    frame_count--;
                    if (frame_count == 0) return;
                    stack[stack_base - 1] = ret_val;
                    sp = stack + stack_base; 
                    frame = &frames[frame_count - 1];
                    Function& fn = get_func_from_frame(*frame);
                    sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                }
                DISPATCH();

                OP(CLOSURE) {
                    Value fn_val = pop(); 
                    Function& fn = std::get<Function>(heap[fn_val.as_ref].data);
                    
                    Closure closure;
                    closure.function = fn_val.as_ref;
                    
                    for (const auto& uv : fn.upvalues) {
                        if (uv.is_local) {
                            closure.upvalues.push_back(capture_upvalue(frame->stack_base + uv.index));
                        } else {
                            Closure& parent_closure = std::get<Closure>(heap[frame->closure].data);
                            closure.upvalues.push_back(parent_closure.upvalues[uv.index]);
                        }
                    }
                    
                    HeapIdx idx = alloc(Object(closure));
                    push(Value(Value::Tag::ClosureRef, idx));
                }
                DISPATCH();

                OP(LOAD_UPVALUE) {
                    Closure& closure = std::get<Closure>(heap[frame->closure].data);
                    Upvalue& uv = std::get<Upvalue>(heap[closure.upvalues[ARG]].data);
                    push(*(uv.location)); 
                }
                DISPATCH();

                OP(SET_UPVALUE) {
                    Closure& closure = std::get<Closure>(heap[frame->closure].data);
                    Upvalue& uv = std::get<Upvalue>(heap[closure.upvalues[ARG]].data);
                    *(uv.location) = peek();
                }
                DISPATCH();

                OP(SET_UPVALUE_POP) {
                    Closure& closure = std::get<Closure>(heap[frame->closure].data);
                    Upvalue& uv = std::get<Upvalue>(heap[closure.upvalues[ARG]].data);
                    *(uv.location) = pop();
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
                    if (callee.tag == Value::Tag::StructRef) {
                        Struct& s = std::get<Struct>(heap[callee.as_ref].data);
                        StructType& type = std::get<StructType>(heap[s.type_idx].data);
                        auto it = type.methods.find(name_val.as_ref);
                        if (it != type.methods.end()) {
                            HeapIdx fn_idx = it->second;

                            HeapIdx closure_idx = build_closure_for_call(fn_idx, frame);
                            Value method_closure(Value::Tag::ClosureRef, closure_idx);

                            Function& fn = std::get<Function>(heap[fn_idx].data);

                            stack[stack_size() - arg_count - 1] = callee;
                            stack[stack_size() - arg_count - 2] = method_closure;
                            int total_args = arg_count + 1;
                            if (fn.arity != total_args) {
                                runtime_error(RuntimeError::ArgumentError, fmt::format("Expected {} arguments, got {}.", fn.arity, total_args));
                                return;
                            }
                            sync_ip();
                            CallFrame new_frame;
                            new_frame.closure = closure_idx;
                            new_frame.ip = 0;
                            new_frame.stack_base = stack_size() - total_args;
                            if (frame_count == frame_capacity) [[unlikely]] {
                                runtime_error(RuntimeError::Runtime, "Stack overflow."); 
                                return;
                            }
                            frames[frame_count++] = new_frame;
                            frame = &frames[frame_count - 1];
                            sync_frame_cache(frame, fn, instr_data, ip, ip_end, const_pool, const_pool_size);
                        } else {
                            std::string method_name = std::get<std::string>(heap[name_val.as_ref].data);
                            runtime_error(RuntimeError::Name, fmt::format("Method '{}' not found.", method_name));
                            return;
                        }
                    } else {
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
                    
                    switch (container.tag) {
                        case Value::Tag::ArrayRef: { 
                            if (index.tag != Value::Tag::Int) {
                                runtime_error(RuntimeError::Type, fmt::format("Array index must be an integer, got '{}'.", index.tag_to_string()));
                                return;
                            }
                            auto& vec = std::get<ArrayType>(heap[container.as_ref].data);
                            if (index.as_int < 0 || index.as_int >= static_cast<int64_t>(vec.size())) {
                                runtime_error(RuntimeError::Index, fmt::format("Array index {} out of bounds for size {}.", index.as_int, vec.size()));
                                return;
                            }
                            set_top(vec[index.as_int]);
                            break;
                        }
                        case Value::Tag::StringRef: {
                            if (index.tag != Value::Tag::Int) {
                                runtime_error(RuntimeError::Type, fmt::format("Array index must be an integer, got '{}'.", index.tag_to_string()));
                                return;
                            }
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
                        case Value::Tag::MapRef: { 
                            if (!is_hashable(index)) { 
                                runtime_error(RuntimeError::Type, fmt::format("Unhashable type '{}' cannot be used as a map key.", index.tag_to_string()));
                                return;
                            }
                            auto& map = std::get<MapType>(heap[container.as_ref].data);
                            auto it = map.find(index);
                            if (it != map.end()) {
                                set_top(it->second);
                            } else {
                                sync_ip();
                                runtime_error(RuntimeError::Index, "Key not found in map.");
                                return;
                            }
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
                    Value container = peek(3);

                    if (container.tag == Value::Tag::ArrayRef) {
                        if (index.tag != Value::Tag::Int) {
                            runtime_error(RuntimeError::Type, fmt::format("Array index must be an integer, got '{}'.", index.tag_to_string()));
                            return;
                        }
                        auto& vec = std::get<ArrayType>(heap[container.as_ref].data);
                        if (index.as_int < 0 || index.as_int >= static_cast<int64_t>(vec.size())) {
                            runtime_error(RuntimeError::Index, fmt::format("Array index {} out of bounds for size {}.", index.as_int, vec.size()));
                            return;
                        }
                        vec[index.as_int] = value;
                        sp -= 2;
                        set_top(value);

                    } else if (container.tag == Value::Tag::MapRef) {
                        if (!is_hashable(index)) { 
                            runtime_error(RuntimeError::Type, fmt::format("Unhashable type '{}' cannot be used as a map key.", index.tag_to_string()));
                            return;
                        }
                        auto& map = std::get<MapType>(heap[container.as_ref].data);
                        map[index] = value;
                        sp -= 2;
                        set_top(value);

                    } else {
                        runtime_error(RuntimeError::Type, fmt::format("Cannot set index on non-array/map type '{}'.", container.tag_to_string()));
                        return;
                    }
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

                OP(NEW_RANGE) {
                    Value end = pop();
                    Value start = peek();

                    if (start.tag != Value::Tag::Int || end.tag != Value::Tag::Int) {
                        sync_ip();
                        runtime_error(RuntimeError::Type, "Range boundaries must be integers.");
                        return;
                    }

                    HeapIdx idx = alloc(Object(Range{start.as_int, end.as_int}));
                    set_top(Value(Value::Tag::RangeRef, idx));
                }
                DISPATCH(); 

                OP(GET_ITER) {
                    Value container = pop();
                    if (container.tag != Value::Tag::ArrayRef && 
                        container.tag != Value::Tag::StringRef &&
                        container.tag != Value::Tag::RangeRef 
                    ) {
                        runtime_error(RuntimeError::Type, "Object is not iterable");
                        return;
                    }
                    HeapIdx idx = alloc(Object(Iterator{container, 0}));
                    push(Value(Value::Tag::IteratorRef, idx));
                }
                DISPATCH();
                
                OP(FOR_ITER) {
                    Value iter_val = peek(); 
                    Iterator& iter = std::get<Iterator>(get_heap_object(iter_val.as_ref).data);
                    Object& container_obj = get_heap_object(iter.container.as_ref);
                    
                    if (auto* array_ptr = std::get_if<ArrayType>(&container_obj.data)) {
                        if (iter.index < array_ptr->size()) {
                            push((*array_ptr)[iter.index++]); 
                        } else {
                            ip += ARG; 
                        }
                    } else if (auto* string_ptr = std::get_if<std::string>(&container_obj.data)) {
                        if (iter.index < string_ptr->size()) {
                            std::string char_str(1, (*string_ptr)[iter.index++]);
                            HeapIdx char_idx = intern_string(char_str);
                            push(Value(Value::Tag::StringRef, char_idx));
                        } else {
                            ip += ARG; 
                        }
                    } else if (auto* range_ptr = std::get_if<Range>(&container_obj.data)) {
                        int64_t current = range_ptr->start + iter.index;
                        if (current < range_ptr->end) {
                            push(Value(current));
                            iter.index++;
                        } else {
                            ip += ARG;
                        }
                    }
                }
                DISPATCH();

                OP(GET_FIELD) {
                    HeapIdx field_id = ARG; 
                    Value obj_val = pop();
                    if (obj_val.tag != Value::Tag::StructRef) {
                        sync_ip();
                        runtime_error(RuntimeError::Type, "Only structs have fields.");
                        return;
                    }
                    Struct& s = std::get<Struct>(heap[obj_val.as_ref].data);
                    StructType& type = std::get<StructType>(heap[s.type_idx].data);
                    size_t offset = type.get_offset(field_id);
                    if (offset == size_t(-1)) {
                        sync_ip();
                        runtime_error(RuntimeError::Name, "Struct has no field with that name.");
                        return;
                    }
                    push(s.fields[offset]);
                }
                DISPATCH();

                OP(SET_FIELD) {
                    HeapIdx field_id = ARG;
                    Value val = pop();
                    Value obj_val = pop();
                    if (obj_val.tag != Value::Tag::StructRef) {
                        sync_ip();
                        runtime_error(RuntimeError::Type, "Only structs have fields.");
                        return;
                    }
                    Struct& s = std::get<Struct>(heap[obj_val.as_ref].data);
                    StructType& type = std::get<StructType>(heap[s.type_idx].data);
                    size_t offset = type.get_offset(field_id);
                    if (offset == size_t(-1)) {
                        sync_ip();
                        runtime_error(RuntimeError::Name, "Struct has no field with that name.");
                        return;
                    }
                    s.fields[offset] = val;
                    push(val); 
                }
                DISPATCH();

                OP(NEW_MAP) {
                    int pair_count = static_cast<int>(ARG);
                    MapType map;
                    for (int i = 0; i < pair_count; i++) {
                        Value val = pop();
                        Value key = pop();

                        if (!is_hashable(key)) {
                            runtime_error(RuntimeError::Type, fmt::format("Unhashable type '{}' cannot be used as a map key.", key.tag_to_string()));
                            return;
                        }

                        map[key] = val;
                    }
                    HeapIdx idx = alloc(Object(std::move(map)));
                    push(Value(Value::Tag::MapRef, idx));
                }
                DISPATCH();

                OP(CALL_KW) {
                    int pair_count = ARG;
                    
                    if (stack_size() < pair_count * 2 + 1) { 
                        runtime_error(RuntimeError::StackUnderflow, "Stack underflow in CALL_KW."); 
                        return;
                    }
                    
                    Value callee = peek(pair_count * 2 + 1);
                    if (callee.tag != Value::Tag::StructTypeRef) {
                        runtime_error(RuntimeError::Type, "Keyword arguments only supported for struct instantiation."); 
                        return;
                    }
                    
                    StructType& type = std::get<StructType>(heap[callee.as_ref].data);
                    
                    HeapIdx fn_idx = type.special_methods[static_cast<size_t>(SpecialMethod::Init)];
                    bool has_init = (fn_idx != 0);

                    Struct instance;
                    instance.type_idx = callee.as_ref;
                    instance.fields.resize(type.field_names.size(), Value());
                    
                    for (int i = 0; i < pair_count; i++) {
                        Value val = pop();
                        Value key = pop();
                        size_t offset = type.get_offset(key.as_ref);
                        if (offset == size_t(-1)) {
                            runtime_error(RuntimeError::Name, "Invalid field name passed in named constructor."); 
                            return;
                        }
                        instance.fields[offset] = val;
                    }
                    
                    HeapIdx idx = alloc(Object(instance));
                    Value instance_val(Value::Tag::StructRef, idx);
                    set_top(instance_val);
                    
                    if (has_init) {
                        Function& fn = std::get<Function>(heap[fn_idx].data);
                        if (fn.arity != 1) { 
                            runtime_error(RuntimeError::ArgumentError, "_init must take 0 arguments when using named initialization."); 
                            return;
                        }
                        
                        HeapIdx closure_idx = build_closure_for_call(fn_idx, frame);
                        Value init_clos(Value::Tag::ClosureRef, closure_idx);
                        Function& fn_post_clos = std::get<Function>(heap[fn_idx].data);
                        
                        push(instance_val); 
                        
                        sync_ip();
                        CallFrame new_frame;
                        new_frame.closure = closure_idx;
                        new_frame.ip = 0;
                        new_frame.stack_base = stack_size() - 1;
                        frames[frame_count++] = new_frame;
                        frame = &frames[frame_count - 1];
                        sync_frame_cache(frame, fn_post_clos, instr_data, ip, ip_end, const_pool, const_pool_size);
                    }
                }
                DISPATCH();

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