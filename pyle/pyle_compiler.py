# pyle/pyle_compiler.py
from pyle.pyle_types import TokenType, Token
from .pyle_bytecode import OpCode, Instruction, PyleFunction # Import PyleFunction
from .pyle_ast import *
from .pyle_builtins import BUILTINS


OPERATION_INTS = {
    "PLUS": OpCode.OP_ADD,
    "MINUS": OpCode.OP_SUBTRACT,
    "MUL": OpCode.OP_MULTIPLY,
    "DIV": OpCode.OP_DIVIDE,
    "MOD": OpCode.OP_MODULO,

    "and": OpCode.OP_AND,
    "or": OpCode.OP_OR,

    "EQ": OpCode.OP_EQUAL,
    "NEQ": OpCode.OP_NOT_EQUAL,
    "GT": OpCode.OP_GREATER,
    "GTE": OpCode.OP_GREATER_EQUAL,
    "LT": OpCode.OP_LESS,
    "LTE": OpCode.OP_LESS_EQUAL,
}

class Compiler:

    def __init__(self) -> None:
        self.bytecode_chunk: list[Instruction] = []
        self.constants: list[any] = []
        self.scope_depth: int = 0 
        self.token_map: dict[int, Token] = {} 

        self.loop_level = 0 # To check if break/continue are inside a loop
        self.loop_start_patches: list[list[int]] = [] # For 'continue', stores lists of jump instruction indices to patch
        self.loop_end_patches: list[list[int]] = []   # For 'break', stores lists of jump instruction indices to patch

        # Pre-define built-in functions here
        self._initialize_builtins()

    
    def _initialize_builtins(self):
        for name, fn_obj in BUILTINS.items():
            fn_idx = self.add_constant(fn_obj)
            name_idx = self.add_constant(name)
            # Always define built-ins as globals (so they're available everywhere)
            self.emit_instruction(OpCode.OP_CONST, fn_idx)
            self.emit_instruction(OpCode.OP_DEF_GLOBAL, name_idx)

    def compile(self, node: ASTNode): 
        self.bytecode_chunk = [] 
        self.constants = [] 
        self.scope_depth = 0 # Start at 0, representing "outside any specific user-defined scope"
        self.token_map = {}
        self.loop_level = 0 
        self.loop_start_patches = [] 
        self.loop_end_patches = [] 
        self._initialize_builtins()

        # --- Script's Main Scope Setup ---
        self.emit_instruction(OpCode.OP_ENTER_SCOPE) # VM enters a new environment
        self.scope_depth = 1 # Compiler now tracks that we are inside the script's main scope

        # Compile the actual AST of the script (which is usually a Block node)
        # self.scope_depth is 1. visit_Block for this top-level node should NOT create another scope.
        self._compile_node(node) 

        # --- Script's Main Scope Teardown ---
        self.emit_instruction(OpCode.OP_EXIT_SCOPE) 
        self.scope_depth = 0 # Back to "global" or "no specific user scope"

        self.emit_instruction(OpCode.OP_CONST, self.add_constant(None)) 
        self.emit_instruction(OpCode.OP_RETURN) 
        return self.bytecode_chunk, self.constants

    def _compile_node(self, node: ASTNode): 
        method_Name = f"visit_{node.__class__.__name__}"
        visitor = getattr(self, method_Name, self.generic_visit)
        visitor(node)

    def add_constant(self, value):
        try:
            # For unhashable types like lists (or PyleFunction if not frozen), direct index fails.
            if isinstance(value, list): # Example for lists, extend as needed
                for i, c in enumerate(self.constants):
                    if isinstance(c, list) and c == value: return i
            elif isinstance(value, PyleFunction): # PyleFunction is frozen, so it's hashable
                 pass # Fall through to normal index/append
            return self.constants.index(value)
        except ValueError:
            self.constants.append(value)
            return len(self.constants) - 1
        
    def emit_instruction(self, opcode: OpCode, operand=None, token: Token=None) -> int:
        instruction_index = len(self.bytecode_chunk)
        self.bytecode_chunk.append(Instruction(opcode, operand))
        if token:
            self.token_map[instruction_index] = token 
        return instruction_index
    
    def patch_jump(self, instruction_index: int, jump_target_ip: int | None = None):
        if instruction_index is None: 
            raise ValueError("patch_jump called with None instruction_index")
        
        # Ensure instruction_index is a valid index for an existing instruction
        if not (isinstance(instruction_index, int) and 0 <= instruction_index < len(self.bytecode_chunk)):
            raise IndexError(f"patch_jump: instruction_index {instruction_index} is out of bounds for bytecode length {len(self.bytecode_chunk)}")

        target_ip_to_set = jump_target_ip if jump_target_ip is not None else len(self.bytecode_chunk)
        self.bytecode_chunk[instruction_index].operand = target_ip_to_set

    def visit_Block(self, node: Block):
        # Only create a new scope if this is an explicit block (node.token is not None)
        create_new_scope = node.token is not None
        if create_new_scope:
            self.scope_depth += 1
            self.emit_instruction(OpCode.OP_ENTER_SCOPE, token=node.token)
        for statement_node in node.statements:
            self._compile_node(statement_node)
            # Only pop if this is an expression statement (not a block, not a variable declaration, etc.)
            if isinstance(statement_node, Expr):
                self.emit_instruction(OpCode.OP_POP)
        if create_new_scope:
            self.emit_instruction(OpCode.OP_EXIT_SCOPE, token=node.token)
            self.scope_depth -= 1

    def visit_Number(self, node: Number):
        constant_index = self.add_constant(node.value)
        self.emit_instruction(OpCode.OP_CONST, constant_index, node.token)        

    def visit_String(self, node: String):
        constant_index = self.add_constant(node.value)
        self.emit_instruction(OpCode.OP_CONST, constant_index, node.token)        

    def visit_UnaryOp(self, node: UnaryOp):
        self._compile_node(node.operand) # First compile the operand
        
        op_kind = node.op.kind
        op_value = node.op.value

        if op_kind == TokenType.MINUS:
            self.emit_instruction(OpCode.OP_NEGATE, token=node.op)
        elif op_kind == TokenType.KEYWORD and op_value == "not":
            self.emit_instruction(OpCode.OP_NOT, token=node.op)
        else:
            # This case should ideally be prevented by the parser.
            raise Exception(f"Compiler error: Unsupported unary operator '{op_value}' at {node.op.get_file_loc()}")

    def visit_BinaryOp(self, node: BinaryOp):
        self._compile_node(node.left)
        self._compile_node(node.right)
        opcode = OPERATION_INTS.get(node.op.kind.name)
        if not opcode:
            raise NotImplementedError(f"Binary operator {node.op.kind.name} not implemented.")
        self.emit_instruction(opcode, token=node.token)

    def visit_LogicalOp(self, node: LogicalOp):
        # For 'and' and 'or', we need short-circuiting
        op_token_value = node.op.value # "and" or "or"
        
        if op_token_value == "or":
            self._compile_node(node.left)
            # If left is true, jump to end, leaving true on stack
            jump_if_true_idx = self.emit_instruction(OpCode.OP_JUMP_IF_FALSE, 99999, token=node.op) # Placeholder
            # This is tricky: OP_JUMP_IF_FALSE jumps if false. For OR, if left is true, we skip right.
            # So, if left is true, we DON'T jump. We pop left, push true.
            # Let's use a simpler approach for now, relying on Python's truthiness for `and`/`or` ops in VM.
            # This means no short-circuiting at bytecode level yet.
            # Fallback to non-short-circuiting (evaluates both sides):
            self._compile_node(node.left)
            self._compile_node(node.right)
            opcode = OPERATION_INTS.get(op_token_value)
            if not opcode: raise NotImplementedError(f"Logical op {op_token_value} error.")
            self.emit_instruction(opcode, token=node.op)
            # Proper short-circuiting for 'or':
            # compile left
            # DUP (if needed)
            # JUMP_IF_TRUE end_or  (jumps if left is true, leaving it on stack)
            # POP (pop false left)
            # compile right
            # end_or:
        elif op_token_value == "and":
            # Proper short-circuiting for 'and':
            # compile left
            # DUP (if needed)
            # JUMP_IF_FALSE end_and (jumps if left is false, leaving it on stack)
            # POP (pop true left)
            # compile right
            # end_and:
            # Fallback to non-short-circuiting:
            self._compile_node(node.left)
            self._compile_node(node.right)
            opcode = OPERATION_INTS.get(op_token_value)
            if not opcode: raise NotImplementedError(f"Logical op {op_token_value} error.")
            self.emit_instruction(opcode, token=node.op)
        else:
            raise NotImplementedError(f"Logical keyword {op_token_value} not implemented.")
        
    def visit_ComparisonOp(self, node: ComparisonOp):
        self._compile_node(node.left)
        self._compile_node(node.right)
        opcode = OPERATION_INTS.get(node.op.kind.name)
        if not opcode:
            raise NotImplementedError(f"Comparison operator {node.op.kind.name} not implemented.")
        self.emit_instruction(opcode, token=node.token)

    def visit_VarDeclareStmt(self, node: VarDeclareStmt):
        if node.initializer:
            self._compile_node(node.initializer)
        else:
            # Pyle requires initializers for `let`, or push a default e.g. None
            self.emit_instruction(OpCode.OP_CONST, self.add_constant(None), token=node.name) 

        var_name_str = node.name.value
        name_idx = self.add_constant(var_name_str)

        op_code = OpCode.OP_DEF_GLOBAL

        if self.scope_depth > 0: # If inside any scope (global script scope, or function/block scope)
            op_code = OpCode.OP_DEF_CONST_LOCAL if node.is_const else OpCode.OP_DEF_LOCAL
            self.emit_instruction(op_code, name_idx, token=node.name)
        else:
            op_code = OpCode.OP_DEF_CONST_GLOBAL if node.is_const else OpCode.OP_DEF_GLOBAL
            self.emit_instruction(op_code, name_idx, token=node.name)
            self.emit_instruction(OpCode.OP_DEF_GLOBAL, name_idx, token=node.name)

    def visit_AssignStmt(self, node: AssignStmt):
        self._compile_node(node.value) 
        var_name_str = node.name.value
        name_idx = self.add_constant(var_name_str)
      
        if self.scope_depth > 0:
            self.emit_instruction(OpCode.OP_SET_LOCAL, name_idx, token=node.name)
        else:
            self.emit_instruction(OpCode.OP_SET_GLOBAL, name_idx, token=node.name)


    def visit_VariableExpr(self, node: VariableExpr):
        var_name = node.name.value
        name_idx = self.add_constant(var_name)
        if self.scope_depth > 0 and var_name not in BUILTINS:
            self.emit_instruction(OpCode.OP_GET_LOCAL, name_idx, token=node.name)
        else:
            self.emit_instruction(OpCode.OP_GET_GLOBAL, name_idx, token=node.name)


    def visit_Boolean(self, node: Boolean):
        if node.value: 
            self.emit_instruction(OpCode.OP_TRUE, token=node.token)
        else:
            self.emit_instruction(OpCode.OP_FALSE, token=node.token)

    def visit_IfStmt(self, node: IfStmt):
        self._compile_node(node.condition)
        jump_if_false_idx = self.emit_instruction(OpCode.OP_JUMP_IF_FALSE, 99999, token=node.token) 
        self._compile_node(node.then_branch)

        jump_over_else_idx = -1
        if node.else_branch:
            jump_over_else_idx = self.emit_instruction(OpCode.OP_JUMP, 99999, token=node.token) # Token of 'else' if available

        self.patch_jump(jump_if_false_idx)

        if node.else_branch:
            self._compile_node(node.else_branch)
            if jump_over_else_idx == -1:
                raise RuntimeError("Internal error: jump_over_else_idx not set for else branch.")
            self.patch_jump(jump_over_else_idx)

    def visit_WhileStmt(self, node: WhileStmt):
        self.loop_level += 1
        self.loop_start_patches.append([])
        self.loop_end_patches.append([])

        loop_start_ip = len(self.bytecode_chunk) # Target for continue

        self._compile_node(node.condition)
        exit_loop_jump_idx = self.emit_instruction(OpCode.OP_JUMP_IF_FALSE, 99999, token=node.token)
        
        self._compile_node(node.body)

        # Patch all continue jumps to point to the start of the loop (condition check)
        for patch_idx in self.loop_start_patches[-1]:
            self.bytecode_chunk[patch_idx].operand = loop_start_ip
        
        self.emit_instruction(OpCode.OP_JUMP, loop_start_ip, token=node.token) # Jump back to condition
        
        self.patch_jump(exit_loop_jump_idx) # Target for loop exit and break
        
        # Patch all break jumps to point to after the loop
        loop_end_target_ip = self.bytecode_chunk[exit_loop_jump_idx].operand
        for patch_idx in self.loop_end_patches[-1]:
            self.bytecode_chunk[patch_idx].operand = loop_end_target_ip

        self.loop_start_patches.pop()
        self.loop_end_patches.pop()
        self.loop_level -= 1

    def visit_RangeSpecifier(self, node: RangeSpecifier):
        self._compile_node(node.start)
        self._compile_node(node.end)
        if node.step:
            self._compile_node(node.step)
        else:
            self.emit_instruction(OpCode.OP_CONST, self.add_constant(1), token=node.token)
        self.emit_instruction(OpCode.OP_BUILD_RANGE, token=node.token)

    def visit_ForInStmt(self, node: ForInStmt):
        self.loop_level += 1
        
        current_loop_break_patches = [] 
        self.loop_end_patches.append(current_loop_break_patches)

        current_loop_continue_patches = []
        self.loop_start_patches.append(current_loop_continue_patches)

        self.scope_depth += 1
        self.emit_instruction(OpCode.OP_ENTER_SCOPE, token=node.token) 

        self._compile_node(node.iterable) 
        self.emit_instruction(OpCode.OP_ITER_NEW, token=node.iterable.token if node.iterable.token else node.token) # Pushes iterator

        var_name_str = node.loop_variable.value
        name_idx = self.add_constant(var_name_str)
        self.emit_instruction(OpCode.OP_CONST, self.add_constant(None), token=node.loop_variable) 
        self.emit_instruction(OpCode.OP_DEF_LOCAL, name_idx, token=node.loop_variable) # Defines loop var

        loop_iteration_start_ip = len(self.bytecode_chunk) # Target for continue AND normal loop back
        
        # OP_ITER_NEXT_OR_JUMP:
        # - If next item: pushes item. Iterator remains on stack under item.
        # - If exhausted: pops iterator, then jumps to its operand.
        jump_if_exhausted_idx = self.emit_instruction(OpCode.OP_ITER_NEXT_OR_JUMP, 99999, token=node.token) # Placeholder for jump on exhaustion

        # Item is now on stack (if not exhausted). Assign to loop variable.
        self.emit_instruction(OpCode.OP_SET_LOCAL, name_idx, token=node.loop_variable) # Pops item, iterator remains.
        
        self._compile_node(node.body) # Body might contain break/continue

        self.emit_instruction(OpCode.OP_JUMP, loop_iteration_start_ip, token=node.token) # Loop back
        
        # --- Loop End Handling ---
        # This IP is the target for 'break' statements from the loop body.
        # It's responsible for popping the iterator that 'break' would leave on stack.
        break_handler_ip = len(self.bytecode_chunk)
        self.emit_instruction(OpCode.OP_POP, token=node.token) # Pop the iterator

        # Patch all 'break' jumps collected from visit_BreakStmt for this loop
        for patch_idx in current_loop_break_patches:
            self.bytecode_chunk[patch_idx].operand = break_handler_ip
        
        # This IP is the target for:
        # 1. OP_ITER_NEXT_OR_JUMP when the iterator is exhausted.
        # 2. Fall-through from break_handler_ip (after iterator has been popped).
        after_loop_ip = len(self.bytecode_chunk)
        self.patch_jump(jump_if_exhausted_idx, after_loop_ip) # Patch OP_ITER_NEXT_OR_JUMP's exhaustion jump

        # Patch all 'continue' jumps for this loop
        for patch_idx in current_loop_continue_patches:
            self.bytecode_chunk[patch_idx].operand = loop_iteration_start_ip
            
        self.emit_instruction(OpCode.OP_EXIT_SCOPE, token=node.token)
        self.scope_depth -= 1

        self.loop_start_patches.pop()
        self.loop_end_patches.pop()
        self.loop_level -= 1


    def visit_ArrayLiteral(self, node: ArrayLiteral):
        for element_expr in node.elements:
            self._compile_node(element_expr)
        num_elements = len(node.elements)
        self.emit_instruction(OpCode.OP_BUILD_LIST, num_elements, token=node.token)

    def visit_BreakStmt(self, node: BreakStmt):
        if self.loop_level == 0:
            raise Exception(f"CompileError: 'break' outside loop at {node.token.get_file_loc()}")
        
        jump_idx = self.emit_instruction(OpCode.OP_JUMP, 99999, token=node.token) # Placeholder target
        self.loop_end_patches[-1].append(jump_idx)

    def visit_ContinueStmt(self, node: ContinueStmt):
        if self.loop_level == 0:
            raise Exception(f"CompileError: 'continue' outside loop at {node.token.get_file_loc()}")

        jump_idx = self.emit_instruction(OpCode.OP_JUMP, 99999, token=node.token) # Placeholder target
        self.loop_start_patches[-1].append(jump_idx)
        
    # --- New Visit Methods for Functions ---

    def visit_FunctionDefStmt(self, node: FunctionDefStmt):
        # Mark the current instruction pointer as where we'd jump over the function body.
        jump_over_body_idx = self.emit_instruction(OpCode.OP_JUMP, 99999, token=node.token)

        function_start_ip = len(self.bytecode_chunk)
        
        # Store current scope depth to restore after compiling function body
        # as function body compilation is self-contained regarding scope changes it makes.
        enclosing_scope_depth = self.scope_depth
        self.scope_depth +=1 # Entering function's own lexical scope context immediately

        self.emit_instruction(OpCode.OP_ENTER_SCOPE, token=node.token) 

        # Parameters are defined as locals. Arguments are pushed by caller.
        # OP_DEF_LOCAL pops from stack. If args are arg0, arg1, argN (top),
        # and params are p0, p1, pN. We need to define pN (from argN), then pN-1 (from argN-1)
        # So iterate params in REVERSE order.
        for param_token in reversed(node.params):
            param_name_idx = self.add_constant(param_token.value)
            # OP_DEF_LOCAL will pop the argument value from the stack and define it.
            self.emit_instruction(OpCode.OP_DEF_LOCAL, param_name_idx, token=param_token)

        self._compile_node(node.body)

        # Implicit return if no explicit return was encountered in the body.
        # This must also handle exiting the function's scope.
        self.emit_instruction(OpCode.OP_EXIT_SCOPE, token=node.body.token if node.body.token else node.token)
        
        self.emit_instruction(OpCode.OP_CONST, self.add_constant(None), token=node.token) # Default return value
        self.emit_instruction(OpCode.OP_RETURN, token=node.token) # Actual return mechanism

        # Restore compiler's scope_depth to what it was before this function def.
        self.scope_depth = enclosing_scope_depth

        # Patch the initial jump to skip over the function's body.
        self.patch_jump(jump_over_body_idx)

        # Create the PyleFunction object (runtime representation)
        pyle_fn_name = node.name.value
        func_obj = PyleFunction(name=pyle_fn_name, arity=len(node.params), start_ip=function_start_ip)
        func_const_idx = self.add_constant(func_obj)

        # Define the function name in the current scope (where 'fn' was declared)
        # This pushes the PyleFunction object onto the stack, then defines it.
        self.emit_instruction(OpCode.OP_CONST, func_const_idx, token=node.name)
        
        # Use current scope_depth (of the enclosing scope) to decide def_local/def_global
        # Note: self.scope_depth was restored above.
        if self.scope_depth > 0: # If fn defined inside another scope (e.g. global script scope, or another fn)
             self.emit_instruction(OpCode.OP_DEF_LOCAL, self.add_constant(node.name.value), token=node.name)
        else: # This case should be rare if script is wrapped in a scope.
             self.emit_instruction(OpCode.OP_DEF_GLOBAL, self.add_constant(node.name.value), token=node.name)


    def visit_CallExpr(self, node: CallExpr):
        self._compile_node(node.callee)
        for arg in node.arguments:
            self._compile_node(arg)
        # For keywords, push values
        for kw in node.keywords:
            self._compile_node(kw.value)
        if node.keywords:
            # Push the names list as a constant BEFORE OP_BUILD_KWARGS
            kw_names = [kw.name.value for kw in node.keywords]
            kw_names_idx = self.add_constant(kw_names)
            self.emit_instruction(OpCode.OP_CONST, kw_names_idx, token=node.token)
            self.emit_instruction(OpCode.OP_BUILD_KWARGS, len(node.keywords), token=node.token)
            self.emit_instruction(OpCode.OP_CALL_KW, (len(node.arguments), len(node.keywords)), token=node.token)
        else:
            self.emit_instruction(OpCode.OP_CALL, len(node.arguments), token=node.token)
            
    def visit_ReturnStmt(self, node: ReturnStmt):
        if node.value:
            self._compile_node(node.value) # Push return value
        else:
            self.emit_instruction(OpCode.OP_CONST, self.add_constant(None), token=node.token) # Return None

        # The OP_RETURN in VM will handle popping the scope environment for the current function.
        self.emit_instruction(OpCode.OP_RETURN, token=node.token)

    def visit_FunctionExpr(self, node: FunctionExpr):
        # 1. Emit a jump over the function body
        jump_over_body_idx = self.emit_instruction(OpCode.OP_JUMP, 99999, token=node.token)

        function_start_ip = len(self.bytecode_chunk)
        enclosing_scope_depth = self.scope_depth
        self.scope_depth += 1
        self.emit_instruction(OpCode.OP_ENTER_SCOPE, token=node.token)

        # 2. Parameters are defined as locals (when function is called)
        for param_token in reversed(node.params):
            param_name_idx = self.add_constant(param_token.value)
            self.emit_instruction(OpCode.OP_DEF_LOCAL, param_name_idx, token=param_token)

        self._compile_node(node.body)
        self.emit_instruction(OpCode.OP_EXIT_SCOPE, token=node.body.token if node.body.token else node.token)
        self.emit_instruction(OpCode.OP_CONST, self.add_constant(None), token=node.token)
        self.emit_instruction(OpCode.OP_RETURN, token=node.token)
        self.scope_depth = enclosing_scope_depth

        # 3. Patch the jump to skip over the function body
        self.patch_jump(jump_over_body_idx)

        # 4. Create the function object and push it onto the stack
        func_obj = PyleFunction(name="<lambda>", arity=len(node.params), start_ip=function_start_ip)
        func_const_idx = self.add_constant(func_obj)
        self.emit_instruction(OpCode.OP_CONST, func_const_idx, token=node.token)

    def visit_IndexExpr(self, node: IndexExpr):
        self._compile_node(node.collection)  
        self._compile_node(node.index)
        self.emit_instruction(OpCode.OP_INDEX_GET, token=node.token)

    def visit_AssignIndexStmt(self, node: AssignIndexStmt):
        self._compile_node(node.collection)  # Push collection
        self._compile_node(node.index)       # Push index
        self._compile_node(node.value)       # Push value
        self.emit_instruction(OpCode.OP_INDEX_SET, token=node.token)

    def visit_DotExpr(self, node: DotExpr):
        self._compile_node(node.object)
        attr_name_idx = self.add_constant(node.attr.value)
        self.emit_instruction(OpCode.OP_GET_ATTR, attr_name_idx, token=node.token)

    def generic_visit(self, node: ASTNode):
        raise Exception(f"No visit_{node.__class__.__name__} method in Compiler for {node}")
