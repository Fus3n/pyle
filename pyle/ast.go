package pyle

import (
	"encoding/json"
	"fmt"
)

type ASTNode interface {
	GetToken() *Token
	String() string
	TypeString() string
}

type Stmt interface {
	ASTNode
	stmtNode() // dummy method
}

type Expr interface {
	ASTNode
	exprNode() // dummy method
}

// Visitor pattern for traversing the AST.
type Visitor interface {
	Visit(node ASTNode)
}

// WalkFunc is a function that can be used as a visitor.
type WalkFunc func(node ASTNode)

func (f WalkFunc) Visit(node ASTNode) {
	f(node)
}

// Walk traverses an AST node and its children for LSP.
func Walk(node ASTNode, visitor Visitor) {
	if node == nil {
		return
	}

	visitor.Visit(node)

	switch n := node.(type) {
	case *Block:
		for _, stmt := range n.Statements {
			Walk(stmt, visitor)
		}
	case *VarDeclareStmt:
		for _, init := range n.Initializers {
			Walk(init, visitor)
		}
	case *AssignStmt:
		Walk(n.Value, visitor)
	case *CompoundAssignStmt:
		Walk(n.Value, visitor)
	case *IfStmt:
		Walk(n.Condition, visitor)
		Walk(n.ThenBranch, visitor)
		Walk(n.ElseBranch, visitor)
	case *ForInStmt:
		Walk(n.Iterable, visitor)
		Walk(&n.Body, visitor)
	case *WhileStmt:
		Walk(n.Cond, visitor)
		Walk(n.Body, visitor)
	case *BinaryOp:
		Walk(n.Left, visitor)
		Walk(n.Right, visitor)
	case *LogicalOp:
		Walk(n.Left, visitor)
		Walk(n.Right, visitor)
	case *ComparisonOp:
		Walk(n.Left, visitor)
		Walk(n.Right, visitor)
	case *UnaryOp:
		Walk(n.Operand, visitor)
	case *PostfixExpr:
		Walk(n.Operand, visitor)
	case *CallExpr:
		Walk(n.Callee, visitor)
		for _, arg := range n.Arguments {
			Walk(arg, visitor)
		}
	case *IndexExpr:
		Walk(n.Collection, visitor)
		Walk(n.Index, visitor)
	case *DotExpr:
		Walk(n.Obj, visitor)
	case *ArrayExpr:
		for _, elem := range n.Elements {
			Walk(elem, visitor)
		}
	case *RangeSpecifier:
		Walk(n.Start, visitor)
		Walk(n.End, visitor)
		if n.Step != nil {
			Walk(*n.Step, visitor)
		}
	}
}

type Block struct {
	Token      *Token
	Statements []ASTNode
	EndToken   *Token
}

func (s *Block) GetToken() *Token { return s.Token }
func (s *Block) String() string {
	str := "Block [\n"
	for _, stmt := range s.Statements {
		str += "  " + stmt.String() + "\n"
	}
	return str + "]"
}
func (a *Block) TypeString() string { return "" }
func (s *Block) stmtNode()          {}
func (s *Block) exprNode()          {}
func (s *Block) MarshalJSON() ([]byte, error) {
	type Alias Block

	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "Block",
		Alias: (*Alias)(s),
	})
}

// statements

type VarDeclareStmt struct {
	Token        *Token
	Names        []*Token
	Type         Expr
	Initializers []Expr
	IsConst      bool
}

func (a *VarDeclareStmt) TypeString() string { return "" }
func (s *VarDeclareStmt) GetToken() *Token   { return s.Token }
func (s *VarDeclareStmt) String() string {
	names := make([]string, len(s.Names))
	for i, name := range s.Names {
		names[i] = name.Value
	}
	return fmt.Sprintf("VarDeclareStmt (\n  Names: %v\n  Initializers: %v\n  IsConst: %t\n)",
		names, s.Initializers, s.IsConst)
}
func (s *VarDeclareStmt) stmtNode() {}
func (s *VarDeclareStmt) MarshalJSON() ([]byte, error) {
	type Alias VarDeclareStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "VarDeclareStmt",
		Alias: (*Alias)(s),
	})
}

type AssignStmt struct {
	Token *Token
	Name  *Token
	Value Expr
}

func (a *AssignStmt) TypeString() string { return "" }
func (s *AssignStmt) GetToken() *Token   { return s.Token }
func (s *AssignStmt) String() string {
	return fmt.Sprintf("AssignStmt (\n  Name: %s\n  Value: %v\n)",
		s.Name.Value, s.Value)
}
func (s *AssignStmt) stmtNode() {}
func (s *AssignStmt) MarshalJSON() ([]byte, error) {
	type Alias AssignStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "AssignStmt",
		Alias: (*Alias)(s),
	})
}

type CompoundAssignStmt struct {
	Token *Token
	Name  *Token
	Op    *Token
	Value Expr
}

func (a *CompoundAssignStmt) TypeString() string { return "" }
func (s *CompoundAssignStmt) GetToken() *Token   { return s.Token }
func (s *CompoundAssignStmt) String() string {
	return fmt.Sprintf("CompoundAssignStmt (\n  Name: %s\n  Op: %s\n  Value: %v\n)",
		s.Name.Value, s.Op.Value, s.Value)
}
func (s *CompoundAssignStmt) stmtNode() {}
func (s *CompoundAssignStmt) MarshalJSON() ([]byte, error) {
	type Alias CompoundAssignStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "CompoundAssignStmt",
		Alias: (*Alias)(s),
	})
}

type IfStmt struct {
	Token      *Token
	Condition  Expr
	ThenBranch *Block
	ElseBranch *Block
}

func (a *IfStmt) TypeString() string { return "" }
func (s *IfStmt) GetToken() *Token   { return s.Token }
func (s *IfStmt) String() string {
	return fmt.Sprintf("IfStmt (\n  Condition: %v\n  Then: %v\n  Else: %v\n)",
		s.Condition, s.ThenBranch, s.ElseBranch)
}
func (s *IfStmt) stmtNode() {}
func (s *IfStmt) MarshalJSON() ([]byte, error) {
	type Alias IfStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "IfStmt",
		Alias: (*Alias)(s),
	})
}

type ForInStmt struct {
	Token        *Token
	LoopVariable *Token
	Iterable     Expr
	Body         Block
}

func (a *ForInStmt) TypeString() string { return "" }
func (s *ForInStmt) GetToken() *Token   { return s.Token }
func (s *ForInStmt) String() string {
	return fmt.Sprintf("ForInStmt (\n  LoopVariable: %v\n  Iterable: %v\n  Body: %v\n)",
		s.LoopVariable.Value, s.Iterable, s.Body)
}
func (s *ForInStmt) stmtNode() {}
func (s *ForInStmt) MarshalJSON() ([]byte, error) {
	type Alias ForInStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "ForInStmt",
		Alias: (*Alias)(s),
	})
}

// Expressions

type VariableExpr struct {
	Token *Token
	Name  *Token
}

func (a *VariableExpr) TypeString() string { return "" }
func (e *VariableExpr) GetToken() *Token   { return e.Token }
func (e *VariableExpr) String() string     { return fmt.Sprintf("VariableExpr (Name: %s)", e.Name.Value) }
func (e *VariableExpr) exprNode()          {}
func (e *VariableExpr) MarshalJSON() ([]byte, error) {
	type Alias VariableExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "VariableExpr",
		Alias: (*Alias)(e),
	})
}

type NumberExpr struct {
	Token *Token
	Value float64
	IsInt bool
}

func (a *NumberExpr) TypeString() string { return "number" }
func (e *NumberExpr) GetToken() *Token   { return e.Token }
func (e *NumberExpr) String() string {
	return fmt.Sprintf("Number (Value: %v, IsInt: %t)", e.Value, e.IsInt)
}
func (e *NumberExpr) exprNode() {}
func (e *NumberExpr) MarshalJSON() ([]byte, error) {
	type Alias NumberExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "Number",
		Alias: (*Alias)(e),
	})
}

type StringExpr struct {
	Token *Token
	Value string
}

func (a *StringExpr) TypeString() string { return "string" }
func (e *StringExpr) GetToken() *Token   { return e.Token }
func (e *StringExpr) String() string     { return fmt.Sprintf("String (Value: \"%s\")", e.Value) }
func (e *StringExpr) exprNode()          {}
func (e *StringExpr) MarshalJSON() ([]byte, error) {
	type Alias StringExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "String",
		Alias: (*Alias)(e),
	})
}

type ArrayExpr struct {
	Token    *Token
	Elements []Expr
}

func (a *ArrayExpr) TypeString() string { return "array" }
func (e *ArrayExpr) GetToken() *Token   { return e.Token }
func (e *ArrayExpr) String() string {
	return fmt.Sprintf("ArrayExpr (\n  Elements: %v\n)", e.Elements)
}
func (e *ArrayExpr) exprNode() {}
func (e *ArrayExpr) MarshalJSON() ([]byte, error) {
	type Alias ArrayExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "ArrayExpr",
		Alias: (*Alias)(e),
	})
}

type MapProperty struct {
	Key        Expr
	Value      Expr
	IsComputed bool
}

// map literal like js
type MapExpr struct {
	Token      *Token
	Properties []MapProperty
}

func (a *MapExpr) TypeString() string { return "object" }
func (e *MapExpr) GetToken() *Token   { return e.Token }
func (e *MapExpr) String() string {
	return fmt.Sprintf("MapExpr (\n  Properties: %v\n)", e.Properties)
}
func (e *MapExpr) exprNode() {}
func (e *MapExpr) MarshalJSON() ([]byte, error) {
	type Alias MapExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "MapExpr",
		Alias: (*Alias)(e),
	})
}

type RangeSpecifier struct {
	Token *Token
	Start Expr
	End   Expr
	Step  *Expr
}

func (a *RangeSpecifier) TypeString() string { return "range" }
func (e *RangeSpecifier) GetToken() *Token   { return e.Token }
func (e *RangeSpecifier) String() string {
	return fmt.Sprintf("RangeSpecifier (\n  start: %v\n  end: %v\n  step: %v\n)",
		e.Start, e.End, e.Step)
}
func (e *RangeSpecifier) exprNode() {}
func (e *RangeSpecifier) MarshalJSON() ([]byte, error) {
	type Alias RangeSpecifier
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "RangeSpecifier",
		Alias: (*Alias)(e),
	})
}

type BinaryOp struct {
	Token *Token
	Left  Expr
	Op    *Token
	Right Expr
}

func (a *BinaryOp) TypeString() string { return "" }
func (e *BinaryOp) GetToken() *Token   { return e.Token }
func (e *BinaryOp) String() string {
	return fmt.Sprintf("BinaryOp (\n  Left: %v\n  Op: %s\n  Right: %v\n)",
		e.Left, e.Op.Value, e.Right)
}
func (e *BinaryOp) exprNode() {}
func (e *BinaryOp) MarshalJSON() ([]byte, error) {
	type Alias BinaryOp
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "BinaryOp",
		Alias: (*Alias)(e),
	})
}

type LogicalOp struct {
	Token *Token
	Left  Expr
	Op    *Token
	Right Expr
}

func (a *LogicalOp) TypeString() string { return "" }
func (e *LogicalOp) GetToken() *Token   { return e.Token }
func (e *LogicalOp) String() string {
	return fmt.Sprintf("LogicalOp (\n  Left: %v\n  Op: %s\n  Right: %v\n)",
		e.Left, e.Op.Value, e.Right)
}
func (e *LogicalOp) exprNode() {}
func (e *LogicalOp) MarshalJSON() ([]byte, error) {
	type Alias LogicalOp
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "LogicalOp",
		Alias: (*Alias)(e),
	})
}

type ComparisonOp struct {
	Token *Token
	Left  Expr
	Op    *Token
	Right Expr
}

func (a *ComparisonOp) TypeString() string { return "" }
func (e *ComparisonOp) GetToken() *Token   { return e.Token }
func (e *ComparisonOp) String() string {
	return fmt.Sprintf("ComparisonOp (\n  Left: %v\n  Op: %s\n  Right: %v\n)",
		e.Left, e.Op.Value, e.Right)
}
func (e *ComparisonOp) exprNode() {}
func (e *ComparisonOp) MarshalJSON() ([]byte, error) {
	type Alias ComparisonOp
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "ComparisonOp",
		Alias: (*Alias)(e),
	})
}

type KeywordArg struct {
	Name  *Token
	Value Expr
}

type CallExpr struct {
	Token     *Token // The '(' token
	Callee    Expr
	Arguments []Expr
}

func (a *CallExpr) TypeString() string { return "" }
func (e *CallExpr) GetToken() *Token   { return e.Token }
func (e *CallExpr) String() string {
	return fmt.Sprintf("CallExpr (\n  Callee: %v\n)", e.Callee)
}
func (e *CallExpr) exprNode() {}
func (e *CallExpr) MarshalJSON() ([]byte, error) {
	type Alias CallExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "CallExpr",
		Alias: (*Alias)(e),
	})
}

type UnaryOp struct {
	Token   *Token
	Op      *Token
	Operand Expr
}

func (a *UnaryOp) TypeString() string { return "" }
func (e *UnaryOp) GetToken() *Token   { return e.Token }
func (e *UnaryOp) String() string {
	return fmt.Sprintf("UnaryOp (\n  Op: %s\n  Operand: %v\n)",
		e.Op.Value, e.Operand)
}
func (e *UnaryOp) exprNode() {}
func (e *UnaryOp) MarshalJSON() ([]byte, error) {
	type Alias UnaryOp
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "UnaryOp",
		Alias: (*Alias)(e),
	})
}

type PostfixExpr struct {
	Token   *Token
	Op      *Token
	Operand Expr
}

func (a *PostfixExpr) TypeString() string { return "" }
func (e *PostfixExpr) GetToken() *Token   { return e.Token }
func (e *PostfixExpr) String() string {
	return fmt.Sprintf("PostfixExpr (\n  Op: %s\n  Operand: %v\n)",
		e.Op.Value, e.Operand)
}
func (e *PostfixExpr) exprNode() {}
func (e *PostfixExpr) MarshalJSON() ([]byte, error) {
	type Alias PostfixExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "PostfixExpr",
		Alias: (*Alias)(e),
	})
}

type BooleanExpr struct {
	Token *Token
	Value bool
}

func (a *BooleanExpr) TypeString() string { return "bool" }
func (e *BooleanExpr) GetToken() *Token   { return e.Token }
func (e *BooleanExpr) String() string     { return fmt.Sprintf("Boolean (Value: %t)", e.Value) }
func (e *BooleanExpr) exprNode()          {}

type NullExpr struct {
	Token *Token
}

func (a *NullExpr) TypeString() string { return "null" }
func (e *NullExpr) GetToken() *Token   { return e.Token }
func (e *NullExpr) String() string     { return "Null" }
func (e *NullExpr) exprNode()          {}

type IndexExpr struct {
	Token      *Token
	Collection Expr
	Index      Expr
}

func (a *IndexExpr) TypeString() string { return "" }
func (e *IndexExpr) GetToken() *Token   { return e.Token }
func (e *IndexExpr) String() string {
	return fmt.Sprintf("IndexExpr (\n  Collection: %v\n  Index: %v\n)",
		e.Collection, e.Index)
}
func (e *IndexExpr) exprNode() {}
func (e *IndexExpr) MarshalJSON() ([]byte, error) {
	type Alias IndexExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "IndexExpr",
		Alias: (*Alias)(e),
	})
}

type IndexAssignStmt struct {
	Token      *Token
	Collection Expr
	Index      Expr
	Value      Expr
}

func (a *IndexAssignStmt) TypeString() string { return "" }
func (s *IndexAssignStmt) GetToken() *Token   { return s.Token }
func (s *IndexAssignStmt) String() string {
	return fmt.Sprintf("IndexAssignStmt (\n  Collection: %v\n  Index: %v\n  Value: %v\n)",
		s.Collection, s.Index, s.Value)
}
func (s *IndexAssignStmt) stmtNode() {}
func (s *IndexAssignStmt) MarshalJSON() ([]byte, error) {
	type Alias IndexAssignStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "IndexAssignStmt",
		Alias: (*Alias)(s),
	})
}

type DotExpr struct {
	Token *Token
	Obj   Expr
	Attr  Token
}

func (a *DotExpr) TypeString() string { return "" }
func (e *DotExpr) GetToken() *Token   { return e.Token }
func (e *DotExpr) String() string {
	return fmt.Sprintf("DotExpr (\n  Obj: %v\n  Attr: %s\n)",
		e.Obj, e.Attr.Value)
}
func (e *DotExpr) exprNode() {}
func (e *DotExpr) MarshalJSON() ([]byte, error) {
	type Alias DotExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "DotExpr",
		Alias: (*Alias)(e),
	})
}

type SetAttrStmt struct {
	Token *Token
	Obj   Expr
	Attr  *Token
	Value Expr
}

func (a *SetAttrStmt) TypeString() string { return "" }
func (s *SetAttrStmt) GetToken() *Token   { return s.Token }
func (s *SetAttrStmt) String() string {
	return fmt.Sprintf("SetAttrStmt (\n  Obj: %v\n  Attr: %s\n  Value: %v\n)",
		s.Obj, s.Attr.Value, s.Value)
}
func (s *SetAttrStmt) stmtNode() {}
func (s *SetAttrStmt) MarshalJSON() ([]byte, error) {
	type Alias SetAttrStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "SetAttrStmt",
		Alias: (*Alias)(s),
	})
}

type WhileStmt struct {
	Token *Token
	Cond  Expr
	Body  *Block
}

func (a *WhileStmt) TypeString() string { return "" }
func (s *WhileStmt) GetToken() *Token   { return s.Token }
func (s *WhileStmt) String() string {
	return fmt.Sprintf("WhileStmt (\n  Condition: %v\n  Body: %v\n)",
		s.Cond, s.Body)
}
func (s *WhileStmt) stmtNode() {}
func (s *WhileStmt) MarshalJSON() ([]byte, error) {
	type Alias WhileStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "WhileStmt",
		Alias: (*Alias)(s),
	})
}

type Parameter struct {
	Name *Token
	Type Expr
}

func (p *Parameter) String() string {
	if p.Type != nil {
		return fmt.Sprintf("%s: %s", p.Name.Value, p.Type.String())
	}
	return p.Name.Value
}

type FunctionExpr struct {
	Token      *Token
	Params     []*Parameter
	Body       *Block
	ReturnType Expr
}

func (a *FunctionExpr) TypeString() string { return "" }
func (e *FunctionExpr) GetToken() *Token   { return e.Token }
func (e *FunctionExpr) String() string {
	return fmt.Sprintf("FunctionExpr (\n  Parameters: %v\n  Body: %v\n)",
		e.Params, e.Body)
}
func (e *FunctionExpr) exprNode() {}
func (e *FunctionExpr) MarshalJSON() ([]byte, error) {
	type Alias FunctionExpr
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "FunctionExpr",
		Alias: (*Alias)(e),
	})
}

type FunctionDefStmt struct {
	Token      *Token
	Name       *Token
	Params     []*Parameter
	Body       *Block
	ReturnType Expr
}

func (a *FunctionDefStmt) TypeString() string { return "" }
func (s *FunctionDefStmt) GetToken() *Token   { return s.Token }
func (s *FunctionDefStmt) String() string {
	return fmt.Sprintf("FunctionDefStmt (\n  Name: %v\n  Parameters: %v\n  Body: %v\n)",
		s.Name.Value, s.Params, s.Body)
}
func (s *FunctionDefStmt) stmtNode() {}
func (s *FunctionDefStmt) MarshalJSON() ([]byte, error) {
	type Alias FunctionDefStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "Func",
		Alias: (*Alias)(s),
	})
}

type ReturnStmt struct {
	Token *Token
	Value *Expr
}

func (a *ReturnStmt) TypeString() string { return "" }
func (s *ReturnStmt) GetToken() *Token   { return s.Token }
func (s *ReturnStmt) String() string {
	if s.Value != nil {
		return fmt.Sprintf("ReturnStmt (\n  Value: %v\n)", *s.Value)
	}
	return "ReturnStmt (No Value)"
}
func (s *ReturnStmt) stmtNode() {}
func (s *ReturnStmt) MarshalJSON() ([]byte, error) {
	type Alias ReturnStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "ReturnStmt",
		Alias: (*Alias)(s),
	})
}

type BreakStmt struct {
	Token *Token
}

func (a *BreakStmt) TypeString() string { return "" }
func (s *BreakStmt) GetToken() *Token   { return s.Token }
func (s *BreakStmt) String() string {
	return "BreakStmt"
}
func (s *BreakStmt) stmtNode() {}
func (s *BreakStmt) MarshalJSON() ([]byte, error) {
	type Alias BreakStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "BreakStmt",
		Alias: (*Alias)(s),
	})
}

type ContinueStmt struct {
	Token *Token
}

func (a *ContinueStmt) TypeString() string { return "" }
func (s *ContinueStmt) GetToken() *Token   { return s.Token }
func (s *ContinueStmt) String() string {
	return "ContinueStmt"
}
func (s *ContinueStmt) stmtNode() {}
func (s *ContinueStmt) MarshalJSON() ([]byte, error) {
	type Alias ContinueStmt
	return json.Marshal(&struct {
		Type string `json:"type"`
		*Alias
	}{
		Type:  "ContinueStmt",
		Alias: (*Alias)(s),
	})
}
