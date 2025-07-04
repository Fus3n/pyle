from dataclasses import dataclass 
from pyle.pyle_errors import PyleRuntimeError, Result
from .pyle_bytecode import OpCode, Instruction, PyleFunction, Variable, Range
from .pyle_types import Token 
from types import MethodType, BuiltinFunctionType, FunctionType

@dataclass
class CallFrame:
    return_ip: int
    stack_slot: int
    env_depth: int  

class PyleVM:
    
    def __init__(self) -> None:
        self.bytecode_chunk: list[Instruction] = []
        self.constants: list[any] = []
        self.ip: int = 0
        self.stack: list[any] = []
        self.globals: dict[str, Variable] = {} 
        self.environments: list[dict[str, Variable]] = [] # For lexical scoping
        self.frames: list[CallFrame] = [] # Call stack

    def interpret(self, bytecode_chunk: list[Instruction], constants: list[any]):
        self.bytecode_chunk = bytecode_chunk
        self.constants = constants

        # reset
        self.ip = 0
        self.stack = []
        self.globals = {} 
        self.environments = []
        self.frames = [] 

        return self._run()
    
    def _push(self, value):
        self.stack.append(value)
    
    def _pop(self):
        if not self.stack:
            raise PyleRuntimeError("VM stack underflow during pop.", None) 
        return self.stack.pop()

    def _is_falsey(self, value) -> bool:
        return not bool(value)

    def _current_instruction(self) -> Instruction | None:
        if 0 <= self.ip < len(self.bytecode_chunk):
            return self.bytecode_chunk[self.ip]
        return None
    
    def _get_token_for_current_instruction(self, compiler_token_map: dict[int, Token]) -> Token | None:
        return compiler_token_map.get(self.ip -1 if self.ip > 0 else 0)

    def _set_variable(self, store, var_name: str, value, is_const=False):
        store[var_name] = Variable(var_name, value, is_const)

    def _run(self):

        while True:
            if self.ip >= len(self.bytecode_chunk):
                print("--- VM Execution Finished (IP out of bounds, no RETURN/HALT) ---")
                break 

            current_instr_obj = self._current_instruction()
            if current_instr_obj is None: # Should not happen if ip < len
                print("--- VM Execution Error: Current instruction is None ---")
                break
            
            # For error reporting, the VM should use the token associated with the current instruction, which requires the compiler to provide a token_map (not yet implemented here).
            current_token: Token | None = None # Placeholder

            # print(f"IP: {self.ip:04} About to execute: {current_instr_obj.opcode.name}" + \
            #       (f" {current_instr_obj.operand}" if current_instr_obj.operand is not None else "") + \
            #       f" Stack: {self.stack} Frames: {len(self.frames)} Envs: {len(self.environments)}")

            self.ip += 1 
            op = current_instr_obj.opcode
            operand = current_instr_obj.operand
            

            if op == OpCode.OP_CONST:
                self._push(self.constants[operand])
            
            #region Global scope
            elif op == OpCode.OP_DEF_GLOBAL:
                var_name = self.constants[operand]
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow for OP_DEF_GLOBAL '{var_name}'.", current_token))
                self._set_variable(self.globals, var_name, self._pop())
            elif op == OpCode.OP_GET_GLOBAL:
                var_name = self.constants[operand]
                if var_name not in self.globals:
                    return Result.err(PyleRuntimeError(f"Undefined global variable '{var_name}'.", current_token))
                var_val = self.globals[var_name]
                self._push(var_val.value)
            elif op == OpCode.OP_SET_GLOBAL:
                var_name = self.constants[operand]
                if var_name not in self.globals:
                    return Result.err(PyleRuntimeError(f"Cannot assign to undefined global variable '{var_name}'.", current_token))
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow for OP_SET_GLOBAL '{var_name}'.", current_token))
                if self.globals[var_name].is_const:
                    return Result.err(PyleRuntimeError(f"Cannot assign to const global variable '{var_name}'.", current_token))
                self._set_variable(self.globals, var_name, self._pop())
            elif op == OpCode.OP_DEF_CONST_GLOBAL:
                var_name = self.constants[operand]
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow for OP_DEF_GLOBAL '{var_name}'.", current_token))
                self._set_variable(self.globals, var_name, self._pop(), is_const=True)
            #endregion

            #region Arithmetic
            elif op == OpCode.OP_ADD:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_ADD.", current_token))
                right = self._pop(); left = self._pop()
                if isinstance(left, (int, float)) and isinstance(right, (int, float)):
                    self._push(left + right)
                elif isinstance(left, str) and isinstance(right, str):
                    self._push(left + right)
                elif isinstance(left, list) and isinstance(right, list):
                    self._push(left + right) # concatenation
                else:
                    return Result.err(PyleRuntimeError(f"Unsupported operand types for +: {type(left).__name__} and {type(right).__name__}", current_token))
            elif op == OpCode.OP_SUBTRACT:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_SUBTRACT.", current_token))
                right = self._pop(); left = self._pop(); self._push(left - right)
            elif op == OpCode.OP_MULTIPLY:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_MULTIPLY.", current_token))
                right = self._pop(); left = self._pop(); self._push(left * right)
            elif op == OpCode.OP_DIVIDE:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_DIVIDE.", current_token))
                right = self._pop(); left = self._pop()
                if right == 0: return Result.err(PyleRuntimeError("Division by zero.", current_token))
                self._push(left / right) 
            elif op == OpCode.OP_MODULO:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_MODULO.", current_token))
                right = self._pop(); left = self._pop()
                if right == 0: return Result.err(PyleRuntimeError("Modulo by zero.", current_token))
                self._push(left % right) 
            #endregion
            
            #region Unary Operations
            elif op == OpCode.OP_NEGATE:
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_NEGATE.", current_token))
                value = self._pop()
                if isinstance(value, (int, float)):
                    self._push(-value)
                else:
                    return Result.err(PyleRuntimeError(f"Operand for '-' must be a number, not {type(value).__name__}.", current_token))
            elif op == OpCode.OP_NOT:
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_NOT.", current_token))
                value = self._pop()
                self._push(not value) 
            #endregion

            #region Comparison
            elif op == OpCode.OP_EQUAL:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_EQUAL.", current_token))
                right = self._pop(); left = self._pop(); self._push(left == right)
            elif op == OpCode.OP_NOT_EQUAL:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_NOT_EQUAL.", current_token))
                right = self._pop(); left = self._pop(); self._push(left != right)
            elif op == OpCode.OP_GREATER:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_GREATER.", current_token))
                right = self._pop(); left = self._pop(); self._push(left > right)
            elif op == OpCode.OP_GREATER_EQUAL:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_GREATER_EQUAL.", current_token))
                right = self._pop(); left = self._pop(); self._push(left >= right)
            elif op == OpCode.OP_LESS:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_LESS.", current_token))
                right = self._pop(); left = self._pop(); self._push(left < right)
            elif op == OpCode.OP_LESS_EQUAL:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_LESS_EQUAL.", current_token))
                right = self._pop(); left = self._pop(); self._push(left <= right)
            #endregion

            #region Logical Comp
            elif op == OpCode.OP_AND: 
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_AND.", current_token))
                right = self._pop(); left = self._pop(); self._push(left and right) # Python's short-circuiting behavior
            elif op == OpCode.OP_OR:
                if len(self.stack) < 2: return Result.err(PyleRuntimeError("Stack underflow for OP_OR.", current_token))
                right = self._pop(); left = self._pop(); self._push(left or right) # Python's short-circuiting behavior
            #endregion

            elif op == OpCode.OP_TRUE: self._push(True)
            elif op == OpCode.OP_FALSE: self._push(False)
            elif op == OpCode.OP_NONE: self._push(None) 

            #region Range and iterators
            elif op == OpCode.OP_BUILD_RANGE:
                if len(self.stack) < 3: return Result.err(PyleRuntimeError("Stack underflow for OP_BUILD_RANGE.", current_token))
                step = self._pop(); end = self._pop(); start = self._pop()
                self._push(Range(start, end, step))
            elif op == OpCode.OP_ITER_NEW:
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_ITER_NEW.", current_token))
                iterable = self._pop()
                try: iterator = iter(iterable)
                except TypeError: return Result.err(PyleRuntimeError(f"Type '{type(iterable).__name__}' is not iterable.", current_token))
                self._push(iterator)
            elif op == OpCode.OP_ITER_NEXT_OR_JUMP:
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_ITER_NEXT_OR_JUMP.", current_token))
                iterator = self.stack[-1] 
                try:
                    value = next(iterator)
                    self._push(value)     
                except StopIteration:
                    self._pop() # Pop the exhausted iterator
                    self.ip = operand  # Jump to end of loop
                except TypeError: # If item on stack wasn't an iterator
                    return Result.err(PyleRuntimeError(f"Cannot iterate non-iterator type '{type(iterator).__name__}'.", current_token))
            #endregion

            #region List & Index
            elif op == OpCode.OP_BUILD_LIST:
                num_elements = operand
                if len(self.stack) < num_elements: return Result.err(PyleRuntimeError(f"Stack underflow for OP_BUILD_LIST: need {num_elements}, have {len(self.stack)}.", current_token))
                elements = []
                for _ in range(num_elements): elements.append(self._pop())
                elements.reverse(); self._push(elements)
            elif op == OpCode.OP_INDEX_GET:
                if len(self.stack) < 2:
                    return Result.err(PyleRuntimeError("Stack underflow for OP_INDEX_GET.", current_token))
                index = self._pop()
                collection = self._pop()
                try:
                    if isinstance(index, Range):
                        self._push(collection[index.start:index.end:index.step])
                    else:
                        self._push(collection[index])
                except (TypeError, IndexError, KeyError) as e:
                    return Result.err(PyleRuntimeError(f"Indexing error: {e}", current_token))
            elif op == OpCode.OP_INDEX_SET:
                if len(self.stack) < 3:
                    return Result.err(PyleRuntimeError("Stack underflow for OP_INDEX_SET.", current_token))
                value = self._pop()
                index = self._pop()
                collection = self._pop()
                if isinstance(collection, list):
                    try:
                        collection[index] = value
                        self._push(value) 
                    except (TypeError, IndexError) as e:
                        return Result.err(PyleRuntimeError(f"Index assignment error: {e}", current_token))
                else:
                    return Result.err(PyleRuntimeError(f"Index assignment not supported on type '{type(collection).__name__}'.", current_token))
            elif op == OpCode.OP_GET_ATTR:
                if not self.stack:
                    return Result.err(PyleRuntimeError("Stack underflow for OP_GET_ATTR.", current_token))
                obj = self._pop()
                attr_name = self.constants[operand]
                try:
                    self._push(getattr(obj, attr_name))
                except Exception as e:
                    return Result.err(PyleRuntimeError(f"Attribute error: {e}", current_token))
            #endregion

            elif op == OpCode.OP_ENTER_SCOPE:
                self.environments.append({})
            elif op == OpCode.OP_EXIT_SCOPE:
                if not self.environments:
                    return Result.err(PyleRuntimeError("VM error: Attempted to exit scope when no local scope active.", current_token))
                self.environments.pop()
            
            #region Local Scope
            elif op == OpCode.OP_DEF_LOCAL:
                var_name = self.constants[operand]
                if not self.environments: return Result.err(PyleRuntimeError(f"VM error: No active local scope to define '{var_name}'.", current_token))
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow defining local '{var_name}'.", current_token))
                
                current_scope = self.environments[-1]
                self._set_variable(current_scope, var_name, self._pop())
            elif op == OpCode.OP_DEF_CONST_LOCAL:
                var_name = self.constants[operand]
                if not self.environments: return Result.err(PyleRuntimeError(f"VM error: No active local scope to define '{var_name}'.", current_token))
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow defining local '{var_name}'.", current_token))
                
                current_scope = self.environments[-1]
                self._set_variable(current_scope, var_name, self._pop(), is_const=True)
            elif op == OpCode.OP_GET_LOCAL:
                var_name = self.constants[operand]
                found_in_locals = False
                for i_env in range(len(self.environments) - 1, -1, -1):
                    scope = self.environments[i_env]
                    if var_name in scope:
                        self._push(scope[var_name].value)
                        found_in_locals = True
                        break
                
                if not found_in_locals:
                    return Result.err(PyleRuntimeError(f"Undefined local variable '{var_name}'.", current_token))
           
            elif op == OpCode.OP_SET_LOCAL:
                var_name = self.constants[operand]
                if not self.stack: return Result.err(PyleRuntimeError(f"Stack underflow setting local '{var_name}'.", current_token))
                val_to_assign = self._pop() # Value is popped first
                assigned = False
                for i in range(len(self.environments) - 1, -1, -1):
                    scope = self.environments[i]
                    if var_name in scope:
                        # scope[var_name] = val_to_assign 
                        if scope[var_name].is_const:
                            return Result.err(PyleRuntimeError(f"Cannot Assign const local variable '{var_name}'.", current_token))
                        self._set_variable(scope, var_name, val_to_assign)
                        assigned = True
                        break
                if not assigned:
                    return Result.err(PyleRuntimeError(f"Cannot assign to undefined local variable '{var_name}'.", current_token))
            #endregion

            elif op == OpCode.OP_JUMP_IF_FALSE: 
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_JUMP_IF_FALSE.", current_token))
                condition_value = self._pop()
                if self._is_falsey(condition_value): self.ip = operand
            elif op == OpCode.OP_JUMP: 
                self.ip = operand
            elif op == OpCode.OP_POP: 
                if not self.stack: return Result.err(PyleRuntimeError("Stack underflow for OP_POP.", current_token))
                self._pop()

            #region Function Call
            elif op == OpCode.OP_CALL:
                num_args = operand
                
                if len(self.stack) < num_args + 1: # Args + function object
                    return Result.err(PyleRuntimeError(f"Stack underflow for OP_CALL: need {num_args+1} args + func, have {len(self.stack)}.", current_token))

                callee_candidate_idx = len(self.stack) - 1 - num_args
                callee_candidate = self.stack[callee_candidate_idx]

                # Calling python functions are currently supported.
                if not isinstance(callee_candidate, (PyleFunction, MethodType, BuiltinFunctionType, FunctionType, type)) :
                    return Result.err(PyleRuntimeError(f"Cannot call non-function type '{type(callee_candidate)}'.", current_token))
                
                function_to_call: PyleFunction | MethodType = callee_candidate

                if isinstance(function_to_call, (MethodType, BuiltinFunctionType, FunctionType, type)):
                    # Python functions/methods/classes
                    args_for_pyfunc = []
                    for _ in range(num_args):
                        args_for_pyfunc.append(self._pop())
                    args_for_pyfunc.reverse()
                    self._pop() 

                    try:
                        native_result = function_to_call(*args_for_pyfunc)
                        self._push(native_result)
                    except Exception as e:
                        return Result.err(PyleRuntimeError(f"Error in python function '{function_to_call.__name__}': {e}", current_token))

                elif function_to_call.native_fn:
                    if function_to_call.arity >= 0 and num_args != function_to_call.arity:
                         return Result.err(PyleRuntimeError(f"Native function '{function_to_call.name}' expected {function_to_call.arity} arguments, but got {num_args}.", current_token))

                    args_for_native_call = []
                    for _ in range(num_args):
                        args_for_native_call.append(self._pop())
                    args_for_native_call.reverse()
                    self._pop()  
                                        
                    try:
                        native_result = function_to_call.native_fn(self, *args_for_native_call)
                        self._push(native_result)
                    except Exception as e:
                        return Result.err(PyleRuntimeError(f"Error in native function '{function_to_call.name}': {e}", current_token))
                else: 
                    if function_to_call.start_ip is None: 
                        return Result.err(PyleRuntimeError(f"Pyle function '{function_to_call.name}' has no bytecode start IP.", current_token))

                    if num_args != function_to_call.arity:
                        return Result.err(PyleRuntimeError(f"Function '{function_to_call.name}' expected {function_to_call.arity} arguments, but got {num_args}.", current_token))

                    frame = CallFrame(return_ip=self.ip, stack_slot=callee_candidate_idx, env_depth=len(self.environments))
                    self.frames.append(frame)
                    self.ip = function_to_call.start_ip
            
            elif op == OpCode.OP_RETURN:
                if not self.stack: 
                    return Result.err(PyleRuntimeError("Stack underflow for OP_RETURN (no return value).", current_token))
                return_value = self._pop() 

                if not self.frames: 
                    return Result.ok(return_value)

                frame = self.frames.pop()
                self.ip = frame.return_ip

                while len(self.environments) > frame.env_depth:
                    self.environments.pop()

                # stack cleanup
                self.stack = self.stack[:frame.stack_slot]
                self._push(return_value)

            elif op == OpCode.OP_BUILD_KWARGS:
                num_kwargs = operand
                if len(self.stack) < num_kwargs + 1:
                    return Result.err(PyleRuntimeError("Stack underflow for OP_BUILD_KWARGS.", current_token))
                kw_names = self._pop() 
                kw_values = [self._pop() for _ in range(num_kwargs)]
                kw_values.reverse()
                kwargs = dict(zip(kw_names, kw_values))
                self._push(kwargs)

            elif op == OpCode.OP_CALL_KW:
                num_args, num_kwargs = operand
                if len(self.stack) < num_args + 2: return Result.err(PyleRuntimeError("Stack underflow for OP_CALL_KW.", current_token))
                kwargs = self._pop()
                args = [self._pop() for _ in range(num_args)]
                args.reverse()
                func = self._pop()
                try:
                    result = func(*args, **kwargs)
                    self._push(result)
                except Exception as e:
                    return Result.err(PyleRuntimeError(f"Error in function call with kwargs: {e}", current_token))
            
            #endregion
            
            elif op == OpCode.OP_HALT: 
                return Result.ok(self.stack[-1] if self.stack else None)
            else:
                return Result.err(PyleRuntimeError(f"Unknown opcode: {op.name}", current_token))

        return Result.ok(self.stack[-1] if self.stack else None)
