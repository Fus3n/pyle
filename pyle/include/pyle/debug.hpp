#pragma once

#include <fmt/core.h>
#include "pyle/bytecode.hpp"
#include "pyle/ast.hpp"


namespace pyle {
    inline void disassemble_chunk(VM& vm, const Chunk& chunk, std::string_view name) {
        fmt::print("== {} ==\n", name);

        for (size_t offset = 0; offset < chunk.instr.size(); offset++) {
            uint32_t instr = chunk.instr[offset];
            OpCode op = get_op(instr);
            uint32_t operand = get_operand(instr);
            size_t line = chunk.lines[offset];

            if (offset > 0 && line == chunk.lines[offset - 1]) {
                fmt::print("   | ");
            } else {
                fmt::print("{:4} ", line);
            }

            fmt::print("{:04d} ", offset);

            switch (op) {
                case OpCode::LOAD_CONST: {
                    std::string val_str = vm.value_to_string(chunk.const_pool[operand]);
                    fmt::print("{:16} {:4} '{}'\n", "LOAD_CONST", operand, val_str);
                    break;
                }
                case OpCode::ADD:  fmt::print("ADD\n"); break;
                case OpCode::SUB:  fmt::print("SUB\n"); break;
                case OpCode::MUL:  fmt::print("MUL\n"); break;
                case OpCode::DIV:  fmt::print("DIV\n"); break;
                case OpCode::HALT: fmt::print("HALT\n"); break;
                case OpCode::POP:   fmt::print("POP\n"); break;
                case OpCode::SET_GLOBAL: {
                    std::string val_str = vm.value_to_string(chunk.const_pool[operand]);
                    fmt::print("{:16} {:4} '{}'\n", "SET_GLOBAL", operand, val_str);
                    break;
                }
                case OpCode::LOAD_GLOBAL: {
                    std::string val_str = vm.value_to_string(chunk.const_pool[operand]);
                    fmt::print("{:16} {:4} '{}'\n", "LOAD_GLOBAL", operand, val_str);
                    break;
                }
                case OpCode::CALL: {
                    fmt::print("{:16} {:4} (args)\n", "CALL", operand);
                    break;
                }
                case OpCode::LOAD_LOCAL: fmt::print("{:16} {:4}\n", "LOAD_LOCAL", operand); break;
                case OpCode::SET_LOCAL:  fmt::print("{:16} {:4}\n", "SET_LOCAL", operand); break;
                case OpCode::DEFINE_GLOBAL: {
                    std::string val_str = vm.value_to_string(chunk.const_pool[operand]);
                    fmt::print("{:16} {:4} '{}'\n", "DEFINE_GLOBAL", operand, val_str);
                    break;
                }
                case OpCode::EQ: fmt::print("EQ\n"); break;
                case OpCode::NEQ: fmt::print("NEQ\n"); break;
                case OpCode::LT: fmt::print("LT\n"); break;
                case OpCode::LTE: fmt::print("LTE\n"); break;
                case OpCode::GT: fmt::print("GT\n"); break;
                case OpCode::GTE: fmt::print("GTE\n"); break;
                case OpCode::JUMP_IF_FALSE: fmt::print("{:16} {:4}\n", "JUMP_IF_FALSE", operand); break;
                case OpCode::JUMP_IF_TRUE: fmt::print("{:16} {:4}\n", "JUMP_IF_TRUE", operand); break;
                case OpCode::JUMP:          fmt::print("{:16} {:4}\n", "JUMP", operand); break;
                case OpCode::LOOP:          fmt::print("{:16} {:4}\n", "LOOP", operand); break;
                case OpCode::CALL_METHOD: fmt::print("{:16} {:4} (args)\n", "CALL_METHOD", operand); break;
                case OpCode::LOAD_GLOBAL_SLOT:   fmt::print("{:16} {:4}\n", "LOAD_GLOBAL_SLOT", operand); break;
                case OpCode::SET_GLOBAL_SLOT:    fmt::print("{:16} {:4}\n", "SET_GLOBAL_SLOT", operand); break;
                case OpCode::DEFINE_GLOBAL_SLOT: fmt::print("{:16} {:4}\n", "DEFINE_GLOBAL_SLOT", operand); break;
                case OpCode::SET_LOCAL_POP:  fmt::print("{:16} {:4}\n", "SET_LOCAL_POP", operand); break;
                case OpCode::SET_GLOBAL_SLOT_POP:  fmt::print("{:16} {:4}\n", "SET_GLOBAL_SLOT_POP", operand); break;
                default: fmt::print("UNKNOWN OPCODE\n"); break;
            }
        }
        fmt::print("\n");
    }

}