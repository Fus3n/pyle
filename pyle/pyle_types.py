from dataclasses import dataclass
from enum import Enum, auto
from typing import NewType

InternedStr = NewType("InternedStr", str)


KEYWORD_CONSTANTS = (
    "and",
    "or",
    "not",
    "for",
    "in",
    "if",
    "else",
    "let",
    "true",
    "false",
    "while",
    "fn",      # Added
    "return",   # Added comma
    "break",
    "continue",
    "const"
)

# Types
class TokenType(Enum):
    INT = auto()
    FLOAT = auto()
    STRING = auto()

    KEYWORD = auto()
    IDENT = auto()
    DOT = auto()

    L_PAREN = auto()
    R_PAREN = auto()
    L_BRACKET = auto()
    R_BRACKET = auto()
    L_CURLY_BRACE = auto()
    R_CURLY_BRACE = auto()
    L_SQ_BRACKET = auto()  # [
    R_SQ_BRACKET = auto()  # ]
    COMMA = auto()         # ,

    PLUS = auto()
    MINUS = auto()
    MUL = auto()
    DIV = auto()
    MOD = auto()
    EOF = auto()
    ERROR = auto()

    SEMICOLON = auto()
    COLON = auto()

    GT = auto() 
    "Greater than (>)"

    GTE = auto() 
    "Greater than or equal (>=)"

    LT = auto()
    "Less than (<)"

    LTE = auto()
    "Less than or equal (<=)"

    EQ = auto()
    "Equal To (==)"


    BANG = auto()
    "!"

    NEQ = auto()
    "Not equal to (!=)"

    ASSIGN = auto()

    "Assign operatior (=)"


@dataclass(frozen=True, slots=True)
class Loc:
    """
    File location dataclass
    """
    line: int
    col_start: int
    col_end: int | None = None

    def copy_with(self, line: int = None, col_start: int = None, col_end: int = None):
        return Loc(
            line=self.line if line is None else line,
            col_start=self.col_start if col_start is None else col_start,
            col_end=self.col_end if col_end is None else col_end
        )

@dataclass(frozen=True, slots=True)
class Token:
    kind: TokenType
    value: str
    loc: Loc
    source_name: InternedStr = ""

    def get_file_loc(self):
        return f"{self.source_name}:{self.loc.line}:{self.loc.col_start}" # Added line

    def cmp(self, ttype: TokenType, val: str | None = None):
        kind_cmp = self.kind == ttype
        if val: 
            return kind_cmp and self.value == val
        return kind_cmp 

    def is_keyword(self, value: str):
        return self.kind == TokenType.KEYWORD and self.value == value
    
    def copy_with(
        self,
        kind: TokenType = None,
        value: str = None,
        loc: Loc = None,
        source_name: 'InternedStr' = None
    ):
        return Token(
            kind=self.kind if kind is None else kind,
            value=self.value if value is None else value,
            loc=self.loc if loc is None else loc,
            source_name=self.source_name if source_name is None else source_name
        )

    def copy(self):
        return Token(
            self.kind,
            self.value,
            self.loc,
            self.source_name
        )
    
