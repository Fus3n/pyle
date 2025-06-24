from __future__ import annotations
from dataclasses import dataclass, field, asdict
from .pyle_types import Token

@dataclass(slots=True, kw_only=True)
class ASTNode:
    token: Token | None = None
    type: str = field(init=False) 

    def __post_init__(self):
        """Initializes the 'type' field to the lowercase name of the class."""
        class_name = self.__class__.__name__
        if not class_name:
            self.type = ""
            return
        
        s = class_name
        self.type = ''.join(['_' + c.lower() if i > 0 and c.isupper() else c.lower() for i, c in enumerate(s)])

    def get_type_name(self) -> str:
        return type(self).__name__

    def get_dict(self):
        def dict_factory_skip_nones(items):
            return {k: v for k, v in items if v is not None}
        return asdict(self, dict_factory=dict_factory_skip_nones)

@dataclass(slots=True)
class Expr(ASTNode):
    pass

@dataclass(slots=True)
class Stmt(ASTNode):
    pass

@dataclass(slots=True)
class VarDeclareStmt(Stmt):
    name: Token
    initializer: Expr
    is_const: bool = False
@dataclass(slots=True)
class AssignStmt(Stmt):
    name: Token
    value: Expr
@dataclass(slots=True)
class VariableExpr(Expr):
    name: Token

@dataclass(slots=True) # Uncommented and using Optional
class IfStmt(Stmt):
    condition: Expr
    then_branch: Block # Assuming Block is a list of statements
    else_branch: Block | None = None

@dataclass(slots=True)
class Block(ASTNode):
    statements: list[Stmt | Expr] = field(default_factory=list)
    token: Token | None = None

@dataclass(slots=True)
class Number(Expr):
    value: int | float

@dataclass(slots=True)
class String(Expr):
    value: str

@dataclass(slots=True)
class UnaryOp(Expr):
    op: Token       # The operator token (e.g., '-', 'not')
    operand: Expr   # The expression the operator applies to

@dataclass
class ArrayLiteral(Expr): # Or whatever your base expression class is
    elements: list[Expr]      # List of expression nodes for each element

@dataclass(slots=True)
class Boolean(Expr):
    value: bool
    
@dataclass(slots=True)
class BinaryOp(Expr):
    left: Expr
    op: Token
    right: Expr

@dataclass(slots=True)
class LogicalOp(Expr):
    left: Expr
    op: Token
    right: Expr

@dataclass(slots=True)
class ComparisonOp(Expr):
    left: Expr
    op: Token
    right: Expr

@dataclass(slots=True)
class WhileStmt(Stmt):
    condition: Expr
    body: Block 

@dataclass(slots=True)
class BreakStmt(Stmt):
    pass

@dataclass(slots=True)
class ContinueStmt(Stmt):
    pass

@dataclass(slots=True)
class RangeSpecifier(ASTNode):
    start: Expr
    end: Expr
    step: Expr | None = None

@dataclass(slots=True)
class ForInStmt(Stmt):
    loop_variable: Token
    iterable: Expr
    body: Block

@dataclass(slots=True)
class FunctionExpr(Expr):
    params: list[Token]
    body: Block


@dataclass(slots=True)
class FunctionDefStmt(Stmt):
    name: Token
    params: list[Token]
    body: Block

@dataclass(slots=True)
class KeywordArg:
    name: Token
    value: Expr

@dataclass(slots=True)
class CallExpr(Expr):
    callee: Expr
    arguments: list[Expr]
    keywords: list[KeywordArg]
    token: Token

@dataclass(slots=True)
class IndexExpr(Expr):
    collection: Expr
    index: Expr

@dataclass(slots=True)
class AssignIndexStmt(Stmt):
    collection: Expr
    index: Expr
    value: Expr
@dataclass(slots=True)
class ReturnStmt(Stmt):
    value: Expr | None = None

@dataclass(slots=True)
class DotExpr(Expr):
    object: Expr
    attr: Token
