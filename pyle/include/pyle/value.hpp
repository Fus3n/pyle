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

namespace pyle {

    using HeapIdx = size_t;
    class VM;

    struct Value {
        enum class Tag {
            Int, Float, Bool, None, StringRef, ArrayRef, StructRef,
            NativeFuncRef, FuncRef, IteratorRef, RangeRef, ClosureRef, UpvalueRef,
            StructTypeRef,
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
                   t == Tag::NativeFuncRef ||
                   t == Tag::FuncRef ||
                   t == Tag::IteratorRef ||
                   t == Tag::RangeRef ||
                   t == Tag::ClosureRef);
        }

        std::string tag_to_string() const {
            switch (tag) {
                case Tag::Int:
                    return "int";
                case Tag::Float:
                    return "float";
                case Tag::Bool:
                    return "bool";
                case Tag::None:
                    return "nil";
                case Tag::StringRef:
                    return "string";
                case Tag::ArrayRef:
                    return "array";
                case Tag::StructRef:
                    return "struct";
                case Tag::NativeFuncRef:
                    return "native_function";
                case Tag::FuncRef:
                    return "function";
                case Tag::IteratorRef:
                    return "iterator";
                case Tag::RangeRef:
                    return "range";
                default:
                    return fmt::format("HeapRef({})", as_ref);
            }
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

    struct StructType {
        std::vector<HeapIdx> field_names; 
        ankerl::unordered_dense::map<HeapIdx, size_t> field_to_offset;
        std::vector<Value> default_values;

        ankerl::unordered_dense::map<HeapIdx, Value> methods; 
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
            Upvalue
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
        explicit Object(StructType strt) : data(std::move(strt)) {} // <-- ADD THIS
        explicit Object(Struct strc)     : data(std::move(strc)) {}
    };

}
