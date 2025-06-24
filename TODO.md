# TODO

1. Implement Lexer
    - [x] Lexer class
    - [x] peeking
    - [x] Handle basic data types
    - [x] Handle comparision operators
    - [x] Handle equality operators
    - [x] Handle arithmetic operators
    - [x] tokenize

2. Implement Parser
    - [x] AST
    - [x] Parser class
    - [x] Parse tokens from lexer
    - [x] peeking
    - [x] Handle basic data types
    - [x] Handle BinaryOp
    - [x] Handle LogicalOp
    - [x] Handle Comparision & Equality Operations
    - [x] Add `UnaryOp` to AST
    - [x] Add `ArrayLiteral` to AST
    - [x] Add `BreakStmt` and `ContinueStmt` to AST
    - [x] Add `KeywordArg` to AST
    - [x] Add `CallExpr` with keyword arguments support to AST
    - [x] Add `IndexExpr` and `AssignIndexStmt` to AST
    - [x] Add `ReturnStmt` to AST
    - [x] Add `DotExpr` to AST

3. Implement Compiler
    - [x] Basic compilation for expressions
    - [x] Global variable handling (DEF_GLOBAL, GET_GLOBAL, SET_GLOBAL)
    - [x] Local variable handling (DEF_LOCAL, GET_LOCAL, SET_LOCAL)
    - [x] Constant variable handling (DEF_CONST_GLOBAL, DEF_CONST_LOCAL)
    - [x] Bytecode for arithmetic operations
    - [x] Bytecode for unary operations (NEGATE, NOT)
    - [x] Bytecode for comparison and equality operations
    - [x] Bytecode for logical operations
    - [x] Bytecode for print/echo
    - [x] Bytecode for pop
    - [x] Bytecode for jump instructions (JUMP_IF_FALSE, JUMP)
    - [x] Bytecode for function calls (CALL, CALL_KW, BUILD_KWARGS, RETURN)
    - [x] Bytecode for indexing (INDEX_GET, INDEX_SET)
    - [x] Bytecode for attribute access (GET_ATTR)

4. Implement Errors
    - [x] Parser Errors
    - [x] Base Error classes
    - [x] Result class
    - [x] Result class for Lexer
    - [x] Handling Lexer Errors
    - [x] Handling Parser Errors
    - [ ] Stack trace
    - [ ] Use token map to propagate error (from main.py comment)

5. Implement Stack-based VM
    - [x] Basic VM structure
    - [x] Instruction pointer
    - [x] Stack operations (push, pop)
    - [x] Execute arithmetic opcodes
    - [x] Execute comparison and equality opcodes
    - [x] Execute logical opcodes
    - [x] Execute global variable opcodes
    - [x] Execute local variable opcodes
    - [x] Execute constant variable opcodes
    - [x] Execute unary opcodes
    - [x] Execute jump opcodes
    - [x] Execute call and return opcodes (including keyword arguments)
    - [x] Execute indexing opcodes
    - [x] Execute attribute access opcodes
    - [x] Builtin functions (`echo`, `len`, `scan`, `perf_counter`)
    - [x] `importpy` builtin function
    - [x] `get_attr` builtin function

6. CLI
    - [x] Accept filename as command line argument

7. VS Code Extension
    - [x] Basic keyword completion
    - [x] Basic builtin function completion
    - [x] Dynamic Python package completion for `importpy`
    - [x] User-defined function completion
    - [x] Variable completion
    - [x] Updated syntax highlighting for new keywords (`continue`, `break`, `fn`, `const`)

8. General Language Features
    - [ ] Implement break and continue in loops (from main.py comment)
    - [ ] Function def should be able to define with default arguments (from main.py comment)
    - [ ] Add `disassemble` utility function

Project Assumptions/Rules:
- TokenType or token type and token kind is used interchangeably