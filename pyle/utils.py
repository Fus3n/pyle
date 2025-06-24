
from pyle.pyle_bytecode import Instruction, OpCode


def disassemble(bytecode_chunk: Instruction, constants) -> str:
    lines = []
    lines.append("\n--------- Constants ---------")
    if constants:
        for i, const_val in enumerate(constants):
            lines.append(f"{i:04}: {const_val!r} (type: {type(const_val).__name__})")
    else:
        lines.append("Constants list is empty.")

    lines.append("\n--------- Disassembled Bytecode ---------")
    if bytecode_chunk:
        for i, instruction in enumerate(bytecode_chunk):
            opcode_name = instruction.opcode.name
            line = f"{i:04}: {opcode_name:<18}"

            if instruction.operand is not None:
                operand_val = instruction.operand
                line += f" {operand_val!s:<5}" 

                if instruction.opcode in (
                    OpCode.OP_CONST, 
                    OpCode.OP_DEF_GLOBAL, 
                    OpCode.OP_GET_GLOBAL, 
                    OpCode.OP_SET_GLOBAL
                ):
                    if 0 <= operand_val < len(constants):
                        line += f" ({constants[operand_val]!r})"
                    else:
                        line += " (INVALID CONSTANT INDEX)"
            print(line)
            lines.append(line)
    else:
        lines.append("Bytecode chunk is empty.")

    return "\n".join(lines)