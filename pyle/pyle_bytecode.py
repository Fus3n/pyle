import enum
from enum import auto
from dataclasses import dataclass
from .pyle_types import Token
from typing import Callable, Any # Import Callable, Any

class OpCode(enum.Enum):
    OP_CONST = auto()
    OP_DEF_GLOBAL = auto()
    OP_GET_GLOBAL = auto()
    OP_SET_GLOBAL = auto()
    OP_DEF_LOCAL = auto()
    OP_GET_LOCAL = auto()
    OP_SET_LOCAL = auto()

    OP_ADD = auto()
    OP_SUBTRACT = auto()
    OP_MULTIPLY = auto()
    OP_DIVIDE = auto()
    OP_MODULO = auto()

    OP_EQUAL = auto()
    OP_NOT_EQUAL = auto()
    OP_GREATER = auto()
    OP_GREATER_EQUAL = auto()
    OP_LESS = auto()
    OP_LESS_EQUAL = auto()

    OP_AND = auto()
    OP_OR = auto()
    
    OP_TRUE = auto()
    OP_FALSE = auto()
    OP_NONE = auto()

    OP_BUILD_RANGE = auto()
    OP_ITER_NEW = auto()
    OP_ITER_NEXT_OR_JUMP = auto()
    OP_BUILD_LIST = auto()

    OP_ENTER_SCOPE = auto()
    OP_EXIT_SCOPE = auto()
    
    OP_JUMP_IF_FALSE = auto()
    OP_JUMP = auto()
    OP_POP = auto()
    
    # --- Functions ---
    OP_CALL = auto()
    OP_RETURN = auto()
    
    OP_HALT = auto()

@dataclass
class Instruction:
    opcode: OpCode
    operand: int | float | str | None = None
    token: Token | None = None

@dataclass(frozen=True, slots=True)
class PyleFunction:
    name: str
    arity: int 
    start_ip: int | None = None  # IP for Pyle-defined functions, None for native
    native_fn: Callable[..., Any] | None = None # Python callable for native functions

    def __repr__(self):
        fn_type = "native" if self.native_fn else "pyle"
        return f"<fn {self.name}/{self.arity} ({fn_type})>"