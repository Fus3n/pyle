from .pyle_types import Token
from dataclasses import dataclass
from typing import Any, Generic, TypeVar, Optional

T = TypeVar('T')

class InterpreterError:
    
    def __init__(self, msg: str, token: Token):
        self.msg = msg
        self.tok = token

    def __str__(self) -> str:
        return f"{self.__class__.__name__}: {self.msg}' -> {self.tok.get_file_loc()}"       

    def __repr__(self) -> str:
        return self.__str__()

class LexerError(InterpreterError):
    pass

class ParseError(InterpreterError):
    pass

class PyleRuntimeError(InterpreterError):
    pass

@dataclass
class Result(Generic[T]):
    ok_val: Optional[T] = None
    err_val: Optional[InterpreterError] = None

    @staticmethod
    def ok(value: T) -> "Result[T]":
        return Result(ok_val=value)

    @staticmethod
    def err(error: InterpreterError) -> "Result[T]":
        if not isinstance(error, InterpreterError):
            raise TypeError("Expected InterpreterError")
        return Result(err_val=error)

    def is_err(self) -> bool:
        return self.err_val is not None

    def is_ok(self) -> bool:
        return self.ok_val is not None

    def __str__(self) -> str:
        if self.is_ok():
            return f"Ok({self.ok_val})"
        return f"Err({self.err_val})"