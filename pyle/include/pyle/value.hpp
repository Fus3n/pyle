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
            Int, Float, Bool, Null, StringRef, ArrayRef, StructRef,
            NativeFuncRef, FuncRef
        } tag;
        union {
            int64_t as_int;
            double as_float;
            bool as_bool;
            HeapIdx as_ref;
        };

        Value() : tag(Tag::Null), as_ref(0) {}

        explicit Value(int64_t val) : tag(Tag::Int), as_int(val) {}

        explicit Value(double val) : tag(Tag::Float), as_float(val) {}

        explicit Value(bool val) : tag(Tag::Bool), as_bool(val) {}

        Value(Tag t, HeapIdx ref) : tag(t), as_ref(ref) {
            assert(t == Tag::StringRef ||
                   t == Tag::ArrayRef ||
                   t == Tag::StructRef ||
                   t == Tag::NativeFuncRef ||
                   t == Tag::FuncRef);
        }

        std::string tag_to_string() const {
            switch (tag) {
                case Tag::Int:
                    return "int";
                case Tag::Float:
                    return "float";
                case Tag::Bool:
                    return "bool";
                case Tag::Null:
                    return "null";
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

    using StructType = ankerl::unordered_dense::map<std::string, Value>;
    using ArrayType = std::vector<Value>;
    using NativeFn = Value (*)(VM& vm, ArgView args);

    struct Function {
        std::string name;
        int arity = 0;
        Chunk chunk;
    };
    
    struct Object {
        bool gc_marked = false;
        std::variant<
            std::monostate,
            std::string,
            ArrayType,
            StructType,
            NativeFn,
            Function
        > data;

        Object() = default;
        explicit Object(std::string str) : data(std::move(str)) {}
        explicit Object(ArrayType arr)   : data(std::move(arr)) {}
        explicit Object(StructType strc) : data(std::move(strc)) {}
        explicit Object(NativeFn fn)   : data(fn) {}
        explicit Object(Function func): data(std::move(func)) {}
    };

}
