from pyle import Lexer, Parser, Compiler, PyleVM
from pprint import pprint
import json
import enum
import pickle
import sys

from pyle.pyle_bytecode import OpCode
import pyle

def json_serial_default(obj):
    """JSON serializer for objects not serializable by default json code"""
    if isinstance(obj, enum.Enum):
        return obj.name  
    raise TypeError(f"Object of type {obj.__class__.__name__} is not JSON serializable")


# TODO: Implement break and continue in loops
# TODO: Function def should able to define with default arguments
# TODO: use token map to propagate error 

def main():
    args = sys.argv

    file_name = "./examples/source.pyle"

    if len(args) > 1:
        file_name = args[1]

    source = open(file_name, "r").read()

    lexer = Lexer(file_name, source)
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

    dis = pyle.disassemble(bytecode_chunk, constants)
    print(dis)

    with open("source.pyled", "w") as f:
        f.write(dis)

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