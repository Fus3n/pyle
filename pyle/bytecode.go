package pyle

import "fmt"

// OpCode represents a bytecode operation
type OpCode int

const (
	OpConst OpCode = iota
	OpDefGlobal
	OpGetGlobal
	OpSetGlobal
	OpDefConstGlobal
	OpDefLocal
	OpGetLocal
	OpSetLocal
	OpDefConstLocal
	OpAdd
	OpSubtract
	OpMultiply
	OpDivide
	OpModulo
	OpNegate
	OpNot
	OpEqual
	OpNotEqual
	OpGreater
	OpGreaterEqual
	OpLess
	OpLessEqual
	OpAnd
	OpOr
	OpTrue
	OpFalse
	OpNull
	OpBuildRange
	OpIterNew
	OpIterNextOrJump
	OpBuildList
	OpBuildMap
	OpEnterScope
	OpExitScope
	OpJumpIfFalse
	OpJump
	OpPop
	OpIndexGet
	OpIndexSet
	OpGetAttr
	OpSetAttr
	OpCall
	OpBuildKwargs
	OpReturn
	OpHalt
	OpInplaceAdd
	OpInplaceSubtract
	OpInplaceMultiply
	OpInplaceDivide
	OpInplaceModulo
	OpExit
	OpUnpack
	OpUnwrap
	OpUnwrapOrReturn
	OpUse
)

func (o OpCode) String() string {
	names := map[OpCode]string{
		OpConst:           "OP_CONST",
		OpDefGlobal:       "OP_DEF_GLOBAL",
		OpGetGlobal:       "OP_GET_GLOBAL",
		OpSetGlobal:       "OP_SET_GLOBAL",
		OpDefConstGlobal:  "OP_DEF_CONST_GLOBAL",
		OpDefLocal:        "OP_DEF_LOCAL",
		OpGetLocal:        "OP_GET_LOCAL",
		OpSetLocal:        "OP_SET_LOCAL",
		OpDefConstLocal:   "OP_DEF_CONST_LOCAL",
		OpAdd:             "OP_ADD",
		OpSubtract:        "OP_SUBTRACT",
		OpMultiply:        "OP_MULTIPLY",
		OpDivide:          "OP_DIVIDE",
		OpModulo:          "OP_MODULO",
		OpNegate:          "OP_NEGATE",
		OpNot:             "OP_NOT",
		OpEqual:           "OP_EQUAL",
		OpNotEqual:        "OP_NOT_EQUAL",
		OpGreater:         "OP_GREATER",
		OpGreaterEqual:    "OP_GREATER_EQUAL",
		OpLess:            "OP_LESS",
		OpLessEqual:       "OP_LESS_EQUAL",
		OpAnd:             "OP_AND",
		OpOr:              "OP_OR",
		OpTrue:            "OP_TRUE",
		OpFalse:           "OP_FALSE",
		OpNull:            "OP_NULL",
		OpBuildRange:      "OP_BUILD_RANGE",
		OpIterNew:         "OP_ITER_NEW",
		OpIterNextOrJump:  "OP_ITER_NEXT_OR_JUMP",
		OpBuildList:       "OP_BUILD_LIST",
		OpBuildMap:        "OP_BUILD_MAP",
		OpEnterScope:      "OP_ENTER_SCOPE",
		OpExitScope:       "OP_EXIT_SCOPE",
		OpJumpIfFalse:     "OP_JUMP_IF_FALSE",
		OpJump:            "OP_JUMP",
		OpPop:             "OP_POP",
		OpIndexGet:        "OP_INDEX_GET",
		OpIndexSet:        "OP_INDEX_SET",
		OpGetAttr:         "OP_GET_ATTR",
		OpSetAttr:         "OP_SET_ATTR",
		OpCall:            "OP_CALL",
		OpBuildKwargs:     "OP_BUILD_KWARGS",
		OpReturn:          "OP_RETURN",
		OpHalt:            "OP_HALT",
		OpInplaceAdd:      "OP_INPLACE_ADD",
		OpInplaceSubtract: "OP_INPLACE_SUBTRACT",
		OpInplaceMultiply: "OP_INPLACE_MULTIPLY",
		OpInplaceDivide:   "OP_INPLACE_DIVIDE",
		OpInplaceModulo:   "OP_INPLACE_MODULO",
		OpExit:            "OP_EXIT",
		OpUnpack:          "OP_UNPACK",
		OpUnwrap:          "OP_UNWRAP",
		OpUnwrapOrReturn:  "OP_UNWRAP_OR_RETURN",
		OpUse:             "OP_USE",
	}
	if name, ok := names[o]; ok {
		return name
	}
	return fmt.Sprintf("OP_UNKNOWN_%d", o)
}

type Instruction struct {
	Op      OpCode
	Operand any
	Token   *Token
}

func (i Instruction) String() string {
	if i.Operand != nil {
		return fmt.Sprintf("%s %v", i.Op, i.Operand)
	}
	return i.Op.String()
}

type PyleFunction struct {
	Name     string
	Arity    int
	StartIP  *int
	NativeFn func([]Object) (Object, error)
}

func (f *PyleFunction) String() string {
	fnType := "pyle"
	if f.NativeFn != nil {
		fnType = "native"
	}
	return fmt.Sprintf("<fn %s/%d (%s)>", f.Name, f.Arity, fnType)
}

type Variable struct {
	Name    string
	Value   Object
	IsConst bool
}
