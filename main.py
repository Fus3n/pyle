from pyle import Lexer, Parser, Compiler, PyleVM
from pprint import pprint
import json
import enum
import pickle

from pyle.pyle_bytecode import OpCode

def json_serial_default(obj):
    """JSON serializer for objects not serializable by default json code"""
    if isinstance(obj, enum.Enum):
        return obj.name  
    raise TypeError(f"Object of type {obj.__class__.__name__} is not JSON serializable")


def main():
    FILE_NAME = "./examples/source.pyle"
    source = open(FILE_NAME, "r").read()

    lexer = Lexer(FILE_NAME, source)
    res = lexer.tokenize()
    if res.is_err():
        pprint(res.err_val)
        return
    
    # pprint(res.ok_val)
    parser = Parser(res.ok_val)
    ast_res = parser.parse()
    
    if ast_res.is_err():
        print("Error while parsing")
        pprint(ast_res.err_val)
        return

    ast_dict = ast_res.ok_val.get_dict()
    # pprint(ast_dict, indent=2, depth=4)

    with open("ast_dump.json", "w", encoding="utf-8") as f:
        json.dump(ast_dict, f, default=json_serial_default, indent=2)

    ast_root_node = ast_res.ok_val 
    

    compiler = Compiler()
    try:
        bytecode_chunk, constants = compiler.compile(ast_root_node)
    except NotImplementedError as e:
        print(f"Compilation Error: {e}")
        return
    except Exception as e:
        print(f"Unexpected Compilation Error: {e}")
        return

    # pprint(bytecode_chunk)

    # --- DISASSEMBLER SECTION ---
    print("\n--------- Constants ---------")
    if constants:
        for i, const_val in enumerate(constants):
            print(f"{i:04}: {const_val!r} (type: {type(const_val).__name__})")
    else:
        print("Constants list is empty.")

    print("\n--------- Disassembled Bytecode ---------")
    lines = []
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
        print("Bytecode chunk is empty.")

    with open("source.pyled", "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    # --- END DISASSEMBLER SECTION ---

    compiled = (bytecode_chunk, constants)
    with open("source.pylec", "wb") as f:
        pickle.dump(compiled, f)

    # with open("source.pylec", "rb") as f:
    #     bytecode_chunk, constants = pickle.load(f)
    
    vm = PyleVM()
    try:
        print("\n--- VM Execution ---")
        result = vm.interpret(bytecode_chunk, constants)
        if result.is_ok():
            print("VM Result:", result.ok_val)
        elif result.is_err():
            print(result.err_val.msg)
        else:
            print("VM Execution finished with no explicit result on stack.")
    except RuntimeError as e:
        print(f"VM Runtime Error: {e}")


if __name__ == "__main__":
    main()