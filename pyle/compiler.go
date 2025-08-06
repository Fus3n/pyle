package pyle

import (
	"fmt"
)

type VariableScoped struct {
	Name    string
	IsConst bool
	Depth   int
}

func NewVarScoped(name string, isConst bool, depth int) *VariableScoped {
	return &VariableScoped{
		Name:    name,
		IsConst: isConst,
		Depth:   depth,
	}
}

type LoopScope struct {
	startPatches []int
	endPatches   []int
	depth        int
}

type Compiler struct {
	bytecodeChunk []Instruction
	constants     []Object
	scopeDepth    int
	tokenMap      map[int]Token

	loopLevel      int
	loopScopes     []*LoopScope
	locals         []map[string]*VariableScoped
	funcBaseDepths []int
}

type BytecodeChunk struct {
	Code      []Instruction
	Constants []Object
	TokenMap  map[int]Token
}


func NewCompiler() *Compiler {
	c := &Compiler{
		bytecodeChunk:    []Instruction{},
		constants:        []Object{},
		scopeDepth:       0,
		tokenMap:         map[int]Token{},
		loopLevel:        0,
		loopScopes:       []*LoopScope{},
		locals:           []map[string]*VariableScoped{},
		funcBaseDepths:   []int{0},
	}
	return c
}

func (c *Compiler) Compile(node ASTNode) (*BytecodeChunk, error) {
	// Reset state at the beginning of a compilation
	c.bytecodeChunk = []Instruction{}
	c.constants = []Object{}
	c.scopeDepth = 0
	c.tokenMap = make(map[int]Token)
	c.loopLevel = 0
	c.loopScopes = []*LoopScope{}
	c.funcBaseDepths = []int{0}

	// c.emitSingleInstruct(OpEnterScope)
	// c.scopeDepth = 1

	// Start the recursive compilation process
	if err := c.compileNode(node); err != nil {
		return nil, err
	}

	// c.emitSingleInstruct(OpExitScope)
	// c.scopeDepth = 0

	// All scripts implicitly return nil if they don't have an explicit return.
	nilConst := c.addConstant(NullObj{})
	c.emitInstruct(OpConst, &nilConst, nil)
	c.emitSingleInstruct(OpReturn)

	return &BytecodeChunk{
		Code:      c.bytecodeChunk,
		Constants: c.constants,
		TokenMap:  c.tokenMap,
	}, nil
}

func (c *Compiler) emitInstruct(opcode OpCode, operand any, token *Token) int {
	instructIdx := len(c.bytecodeChunk)
	c.bytecodeChunk = append(c.bytecodeChunk, Instruction{opcode, operand, token})
	if token != nil {
		c.tokenMap[instructIdx] = *token
	}
	return instructIdx
}

func (c *Compiler) enterScope() {
	c.scopeDepth++
	c.emitSingleInstruct(OpEnterScope)
	c.locals = append(c.locals, map[string]*VariableScoped{})
}

func (c *Compiler) exitScope() {
	c.scopeDepth--
	c.emitSingleInstruct(OpExitScope)
	c.locals = c.locals[:len(c.locals)-1]
}


func (c *Compiler) addLocal(varName string, isConst bool) *VariableScoped {
	absDepth := len(c.locals) - 1
	baseDepth := c.funcBaseDepths[len(c.funcBaseDepths)-1]
	relDepth := absDepth - baseDepth

	locVar := NewVarScoped(varName, isConst, relDepth)
	c.locals[absDepth][varName] = locVar // Store in the map using absolute depth
	return locVar
}

// helper function when both operand and token is nil
func (c *Compiler) emitSingleInstruct(opcode OpCode) int {
	return c.emitInstruct(opcode, nil, nil)
}

func (c *Compiler) addConstant(value Object) int {
	// Function objects are not comparable, so we don't check for duplicates.
	if _, ok := value.(*FunctionObj); ok {
		c.constants = append(c.constants, value)
		return len(c.constants) - 1
	}

	// For other (comparable) types, we can check for duplicates.
	for i, constant := range c.constants {
		// Skip non-comparable types like functions.
		if _, isFunc := constant.(*FunctionObj); isFunc {
			continue
		}

		if constant == value {
			return i
		}
	}

	c.constants = append(c.constants, value)
	return len(c.constants) - 1
}

func (c *Compiler) compileNode(node ASTNode) error {
	switch n := node.(type) {
	case *Block:
		return c.visitBlock(n)
	case *NumberExpr:
		return c.visitNumber(n)
	case *StringExpr:
		return c.visitString(n)
	case *BooleanExpr:
		return c.visitBoolean(n)
	case *NullExpr:
		c.emitInstruct(OpNull, nil, node.GetToken())
		return nil
	case *BinaryOp:
		return c.visitBinaryOp(n)
	case *VariableExpr:
		return c.visitVariableExpr(n)
	case *VarDeclareStmt:
		return c.visitVarDeclareStmt(n)
	case *AssignStmt:
		return c.visitAssignStmt(n)
	case *CallExpr:
		return c.visitCallExpr(n)
	case *LogicalOp:
		return c.visitLogicalOp(n)
	case *ComparionOp:
		return c.visitComparionOp(n)
	case *UnaryOp:
		return c.visitUnaryOp(n)
	case *RangeSpecifier:
		return c.visitRangeSpecifier(n)
	case *ForInStmt:
    	return c.visitForInStmt(n)
	case *ArrayExpr:
		return c.visitArrayExpr(n)
	case *MapExpr:
		return c.visitMapExpr(n)
	case *IndexExpr:
		return c.visitIndexExpr(n)
	case *WhileStmt:
		return c.visitWhileStmt(n)
	case *CompoundAssignStmt:
		return c.visitCompoundAssignStmt(n)
	case *FunctionDefStmt:
		return c.visitFunctionDefStmt(n)
	case *FunctionExpr:
		return c.visitFunctionExpr(n)
	case *ReturnStmt:
		return c.visitReturnStmt(n)
	case *IndexAssignStmt:
		return c.visitIndexAssignStmt(n)
	case *IfStmt:
		return c.visitIfStmt(n)
	case *DotExpr:
		return c.visitDotExpr(n)
	case *SetAttrStmt:
		return c.visitSetAttrStmt(n)
	case *BreakStmt:
		return c.visitBreakStmt(n)
	case *ContinueStmt:
		return c.visitContinueStmt(n)
	default:
		// This is the equivalent of Python's `generic_visit` for unhandled cases.
		return fmt.Errorf("compiler error: unhandled AST node type %T", n)

	}
}

func (c *Compiler) visitBlock(node *Block) error {
	createNewScope := node.GetToken() != nil
	if createNewScope {
		c.enterScope()
	}

	for _, stmt := range node.Statements {
		err := c.compileNode(stmt)
		if err != nil {
			return err
		}

		if _, ok := stmt.(Expr); ok {
			c.emitSingleInstruct(OpPop)
		}
	}

	if createNewScope {
		c.exitScope()
	}
	return nil
}

func (c *Compiler) visitNumber(node *NumberExpr) error {
	numVal := NumberObj{Value: node.Value, IsInt: node.IsInt}
	constIdx := c.addConstant(numVal)
	c.emitInstruct(OpConst, &constIdx, node.GetToken())
	return nil
}

func (c *Compiler) visitString(node *StringExpr) error {
	strVal := StringObj{Value: node.Value}
	constIdx := c.addConstant(strVal)
	c.emitInstruct(OpConst, &constIdx, node.GetToken())
	return nil
}

func (c *Compiler) visitBinaryOp(node *BinaryOp) error {
	if err := c.compileNode(node.Left); err != nil {
		return err
	}
	if err := c.compileNode(node.Right); err != nil {
		return err
	}

	var op OpCode
	switch node.Op.Kind {
	case TokenPlus:
		op = OpAdd
	case TokenMinus:
		op = OpSubtract
	case TokenMul:
		op = OpMultiply
	case TokenDiv:
		op = OpDivide
	case TokenMod:
		op = OpModulo

	default:
		return fmt.Errorf("compiler error: unsupported binary operator '%s'", node.Op.Value)
	}

	c.emitInstruct(op, nil, node.GetToken())
	return nil
}

// calculate variable depth
func (c *Compiler) findVariable(name string) (int, bool) {
	for i := len(c.locals) - 1; i >= 0; i-- {
		if _, ok := c.locals[i][name]; ok {
			return i, true
		}
	}
	return -1, false
}


func (c *Compiler) visitVariableExpr(node *VariableExpr) error {
	varName := node.Name.Value
	if c.scopeDepth > 0 {
		if _, ok := Builtins[varName]; !ok {
			if depth, ok := c.findVariable(varName); ok {
				// get var
				varV := c.locals[depth][varName]
				c.emitInstruct(OpGetLocal, varV, node.Name)
				return nil
			}
		} 
	} 

	nameIdx := c.addConstant(StringObj{Value: varName})
	c.emitInstruct(OpGetGlobal, &nameIdx, node.Name)
	return nil
}

func (c *Compiler) visitCallExpr(node *CallExpr) error {
	if err := c.compileNode(node.Callee); err != nil {
		return err
	}

	for _, arg := range node.Arguments {
		if err := c.compileNode(arg); err != nil {
			return err
		}
	}

	numArgs := len(node.Arguments)
	c.emitInstruct(OpCall, &numArgs, node.GetToken())
	return nil
}

func (c *Compiler) visitVarDeclareStmt(node *VarDeclareStmt) error {
	if node.Initializer != nil {
		if err := c.compileNode(node.Initializer); err != nil {
			return err
		}
	} else {
		nullConst := c.addConstant(NullObj{});
		c.emitInstruct(OpConst, nullConst, node.GetToken())
	}

	varNameStr := node.Names[0].Value
	nameIdx := c.addConstant(StringObj{Value: varNameStr})


	var opCode OpCode 

	if c.scopeDepth > 0 {
		if node.IsConst {
			opCode = OpDefConstLocal
		} else {
			opCode = OpDefLocal
		}
		c.addLocal(varNameStr, node.IsConst)
		c.emitInstruct(opCode, &nameIdx, node.GetToken())
	} else {
		if node.IsConst {
			opCode = OpDefConstGlobal
		} else {
			opCode = OpDefGlobal
		}
		c.emitInstruct(opCode, &nameIdx, node.GetToken())
	}

	return nil
}

func (c *Compiler) visitAssignStmt(node *AssignStmt) error {
	if err := c.compileNode(node.Value); err != nil {
		return err
	}
	varNameStr := node.Name.Value
	nameIdx := c.addConstant(StringObj{Value: varNameStr})

	depth, ok := c.findVariable(varNameStr)
	if ok {
		scopedVar := c.locals[depth][varNameStr]
		if scopedVar.IsConst {
			return fmt.Errorf("Cannot assign to constant variable '%s' at %s", varNameStr, node.GetToken().GetFileLoc())
		}
		c.emitInstruct(OpSetLocal, scopedVar, node.GetToken())
	} else {
		c.emitInstruct(OpSetGlobal, &nameIdx, node.GetToken())
	}

	return nil
}

func (c *Compiler) visitLogicalOp(node *LogicalOp) error {
	opTokvalue := node.Op.Value

	if err := c.compileNode(node.Left); err != nil {
		return err
	}

	if err := c.compileNode(node.Right); err != nil {
		return err
	}

	switch opTokvalue {
	case "or":
		c.emitInstruct(OpOr, nil, node.GetToken())
	case "and":
		c.emitInstruct(OpAnd, nil, node.GetToken())
	default:
		return fmt.Errorf("compiler error: unsupported logical operator '%s'", opTokvalue)
	}

	return nil
}

// func (c *Compiler) visitComparionOp(node *ComparionOp) error {
// }

func (c *Compiler) visitUnaryOp(node *UnaryOp) error {
	if err := c.compileNode(node.Operand); err != nil {
		return err
	}
	opValue := node.Op.Value

	switch node.Op.Kind {
	case TokenMinus:
		c.emitInstruct(OpNegate, nil, node.GetToken())
	case TokenKeyword:
		if opValue == "not" {
			c.emitInstruct(OpNot, nil, node.GetToken())
		} else {
			return fmt.Errorf("compiler error: unsupported unary operator '%s'", opValue)
		}
	default:
		return fmt.Errorf("compiler error: Unsupported unary operator '%s' at %s", opValue, node.GetToken().GetFileLoc())
	}

	return nil
}

func (c *Compiler) visitRangeSpecifier(node *RangeSpecifier) error {
	if err := c.compileNode(node.Start); err != nil {
		return err
	}
	if err := c.compileNode(node.End); err != nil {
		return err
	}
	
	if node.Step != nil {
		if err := c.compileNode(*node.Step); err != nil {
			return err
		}
	} else {
		numIdx := c.addConstant(NumberObj{Value: 1, IsInt: true})
		c.emitInstruct(OpConst, &numIdx, node.GetToken())
	}

	c.emitInstruct(OpBuildRange, nil, node.GetToken())
	return nil
}


func (c *Compiler) visitBoolean(node *BooleanExpr) error {
	if node.Value {
		c.emitInstruct(OpTrue, nil, node.GetToken())
	} else {
		c.emitInstruct(OpFalse, nil, node.GetToken())
	}
	return nil
}

func (c *Compiler) visitForInStmt(node *ForInStmt) error {
	c.loopLevel++
	c.enterScope() // Enter scope for loop variable FIRST
	c.loopScopes = append(c.loopScopes, &LoopScope{depth: c.scopeDepth})

	// Compile iterable and create iterator
	if err := c.compileNode(node.Iterable); err != nil {
		return err
	}
	c.emitSingleInstruct(OpIterNew)

	// declare loop variable to null; it will be set on the first iteration.
	nullConst := c.addConstant(NullObj{})
	c.emitInstruct(OpConst, &nullConst, nil)
	loopVarName := node.LoopVariable.Value
	nameIdx := c.addConstant(StringObj{Value: loopVarName})
	
	locVar := c.addLocal(loopVarName, false)
	c.emitInstruct(OpDefLocal, &nameIdx, node.LoopVariable) 

	// Mark the top of the loop
	loopStartIP := len(c.bytecodeChunk)

	// Emit the forward jump placeholder
	placeholder := -1
	jumpForwardInstrIdx := c.emitInstruct(OpIterNextOrJump, &placeholder, node.GetToken())
	addressAfterForwardJump := len(c.bytecodeChunk)

	// If the jump didnt happen, the new value is on top of the stack.
	//    Set the loop variable to this new value.
	c.emitInstruct(OpSetLocal, locVar, node.LoopVariable)

	// Compile the loop body
	if err := c.visitBlock(&node.Body); err != nil {
		return err
	}

	// Patch continue jumps to loopStartIP
	for _, patchIdx := range c.loopScopes[len(c.loopScopes)-1].startPatches {
		offsetToLoopStart := loopStartIP - (patchIdx + 1)
		c.bytecodeChunk[patchIdx].Operand = &offsetToLoopStart
	}

	// Emit the backward jump to the top of the loop
	addressOfNextInstruction := len(c.bytecodeChunk) + 1
	offsetBack := loopStartIP - addressOfNextInstruction
	c.emitInstruct(OpJump, &offsetBack, nil)

	// Patch the forward jump (when iterator is exhausted)
	breakHandlerIP := len(c.bytecodeChunk)
	offsetForward := breakHandlerIP - addressAfterForwardJump
	c.bytecodeChunk[jumpForwardInstrIdx].Operand = &offsetForward

	// Patch break jumps to the same spot
	for _, patchIdx := range c.loopScopes[len(c.loopScopes)-1].endPatches {
		offsetToBreakHandler := breakHandlerIP - (patchIdx + 1)
		c.bytecodeChunk[patchIdx].Operand = &offsetToBreakHandler
	}

	// Pop the exhausted/broken-from iterator and exit the variable's scope
	c.emitSingleInstruct(OpPop) // Pop iterator
	c.exitScope()
	c.loopLevel--
	c.loopScopes = c.loopScopes[:len(c.loopScopes)-1]
	return nil
}

func (c *Compiler) visitArrayExpr(node *ArrayExpr) error {
	for _, elem := range node.Elements {
		if err := c.compileNode(elem); err != nil {
			return err
		}
	}

	numElems := len(node.Elements)
	c.emitInstruct(OpBuildList, &numElems, node.GetToken())
	return nil
}

func (c *Compiler) visitIndexExpr(node *IndexExpr) error {
	if err := c.compileNode(node.Collection); err != nil {
		return err
	}

	if err := c.compileNode(node.Index); err != nil {
		return err
	}

	c.emitInstruct(OpIndexGet, nil, node.GetToken())
	return nil
}

func (c *Compiler) visitWhileStmt(node *WhileStmt) error {
	c.loopLevel++
	c.loopScopes = append(c.loopScopes, &LoopScope{depth: c.scopeDepth})

	loopStartIP := len(c.bytecodeChunk)

	// Compile the condition
	if err := c.compileNode(node.Cond); err != nil {
		return err
	}

	// Emit OpJumpIfFalse with a placeholder
	placeholder := -1
	exitLoopJumpIdx := c.emitInstruct(OpJumpIfFalse, &placeholder, node.GetToken())
	addressAfterJumpIfFalse := len(c.bytecodeChunk)

	// Compile the loop body
	if err := c.compileNode(node.Body); err != nil {
		return err
	}

	// Patch continue jumps
	for _, patchIdx := range c.loopScopes[len(c.loopScopes)-1].startPatches {
		offsetToLoopStart := loopStartIP - (patchIdx + 1)
		c.bytecodeChunk[patchIdx].Operand = &offsetToLoopStart
	}

	// Emit backward jump to the start of the loop
	addressOfNextInstruction := len(c.bytecodeChunk) + 1
	offsetBack := loopStartIP - addressOfNextInstruction
	c.emitInstruct(OpJump, &offsetBack, nil)

	// Patch the forward jump (exit on false condition)
	afterLoopIP := len(c.bytecodeChunk)
	offsetForward := afterLoopIP - addressAfterJumpIfFalse
	c.bytecodeChunk[exitLoopJumpIdx].Operand = &offsetForward

	// Patch break jumps
	for _, patchIdx := range c.loopScopes[len(c.loopScopes)-1].endPatches {
		offsetToAfterLoop := afterLoopIP - (patchIdx + 1)
		c.bytecodeChunk[patchIdx].Operand = &offsetToAfterLoop
	}

	c.loopScopes = c.loopScopes[:len(c.loopScopes)-1]
	c.loopLevel--
	return nil
}

func (c *Compiler) visitCompoundAssignStmt(node *CompoundAssignStmt) error {
	if err := c.compileNode(node.Value); err != nil {
		return err
	}

	varNameStr := node.Name.Value
	nameIdx := c.addConstant(StringObj{Value: varNameStr})

	var opCode OpCode
	switch node.Op.Kind {
	case TokenPlusEquals:
		opCode = OpInplaceAdd
	case TokenMinusEquals:
		opCode = OpInplaceSubtract
	case TokenMulEquals:
		opCode = OpInplaceMultiply
	case TokenDivEquals:
		opCode = OpInplaceDivide
	case TokenModEquals:
		opCode = OpInplaceModulo
	default:
		return fmt.Errorf("compiler error: unsupported compound assignment operator '%s'", node.Op.Value)
	}

	c.emitInstruct(opCode, &nameIdx, node.GetToken())
	return nil
}

func (c *Compiler) visitComparionOp(node *ComparionOp) error {
	if err := c.compileNode(node.Left); err != nil {
		return err
	}
	if err := c.compileNode(node.Right); err != nil {
		return err
	}

	opCode := OpEqual
	switch node.Op.Kind {
		case TokenEQ:
			opCode = OpEqual
		case TokenNEQ:
			opCode = OpNotEqual
		case TokenGT:
			opCode = OpGreater
		case TokenGTE:
			opCode = OpGreaterEqual
		case TokenLT:
			opCode = OpLess
		case TokenLTE:
			opCode = OpLessEqual
		default:
			return fmt.Errorf("compiler error: unsupported comparison operator '%s'", node.Op.Value)
	}

	c.emitInstruct(opCode, nil, node.GetToken())
	return nil
}

func (c *Compiler) visitFunctionDefStmt(node *FunctionDefStmt) error {
	placeholder := -1
	jmpOverBodyIdx := c.emitInstruct(OpJump, &placeholder, node.GetToken())
	functionStartIp := len(c.bytecodeChunk)

	// Store current scope depth to restore after compiling function body
	// as function body compilation is self-contained regarding scope changes it makes.
	enclosingScopeDepth := c.scopeDepth
	c.enterScope()
	c.funcBaseDepths = append(c.funcBaseDepths, c.scopeDepth-1)

	// loop over params in reverse and add to constant and def local
	for i := len(node.Params) - 1; i >= 0; i-- {
		param := node.Params[i]
		paramName := param.Name.Value
		nameIdx := c.addConstant(StringObj{Value: paramName})
		
		c.addLocal(paramName, false)
		c.emitInstruct(OpDefLocal, &nameIdx, param.Name)
	}

	// Check for docstring
	var doc *DocstringObj
	bodyStmts := node.Body.Statements
	if len(bodyStmts) > 0 {
		if strExpr, ok := bodyStmts[0].(*StringExpr); ok {
			doc = &DocstringObj{Description: strExpr.Value}
			// Compile the docstring expression (it will be popped) and then remove it from the list
			// so we don't compile it again.
			if err := c.compileNode(strExpr); err != nil {
				return err
			}
			c.emitSingleInstruct(OpPop)
			bodyStmts = bodyStmts[1:]
		}
	}

	// compile body
	for _, stmt := range bodyStmts {
		if err := c.compileNode(stmt); err != nil {
			return err
		}
	}

	c.emitInstruct(OpExitScope, nil, node.GetToken())
	c.locals = c.locals[:len(c.locals)-1] // This is just the second half of exitScope

	implNull := c.addConstant(NullObj{})
	c.emitInstruct(OpConst, &implNull, nil)
	c.emitInstruct(OpReturn, nil, node.GetToken())

	// Restore compiler's state to what it was before this function def.
	c.funcBaseDepths = c.funcBaseDepths[:len(c.funcBaseDepths)-1]
	c.scopeDepth = enclosingScopeDepth
	c.locals = c.locals[:c.scopeDepth]

	// Patch the initial jump to skip over the function's body.
	addressAfterBody := len(c.bytecodeChunk)
	offset := addressAfterBody - functionStartIp
	c.bytecodeChunk[jmpOverBodyIdx].Operand = &offset

	// Create the Pyle Function object
	pyleFnName := node.Name.Value
	funcObj := FunctionObj{
		Name:    pyleFnName,
		Arity:   len(node.Params),
		Doc:     doc,
		StartIP: &functionStartIp,
	}

	funcConstIdx := c.addConstant(&funcObj)
	c.emitInstruct(OpConst, &funcConstIdx, node.GetToken())

	funcNameIdx := c.addConstant(StringObj{Value: pyleFnName})

	if c.scopeDepth > 0 {
		c.emitInstruct(OpDefLocal, &funcNameIdx, node.Name)
	} else {
		c.emitInstruct(OpDefGlobal, &funcNameIdx, node.Name)
	}

	return nil
}

func (c *Compiler) visitFunctionExpr(node *FunctionExpr) error {
	placeholder := -1
	jmpOverBodyIdx := c.emitInstruct(OpJump, &placeholder, node.GetToken())
	functionStartIp := len(c.bytecodeChunk)

	enclosingScopeDepth := c.scopeDepth
	c.enterScope()
	c.funcBaseDepths = append(c.funcBaseDepths, c.scopeDepth-1)

	// loop over params in reverse and add to constant and def local
	for i := len(node.Params) - 1; i >= 0; i-- {
		param := node.Params[i]
		paramName := param.Name.Value
		nameIdx := c.addConstant(StringObj{Value: paramName})

		c.addLocal(paramName, false)
		c.emitInstruct(OpDefLocal, &nameIdx, param.Name)
	}

	// Check for docstring
	var doc *DocstringObj
	bodyStmts := node.Body.Statements
	if len(bodyStmts) > 0 {
		if strExpr, ok := bodyStmts[0].(*StringExpr); ok {
			doc = &DocstringObj{Description: strExpr.Value}
			if err := c.compileNode(strExpr); err != nil {
				return err
			}
			c.emitSingleInstruct(OpPop) // pop doc
			bodyStmts = bodyStmts[1:]
		}
	}

	// compile body
	for _, stmt := range bodyStmts {
		if err := c.compileNode(stmt); err != nil {
			return err
		}
	}

	// Implicit return if no explicit return was encountered in the body.

	c.emitInstruct(OpExitScope, nil, node.GetToken())
	c.locals = c.locals[:len(c.locals)-1] // This is just the second half of exitScope
	
	implNull := c.addConstant(NullObj{})
	c.emitInstruct(OpConst, &implNull, nil)
	c.emitInstruct(OpReturn, nil, node.GetToken())

	// Restore compiler's state to what it was before this function def.
	c.funcBaseDepths = c.funcBaseDepths[:len(c.funcBaseDepths)-1]
	c.scopeDepth = enclosingScopeDepth
	c.locals = c.locals[:c.scopeDepth]

	// Patch the initial jump to skip over the function's body.
	addressAfterBody := len(c.bytecodeChunk)
	offset := addressAfterBody - functionStartIp
	c.bytecodeChunk[jmpOverBodyIdx].Operand = &offset

	funcObj := FunctionObj{
		Name:    "<lambda>",
		Arity:   len(node.Params),
		Doc:     doc,
		StartIP: &functionStartIp,
	}

	funcConstIdx := c.addConstant(&funcObj)
	c.emitInstruct(OpConst, &funcConstIdx, node.GetToken())
	return nil
}

func (c *Compiler) visitReturnStmt(node *ReturnStmt) error {
	if node.Value != nil {
		if err := c.compileNode(*node.Value); err != nil {
			return err
		}
	} else {
		nullConst := c.addConstant(NullObj{})
		c.emitInstruct(OpConst, &nullConst, node.GetToken())
	}
	
	c.emitSingleInstruct(OpReturn)
	return nil
}

func (c *Compiler) visitIndexAssignStmt(node *IndexAssignStmt) error {
	if err := c.compileNode(node.Collection); err != nil {
		return err
	}
	if err := c.compileNode(node.Index); err != nil {
		return err
	}
	if err := c.compileNode(node.Value); err != nil {
		return err
	}

	c.emitInstruct(OpIndexSet, nil, node.GetToken())
	return nil
}

func (c *Compiler) visitBreakStmt(node *BreakStmt) error {
	if c.loopLevel == 0 {
		return fmt.Errorf("CompileError: 'break' outside loop at: %s", node.GetToken().GetFileLoc())
	}

	numScopesToPop := c.scopeDepth - c.loopScopes[c.loopLevel-1].depth
	for range numScopesToPop {
		c.emitSingleInstruct(OpExitScope)
	}

	placeholder := -1
	jmpIdx := c.emitInstruct(OpJump, &placeholder, node.GetToken())

	c.loopScopes[c.loopLevel-1].endPatches = append(c.loopScopes[c.loopLevel-1].endPatches, jmpIdx)
	return nil
}

func (c *Compiler) visitContinueStmt(node *ContinueStmt) error {
	if c.loopLevel == 0 {
		return fmt.Errorf("CompileError: 'continue' outside loop at: %s", node.GetToken().GetFileLoc())
	}

	// Pop any scopes created inside the loop body before jumping.
	numScopesToPop := c.scopeDepth - c.loopScopes[c.loopLevel-1].depth
	for i := 0; i < numScopesToPop; i++ {
		c.emitSingleInstruct(OpExitScope)
	}

	placeholder := -1
	jmpIdx := c.emitInstruct(OpJump, &placeholder, node.GetToken())

	c.loopScopes[c.loopLevel-1].startPatches = append(c.loopScopes[c.loopLevel-1].startPatches, jmpIdx)
	return nil
}

func (c *Compiler) visitIfStmt(node *IfStmt) error {
	if err := c.compileNode(node.Condition); err != nil {
		return err
	}

	placeholder := -1
	jmpIfFalseIdx := c.emitInstruct(OpJumpIfFalse, &placeholder, node.GetToken())
	
	if err := c.compileNode(node.ThenBranch); err != nil {
		return err
	}

	jmpOverElse := -1
	if node.ElseBranch != nil {
		jmpOverElse = c.emitInstruct(OpJump, &placeholder, node.GetToken())
	}

	// patch jump 
	addressAfterThen := len(c.bytecodeChunk)
	offsetToAfterThen := addressAfterThen - (jmpIfFalseIdx + 1)
	c.bytecodeChunk[jmpIfFalseIdx].Operand = &offsetToAfterThen

	if node.ElseBranch != nil {
		if err := c.compileNode(node.ElseBranch); err != nil {
			return err
		}
		if jmpOverElse == -1 {
			return fmt.Errorf("Internal error: jump_over_else_idx not set for else branch.")
		}
		// patch jmpOverElse 
		addressAfterElse := len(c.bytecodeChunk)
		offsetToAfterElse := addressAfterElse - (jmpOverElse + 1)
		c.bytecodeChunk[jmpOverElse].Operand = &offsetToAfterElse
	}

	return nil
}

func (c *Compiler) visitMapExpr(node *MapExpr) error {
	for _, prop := range node.Properties {
		// KEY compilation
		if prop.IsComputed {
			if err := c.compileNode(prop.Key); err != nil {
				return err
			}
		} else {
			var keyName string
			if ident, ok := prop.Key.(*VariableExpr); ok {
				keyName = ident.Name.Value
			} else if str, ok := prop.Key.(*StringExpr); ok {
				keyName = str.Value
			} else {
				return fmt.Errorf("compiler error: invalid key type for non-computed map property")
			}
			constIdx := c.addConstant(StringObj{Value: keyName})
			c.emitInstruct(OpConst, &constIdx, prop.Key.GetToken())
		}

		// VALUE compilation
		if err := c.compileNode(prop.Value); err != nil {
			return err
		}
	}

	numProps := len(node.Properties)
	c.emitInstruct(OpBuildMap, &numProps, node.GetToken())
	return nil
}

func (c *Compiler) visitDotExpr(node *DotExpr) error {
	if err := c.compileNode(node.Obj); err != nil {
		return err
	}

	keyName := node.Attr.Value
	constIdx := c.addConstant(StringObj{Value: keyName})
	c.emitInstruct(OpGetAttr, &constIdx, node.GetToken())
	return nil
}

func (c *Compiler) visitSetAttrStmt(node *SetAttrStmt) error {
	if err := c.compileNode(node.Obj); err != nil {
		return err
	}

	if err := c.compileNode(node.Value); err != nil {
		return err
	}

	attrName := node.Attr.Value
	nameIdx := c.addConstant(StringObj{Value: attrName})
	c.emitInstruct(OpSetAttr, &nameIdx, node.GetToken())
	return nil
}
