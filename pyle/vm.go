package pyle

import (
	"fmt"
	"io"
	"math"
	"os"
)

type CallFrame struct {
	ReturnIP  int
	StackSlot int
	EnvDepth  int
}

const InitialStackCapacity = 256

type VM struct {
	bytecodeChunk []Instruction
	constants     []Object
	ip            int
	stack         []Object
	sp            int // Stack pointer
	globals       map[string]*Variable
	environments  []map[string]*Variable
	frames        []*CallFrame
	Stdout        io.Writer
}

func NewVM() *VM {
	return &VM{
		stack:        make([]Object, InitialStackCapacity),
		sp:           0,
		globals:      make(map[string]*Variable),
		environments: make([]map[string]*Variable, 0),
		frames:       make([]*CallFrame, 0),
		Stdout:       os.Stdout,
	}
}

func (vm *VM) AddGlobal(name string, value Object) Error {
	if _, ok := vm.globals[name]; ok {
		return vm.runtimeError("Global variable '%s' already defined", name)
	}
	vm.globals[name] = &Variable{Name: name, Value: value, IsConst: false}
	return nil
}

func (vm *VM) LoadBuiltins() error {
	for name, fn := range Builtins {
		doc := BuiltinDocs[name]
		err := vm.RegisterGOFunction(name, fn, doc)
		if err != nil {
			return err
		}
	}

	// temporary solution for grouped values/modules
	timeFuncs := map[string]any{
		"time":    nativeTime,
		"timeNs":  nativeTimeNs,
		"timeMs": nativeTimeMs,
	}

	mapObj := NewMap()
	
	for name, fn := range timeFuncs {
		doc := BuiltinDocs[name]
		nativeFunc, err := CreateNativeFunction(name, fn, doc)
		if err != nil {
			return err
		}
		keyVal := StringObj{Value: name}
		err = mapObj.Set(keyVal, nativeFunc)
		if err != nil {
			return err
		}
	}
	
	vm.AddGlobal("time", mapObj)
	return nil
}

func (vm *VM) RegisterGOFunction(name string, fn any, doc *DocstringObj) error {
	nativeFunc, err := CreateNativeFunction(name, fn, doc)
	if err != nil {
		return err
	}

	vm.AddGlobal(name, nativeFunc)
	return nil
}

func (vm *VM) Interpret(bytecodeChunk []Instruction, constants []Object) Result[Object] {
	vm.bytecodeChunk = bytecodeChunk
	vm.constants = constants
	vm.ip = 0
	vm.sp = 0
	vm.frames = vm.frames[:0]
	vm.environments = vm.environments[:0]

	return vm.run(0)
}

func (vm *VM) CallFunction(callable Object, args []Object) (Object, Error) {
	stackBottom := vm.sp
	vm.push(callable)
	for _, arg := range args {
		vm.push(arg)
	}

	if _, err := vm.handleCall(len(args), nil); err != nil {
		return nil, err
	}

	initialFrameCount := len(vm.frames)
	runResult := vm.run(initialFrameCount)
	if runResult.Err != nil {
		return nil, runResult.Err
	}

	if vm.sp > stackBottom {
		returnVal := vm.stack[vm.sp-1]
		vm.sp = stackBottom
		return returnVal, nil
	}

	return NullObj{}, nil
}

func (vm *VM) callPyleFuncFromNative(callable Object, args []Object) (Object, Error) {
	initialStackSize := vm.sp
	initialFrameDepth := len(vm.frames)

	vm.push(callable)
	for _, arg := range args {
		vm.push(arg)
	}

	pyleFuncCalled, err := vm.handleCall(len(args), nil)
	if err != nil {
		return nil, err
	}

	if pyleFuncCalled {
		runResult := vm.run(len(vm.frames))
		if runResult.Err != nil {
			return nil, runResult.Err
		}
	}

	var result Object = NullObj{}
	if vm.sp > initialStackSize {
		result, err = vm.pop()
		if err != nil {
			return nil, err
		}
	}

	if vm.sp > initialStackSize {
		vm.sp = initialStackSize
	}
	for len(vm.frames) > initialFrameDepth {
		vm.popCallFrame()
	}

	return result, nil
}

func (vm *VM) handleCall(numArgs int, currentTok *Token) (bool, Error) {
	calleeIdx := vm.sp - 1 - numArgs
	if calleeIdx < 0 {
		return false, vm.runtimeError("Stack underflow during call setup")
	}

	callee := vm.stack[calleeIdx]
	args := vm.stack[calleeIdx+1 : vm.sp]

	switch c := callee.(type) {
	case *BoundMethodObj:
		if nativeMethod, ok := c.Method.(*NativeFuncObj); ok && nativeMethod.DirectCall != nil {
			newArgs := make([]Object, len(args)+1)
			newArgs[0] = c.Receiver
			copy(newArgs[1:], args)

			if len(newArgs) != nativeMethod.Arity {
				return false, vm.runtimeError("Method '%s' expected %d arguments, but got %d", nativeMethod.Name, nativeMethod.Arity, len(newArgs))
			}

			var result Object
			var err Error

			switch nativeMethod.Arity {
			case 2:
				if fn, ok := nativeMethod.DirectCall.(NativeFunc2); ok {
					result, err = fn(vm, newArgs[0], newArgs[1])
				}
			}

			if err != nil {
				return false, err
			}
			if result != nil {
				vm.sp = calleeIdx
				vm.push(result)
				return false, nil
			}
		}

		vm.stack[calleeIdx] = c.Method
		vm.push(nil)
		copy(vm.stack[calleeIdx+2:], vm.stack[calleeIdx+1:calleeIdx+1+numArgs])
		vm.stack[calleeIdx+1] = c.Receiver
		return vm.handleCall(numArgs+1, currentTok)
	case *FunctionObj:
		if numArgs != c.Arity {
			return false, vm.runtimeError("Function '%s' expected %d arguments, but got %d", c.Name, c.Arity, numArgs)
		}

		frame := &CallFrame{
			ReturnIP:  vm.ip,
			StackSlot: calleeIdx,
			EnvDepth:  len(vm.environments),
		}
		vm.frames = append(vm.frames, frame)
		vm.ip = *c.StartIP
		return true, nil 
	case *NativeFuncObj:
		if c.DirectCall != nil {
			if numArgs != c.Arity {
				return false, vm.runtimeError("Function '%s' expected %d arguments, but got %d", c.Name, c.Arity, numArgs)
			}
			
			var result Object
			var err Error

			switch c.Arity {
			case 0:
				if fn, ok := c.DirectCall.(NativeFunc0); ok {
					result, err = fn(vm)
				}
			case 1:
				if fn, ok := c.DirectCall.(NativeFunc1); ok {
					result, err = fn(vm, args[0])
				}
			}

			if err != nil {
				return false, err
			}
			if result != nil {
				vm.sp = calleeIdx
				vm.push(result)
				return false, nil
			}
		}

		if c.ReflectCall != nil {
			// The arity check for reflection-based calls is handled inside ReflectCall
			result, err := c.ReflectCall(vm, args)
			if err != nil {
				return false, err
			}
			vm.sp = calleeIdx
			vm.push(result)
			return false, nil
		}

		return false, vm.runtimeError("Cannot call uncallable native function '%s'", c.Name)

	default:
		return false, vm.runtimeError("Cannot call non-function type '%s'", callee.Type())
	}
}

func (vm *VM) currentInstruction() *Instruction {
	if vm.ip >= len(vm.bytecodeChunk) {
		return nil
	}
	return &vm.bytecodeChunk[vm.ip]
}

func (vm *VM) push(value Object) {
	if vm.sp >= len(vm.stack) {
		vm.stack = append(vm.stack, value)
	} else {
		vm.stack[vm.sp] = value
	}
	vm.sp++
}

func (vm *VM) pop() (Object, Error) {
	if vm.sp == 0 {
		return nil, vm.runtimeError("Stack underflow, cannot pop value")
	}
	vm.sp--
	return vm.stack[vm.sp], nil
}

func (vm *VM) popCallFrame() (CallFrame, Error) {
	if len(vm.frames) == 0 {
		return CallFrame{}, vm.runtimeError("No call frames to pop")
	}
	frame := vm.frames[len(vm.frames)-1]
	vm.frames = vm.frames[:len(vm.frames)-1]
	return *frame, nil
}

func (vm *VM) popEnv() Error {
	if len(vm.environments) == 0 {
		return vm.runtimeError("No environments to pop")
	}
	vm.environments = vm.environments[:len(vm.environments)-1]
	return nil
}

func (vm *VM) run(targetFrameDepth int) Result[Object] {
	for len(vm.frames) >= targetFrameDepth {
		if vm.ip >= len(vm.bytecodeChunk) {
			break
		}

		currInstr := vm.currentInstruction()
		if currInstr == nil {
			break
		}

		vm.ip++
		op := currInstr.Op
		operand := currInstr.Operand
		currentTok := currInstr.Token

		switch op {
		case OpPop:
			_, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
		case OpConst:
			val := vm.constants[*operand.(*int)]
			vm.push(val)
		case OpEnterScope:
			vm.environments = append(vm.environments, make(map[string]*Variable))
		case OpExitScope:
			err := vm.popEnv()
			if err != nil {
				return ResErr[Object](err)
			}
		case OpDefGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			vm.globals[name] = &Variable{Name: name, Value: val, IsConst: false}
		case OpDefConstGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			vm.globals[name] = &Variable{Name: name, Value: val, IsConst: true}
		case OpGetGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			variable, ok := vm.globals[name]
			if !ok {
				return ResErr[Object](vm.runtimeError("Undefined variable '%s'", name))
			}
			vm.push(variable.Value)
		case OpSetGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if _, ok := vm.globals[name]; !ok {
				return ResErr[Object](vm.runtimeError("Undefined variable '%s'", name))
			}
			if vm.globals[name].IsConst {
				return ResErr[Object](vm.runtimeError("Cannot assign to constant variable '%s'", name))
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			vm.globals[name].Value = value
		case OpDefLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			// name := vm.constants[nameIdx].(StringObj).Value
			if len(vm.environments) == 0 {
				return ResErr[Object](vm.runtimeError("No active local scope active for OP_DEF_LOCAL"))
			}
			if vm.sp == 0 {
				return ResErr[Object](vm.runtimeError("Stack underflow for OP_DEF_LOCAL"))
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return ResErr[Object](vm.runtimeError("Local variable '%s' already defined in this scope", name))
			}

			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			currentScope[name] = &Variable{Name: name, Value: val, IsConst: false}
		case OpGetLocal:
			varScoped := operand.(*VariableScoped)
			
			var targetDepth int
			if len(vm.frames) > 0 {
				frame := vm.frames[len(vm.frames)-1]
				targetDepth = frame.EnvDepth + varScoped.Depth
			} else {
				targetDepth = varScoped.Depth
			}

			if targetDepth < 0 || targetDepth >= len(vm.environments) {
				return ResErr[Object](vm.runtimeError("Internal error: invalid scope depth for '%s'", varScoped.Name))
			}

			scope := vm.environments[targetDepth]
			if variable, ok := scope[varScoped.Name]; ok {
				vm.push(variable.Value)
			} else {
				return ResErr[Object](vm.runtimeError("Undefined local variable '%s'", varScoped.Name))
			}
		case OpDefConstLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			if len(vm.environments) == 0 {
				return ResErr[Object](vm.runtimeError("No active local scope active for OP_DEF_CONST_LOCAL"))
			}
			if vm.sp == 0 {
				return ResErr[Object](vm.runtimeError("Stack underflow for OP_DEF_CONST_LOCAL"))
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return ResErr[Object](vm.runtimeError("Local variable '%s' already defined in this scope", name))
			}

			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			currentScope[name] = &Variable{Name: name, Value: val, IsConst: true}
		case OpSetLocal:
			varScoped := operand.(*VariableScoped)

			valToAssign, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			var targetDepth int
			if len(vm.frames) > 0 {
				// if there are active call frames adjust frame depth
				frame := vm.frames[len(vm.frames)-1]
				targetDepth = frame.EnvDepth + varScoped.Depth
			} else {
				targetDepth = varScoped.Depth
			}

			if targetDepth < 0 || targetDepth >= len(vm.environments) {
				return ResErr[Object](vm.runtimeError("Internal error: invalid scope depth for '%s'", varScoped.Name))
			}

			scope := vm.environments[targetDepth]
			if variable, ok := scope[varScoped.Name]; ok {
				if variable.IsConst {
					return ResErr[Object](vm.runtimeError("Cannot assign to const local variable '%s'", varScoped.Name))
				}
				variable.Value = valToAssign
			} else {
				return ResErr[Object](vm.runtimeError("Cannot assign to undefined local variable '%s'", varScoped.Name))
			}

		case OpAdd, OpSubtract, OpMultiply, OpDivide, OpModulo:
			if err := vm.binaryOp(op, currentTok); err != nil {
				return ResErr[Object](err)
			}
		case OpEqual, OpNotEqual, OpGreater, OpGreaterEqual, OpLess, OpLessEqual:
			if err := vm.binaryOpCompare(op, currentTok); err != nil {
				return ResErr[Object](err)
			}
		case OpInplaceAdd, OpInplaceSubtract, OpInplaceMultiply, OpInplaceDivide, OpInplaceModulo:
			if err := vm.inplaceOp(op, operand, currentTok); err != nil {
				return ResErr[Object](err)
			}

		case OpReturn:
			if vm.sp == 0 {
				return ResErr[Object](vm.runtimeError("Stack underflow for OP_RETURN (no return value)."))
			}
			returnVal, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if len(vm.frames) == 0 {
				return ResOk(returnVal)
			}

			frame, err := vm.popCallFrame()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.ip = frame.ReturnIP

			for len(vm.environments) > frame.EnvDepth {
				if err := vm.popEnv(); err != nil {
					return ResErr[Object](err)
				}
			}

			vm.sp = frame.StackSlot
			vm.push(returnVal)

		case OpBuildKwargs:
			numKwargs := *operand.(*int)
			if vm.sp < numKwargs+1 {
				return ResErr[Object](vm.runtimeError("Stack underflow for OP_BUILD_KWARGS"))
			}
		case OpCall:
			numArgs := *operand.(*int)
			if _, err := vm.handleCall(numArgs, currentTok); err != nil {
				return ResErr[Object](err)
			}
		case OpGetAttr:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			obj, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if getter, ok := obj.(AttributeGetter); ok {
				attr, found, err := getter.GetAttribute(name)
				if err != nil {
					return ResErr[Object](err)
				}
				if found {
					vm.push(attr)
					continue
				}
			}

			return ResErr[Object](NewRuntimeError(fmt.Sprintf("type '%s' has no attribute '%s'", obj.Type(), name), currentTok))
		case OpSetAttr:
			nameIdx := *operand.(*int)
			attrNameObj, ok := vm.constants[nameIdx].(StringObj)
			if !ok {
				return ResErr[Object](NewRuntimeError("Object attribute must be a string", currentTok))
			}

			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			obj, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if pyleMap, ok := obj.(*MapObj); ok {
				if err := pyleMap.Set(attrNameObj, value); err != nil {
					return ResErr[Object](NewRuntimeError(err.Error(), currentTok))
				}
				vm.push(value)
			} else {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Cannot set property on non-object type '%s'", obj.Type()), currentTok))
			}
		case OpTrue:
			vm.push(BooleanObj{Value: true})
		case OpFalse:
			vm.push(BooleanObj{Value: false})
		case OpNull:
			vm.push(NullObj{})
		case OpAnd:
			if vm.sp < 2 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_AND", currentTok))
			}
			right, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			left, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: left.IsTruthy() && right.IsTruthy()})

		case OpOr:
			if vm.sp < 2 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_OR", currentTok))
			}
			right, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			left, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: left.IsTruthy() || right.IsTruthy()})

		case OpNegate:
			if vm.sp == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_NEGATE", currentTok))
			}

			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if num, ok := value.(NumberObj); ok {
				num.Value = -num.Value
				vm.push(num)
			} else {
				return ResErr[Object](NewRuntimeError("Operand of unary OP_NEGATE must be a number", currentTok))
			}
		case OpNot:
			if vm.sp == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_NOT", currentTok))
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: !value.IsTruthy()})

		case OpBuildRange:
			if vm.sp < 3 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_BUILD_RANGE", currentTok))
			}

			step, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			end, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			start, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			startObj, ok := start.(NumberObj)
			if !ok {
				return ResErr[Object](NewRuntimeError("Range start must be a number", currentTok))
			}
			endObj, ok := end.(NumberObj)
			if !ok {
				return ResErr[Object](NewRuntimeError("Range end must be a number", currentTok))
			}
			stepObj, ok := step.(NumberObj)
			if !ok {
				return ResErr[Object](NewRuntimeError("Range step must be a number", currentTok))
			}

			vm.push(
				&RangeObj{
					Start: int(startObj.Value),
					End:   int(endObj.Value),
					Step:  int(stepObj.Value),
				},
			)

		case OpIterNew:
			if vm.sp == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_ITER_NEW", currentTok))
			}
			iterable, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			iterator, err := iterable.Iter()
			if err != nil {
				return ResErr[Object](err)
			}
			vm.push(iterator)

		case OpIterNextOrJump:
			offset := *operand.(*int)

			if vm.sp == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_ITER_NEXT_OR_JUMP", currentTok))
			}

			iterator, ok := vm.stack[vm.sp-1].(Iterator)
			if !ok {
				return ResErr[Object](NewRuntimeError("Object on stack is not an iterator", currentTok))
			}

			nextVal, hasNext := iterator.Next()
			if hasNext {
				vm.push(nextVal)
			} else {
				vm.ip += offset
			}

		case OpJump:
			offset := *operand.(*int)
			vm.ip += offset
		case OpJumpIfFalse:
			offset := *operand.(*int)
			condition, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			if !condition.IsTruthy() {
				vm.ip += offset
			}

		case OpBuildList:
			numElms := *operand.(*int)
			if vm.sp < numElms {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_BUILD_LIST", currentTok))
			}

			elements := make([]Object, numElms)
			for i := numElms - 1; i >= 0; i-- {
				popVal, err := vm.pop()
				if err != nil {
					return ResErr[Object](err)
				}
				elements[i] = popVal
			}
			vm.push(&ArrayObj{Elements: elements})
		case OpBuildMap:
			numProps := *operand.(*int)
			if vm.sp < numProps*2 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_BUILD_MAP", currentTok))
			}

			obj := NewMap()

			for i := 0; i < numProps; i++ {
				val, err := vm.pop()
				if err != nil {
					return ResErr[Object](err)
				}
				key, err := vm.pop()
				if err != nil {
					return ResErr[Object](err)
				}

				if err := obj.Set(key, val); err != nil {
					return ResErr[Object](NewRuntimeError(err.Error(), currentTok))
				}
			}
			vm.push(obj)
		case OpIndexGet:
			if vm.sp < 2 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_INDEX_GET", currentTok))
			}

			idx, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			collection, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			switch coll := collection.(type) {
			case *ArrayObj, *TupleObj, StringObj:
				index, ok, err := vm.coerceIndexToInt(idx)
				if err != nil {
					return ResErr[Object](err)
				}
				if !ok {
					return ResErr[Object](NewRuntimeError(fmt.Sprintf("Unsupported index type '%s' for sequence", idx.Type()), currentTok))
				}

				switch seq := collection.(type) {
				case *ArrayObj:
					if index < 0 || index >= len(seq.Elements) {
						return ResErr[Object](NewRuntimeError(fmt.Sprintf("Array index out of bounds: %d", index), currentTok))
					}
					vm.push(seq.Elements[index])
				case *TupleObj:
					if index < 0 || index >= len(seq.Elements) {
						return ResErr[Object](NewRuntimeError(fmt.Sprintf("Tuple index out of bounds: %d", index), currentTok))
					}
					vm.push(seq.Elements[index])
				case StringObj:
					if index < 0 || index >= len(seq.Value) {
						return ResErr[Object](NewRuntimeError(fmt.Sprintf("String index out of bounds: %d", index), currentTok))
					}
					vm.push(StringObj{Value: string(seq.Value[index])})
				}

			case *MapObj:
				val, found, err := coll.Get(idx)
				if err != nil {
					return ResErr[Object](NewRuntimeError(err.Error(), currentTok))
				}
				if !found {
					vm.push(NullObj{})
				} else {
					vm.push(val)
				}
			default:
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Object of type '%s' does not support indexing", collection.Type()), currentTok))
			}
		case OpIndexSet:
			if vm.sp < 3 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_INDEX_SET", currentTok))
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			idx, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			collection, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			switch coll := collection.(type) {
			case *ArrayObj:
				index, ok, err := vm.coerceIndexToInt(idx)
				if err != nil {
					return ResErr[Object](err)
				}
				if !ok {
					return ResErr[Object](NewRuntimeError(fmt.Sprintf("Unsupported index type '%s' for array", idx.Type()), currentTok))
				}

				if index < 0 || index >= len(coll.Elements) {
					return ResErr[Object](NewRuntimeError(fmt.Sprintf("Array index out of bounds: %d", index), currentTok))
				}
				coll.Elements[index] = value
				vm.push(value)
			case *MapObj:
				if err := coll.Set(idx, value); err != nil {
					return ResErr[Object](NewRuntimeError(err.Error(), currentTok))
				}
				vm.push(value)
			default:
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Object of type '%s' does not support index assignment", collection.Type()), currentTok))
			}
		default:
			return ResErr[Object](NewRuntimeError(fmt.Sprintf("Unknown opcode %s", op), currentTok))
		}
	}

	var lastVal Object = nil
	if vm.sp > 0 {
		lastVal = vm.stack[vm.sp-1]
	}
	return ResOk(lastVal)
}

func (vm *VM) coerceIndexToInt(idxObj Object) (int, bool, Error) {
	switch idx := idxObj.(type) {
	case NumberObj:
		if !idx.IsInt {
			return 0, false, NewRuntimeError("Sequence index must be an integer", nil)
		}
		return int(idx.Value), true, nil
	case BooleanObj:
		if idx.Value {
			return 1, true, nil
		}
		return 0, true, nil
	default:
		return 0, false, nil
	}
}

func (vm *VM) binaryOp(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return NewRuntimeError(fmt.Sprintf("Stack underflow for %s", op), currentTok)
	}
	right, err := vm.pop()
	if err != nil {
		return err
	}
	left, err := vm.pop()
	if err != nil {
		return err
	}

	switch l := left.(type) {
	case NumberObj:
		r, ok := right.(NumberObj)
		if !ok {
			return NewRuntimeError(fmt.Sprintf("unsupported operand type(s) for %s: 'number' and '%s'", op, right.Type()), currentTok)
		}

		var result float64
		isInt := false

		switch op {
		case OpAdd:
			result = l.Value + r.Value
			isInt = l.IsInt && r.IsInt
		case OpSubtract:
			result = l.Value - r.Value
			isInt = l.IsInt && r.IsInt
		case OpMultiply:
			result = l.Value * r.Value
			isInt = l.IsInt && r.IsInt
		case OpDivide:
			if r.Value == 0 {
				return NewRuntimeError("Division by zero", currentTok)
			}
			result = l.Value / r.Value
			isInt = false
		case OpModulo:
			if r.Value == 0 {
				return NewRuntimeError("Modulo by zero", currentTok)
			}
			if l.IsInt && r.IsInt {
				result = float64(int64(l.Value) % int64(r.Value))
				isInt = true
			} else {
				result = math.Mod(l.Value, r.Value)
				isInt = false
			}
		}
		vm.push(NumberObj{Value: result, IsInt: isInt})

	case StringObj:
		if op == OpAdd {
			if r, ok := right.(StringObj); ok {
				vm.push(StringObj{Value: l.Value + r.Value})
			} else {
				return NewRuntimeError(fmt.Sprintf("unsupported operand type(s) for +: 'string' and '%s'", right.Type()), currentTok)
			}
		} else {
			return NewRuntimeError(fmt.Sprintf("unsupported operand type(s) for %s: 'string'", op), currentTok)
		}

	default:
		return NewRuntimeError(fmt.Sprintf("unsupported operand type(s) for %s: '%s' and '%s'", op, left.Type(), right.Type()), currentTok)
	}

	return nil
}

func (vm *VM) binaryOpCompare(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return NewRuntimeError(fmt.Sprintf("Stack underflow for %s", op), currentTok)
	}
	right, err := vm.pop()
	if err != nil {
		return err
	}
	left, err := vm.pop()
	if err != nil {
		return err
	}

	comparable, ok := left.(Comparable)
	if !ok {
		var result bool
		switch op {
		case OpEqual:
			result = left == right
		case OpNotEqual:
			result = left != right
		default:
			return NewRuntimeError(fmt.Sprintf("type '%s' does not support ordering comparisons", left.Type()), currentTok)
		}
		vm.push(BooleanObj{Value: result})
		return nil
	}

	cmpResult, cmpErr := comparable.Compare(right)
	if cmpErr != nil {
		return NewRuntimeError(cmpErr.Error(), currentTok)
	}

	var result bool
	switch op {
	case OpEqual:
		result = cmpResult == 0
	case OpNotEqual:
		result = cmpResult != 0
	case OpGreater:
		result = cmpResult > 0
	case OpGreaterEqual:
		result = cmpResult >= 0
	case OpLess:
		result = cmpResult < 0
	case OpLessEqual:
		result = cmpResult <= 0
	}
	vm.push(BooleanObj{Value: result})
	return nil
}

func (vm *VM) inplaceOp(op OpCode, operand any, currentTok *Token) Error {
	nameIdx := *operand.(*int)
	name := vm.constants[nameIdx].(StringObj).Value

	var variable *Variable
	found := false
	for i := len(vm.environments) - 1; i >= 0; i-- {
		scope := vm.environments[i]
		if v, ok := scope[name]; ok {
			variable = v
			found = true
			break
		}
	}

	if !found {
		if v, ok := vm.globals[name]; ok {
			variable = v
		} else {
			return NewRuntimeError(fmt.Sprintf("Undefined variable '%s'", name), currentTok)
		}
	}

	if variable.IsConst {
		return NewRuntimeError(fmt.Sprintf("Cannot assign to constant variable '%s'", name), currentTok)
	}

	vm.push(variable.Value)

	var binaryOpCode OpCode
	switch op {
	case OpInplaceAdd:
		binaryOpCode = OpAdd
	case OpInplaceSubtract:
		binaryOpCode = OpSubtract
	case OpInplaceMultiply:
		binaryOpCode = OpMultiply
	case OpInplaceDivide:
		binaryOpCode = OpDivide
	case OpInplaceModulo:
		binaryOpCode = OpModulo
	default:
		return NewRuntimeError(fmt.Sprintf("internal VM error: unhandled inplace operator %s", op), currentTok)
	}

	if err := vm.binaryOp(binaryOpCode, currentTok); err != nil {
		return err
	}

	result, err := vm.pop()
	if err != nil {
		return err
	}

	variable.Value = result
	return nil
}

func (vm *VM) runtimeError(format string, args ...interface{}) Error {
	return NewRuntimeError(fmt.Sprintf(format, args...), nil)
}