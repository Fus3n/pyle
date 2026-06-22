#pragma once
#include <cstdint>
#include <vector>
// #include "pyle/value.hpp"

namespace pyle {
    struct Value;

    enum class OpCode: uint8_t {
        LOAD_CONST,
        LOAD_LOCAL,
        SET_LOCAL,
        LOAD_GLOBAL_SLOT,
        SET_GLOBAL_SLOT,
        DEFINE_GLOBAL_SLOT,
        SET_LOCAL_POP,        
        SET_GLOBAL_SLOT_POP,  
        GET_ITER,
        FOR_ITER,  
        NEW_RANGE,          
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        NEG,
        EQ,
        NEQ,
        LT,
        LTE,
        GT,
        GTE,
        NOT,
        JUMP,
        JUMP_IF_FALSE,
        JUMP_IF_TRUE,
        LOOP,
        CALL,
        CALL_METHOD,
        RETURN,
        POP,
        NEW_STRUCT,
        GET_FIELD,
        SET_FIELD,
        NEW_ARRAY,
        GET_INDEX,
        SET_INDEX,
        HALT
    };

    inline uint32_t encode(OpCode op, const uint32_t operand = 0) {
        return (static_cast<uint32_t>(operand) << 8) | static_cast<uint32_t>(op);
    }
    inline OpCode   get_op (const uint32_t instr) { return static_cast<OpCode>(instr & 0xFF); }
    inline uint32_t get_operand (const uint32_t instr) { return instr >> 8; }

    struct Chunk {
        std::vector<uint32_t> instr;
        std::vector<Value>    const_pool;
        std::vector<size_t>   lines;
    };
}