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
	globals       map[string]*Variable
	environments  []map[string]*Variable
	frames        []*CallFrame
	Stdout        io.Writer
}

func NewVM() *VM {
	return &VM{
		stack:        make([]Object, 0, InitialStackCapacity),
		globals:      make(map[string]*Variable),
		environments: make([]map[string]*Variable, 0),
		frames:       make([]*CallFrame, 0),
		Stdout:       os.Stdout,
	}
}

func (vm *VM) AddGlobal(name string, value Object) Error {
	if _, ok := vm.globals[name]; ok {
		return NewRuntimeError(fmt.Sprintf("Global variable '%s' already defined", name), nil)
	}
	vm.globals[name] = &Variable{Name: name, Value: value, IsConst: false}
	return nil
}

func (vm *VM) LoadBuiltins() error {
	for name, fn := range Builtins {
		doc := BuiltinDocs[name] // doc is now *DocstringObj, or nil
		err := vm.RegisterGOFunction(name, fn, doc)
		if err != nil {
			return err
		}
	}
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

// Reset VM state and run the instructions
func (vm *VM) Interpret(bytecodeChunk []Instruction, constants []Object) Result[Object] {
	vm.bytecodeChunk = bytecodeChunk
	vm.constants = constants
	vm.ip = 0
	vm.stack = vm.stack[:0]
	vm.frames = vm.frames[:0]
	vm.environments = vm.environments[:0]

	return vm.run(0)
}

// CallFunction provides a Go-native way to call a Pyle function.
// This is the primary API for interoperability.
func (vm *VM) CallFunction(callable Object, args []Object) (Object, Error) {
	// This function is now a wrapper around the internal call handler.
	// It's primarily for external Go code to call into the VM.
	// We place the callable and args on the stack and call the handler.
	stackBottom := len(vm.stack)
	vm.push(callable)
	for _, arg := range args {
		vm.push(arg)
	}

	if _, err := vm.handleCall(len(args), nil); err != nil {
		return nil, err
	}

	// After handleCall, the VM's main loop (`run`) will execute the function.
	// We need to run the VM until the call frame we just created is popped.
	initialFrameCount := len(vm.frames)
	runResult := vm.run(initialFrameCount)
	if runResult.Err != nil {
		return nil, runResult.Err
	}

	// After the run, the return value should be on top of the stack.
	// We need to be careful to retrieve it correctly.
	if len(vm.stack) > stackBottom {
		returnVal := vm.stack[len(vm.stack)-1]
		vm.stack = vm.stack[:stackBottom] // Clean up the stack
		return returnVal, nil
	}

	// If the stack is empty or at its original level, it means the function
	// might have not returned a value, so we return null.
	return NullObj{}, nil
}

// callPyleFuncFromNative is an internal helper for native Go methods to call a Pyle function.
// It correctly manages the VM's execution loop to only run the called function and then return.
func (vm *VM) callPyleFuncFromNative(callable Object, args []Object) (Object, Error) {
	initialStackSize := len(vm.stack)
	initialFrameDepth := len(vm.frames)

	// Manually push the function and argument onto the stack
	vm.push(callable)
	for _, arg := range args {
		vm.push(arg)
	}

	// Use the internal handleCall to set up the frame
	pyleFuncCalled, err := vm.handleCall(len(args), nil)
	if err != nil {
		return nil, err
	}

	// If a Pyle function was called, a new frame was pushed. We need to run the VM
	// to execute it. If a native function was called, it already executed,
	// and the result is on the stack.
	if pyleFuncCalled {
		runResult := vm.run(len(vm.frames))
		if runResult.Err != nil {
			return nil, runResult.Err
		}
	}

	var result Object = NullObj{}
	// After the run, the result is on top of the stack.
	if len(vm.stack) > initialStackSize {
		result, err = vm.pop()
		if err != nil {
			return nil, err
		}
	}

	// Ensure the stack and frames are restored to their original state
	if len(vm.stack) > initialStackSize {
		vm.stack = vm.stack[:initialStackSize]
	}
	for len(vm.frames) > initialFrameDepth {
		vm.popCallFrame()
	}

	return result, nil
}

func (vm *VM) handleCall(numArgs int, currentTok *Token) (bool, Error) {
	calleeIdx := len(vm.stack) - 1 - numArgs
	if calleeIdx < 0 {
		return false, NewRuntimeError("Stack underflow during call setup", currentTok)
	}

	callee := vm.stack[calleeIdx]
	args := vm.stack[calleeIdx+1:]

	switch c := callee.(type) {
	case *BoundMethodObj:
		// For bound methods, the receiver becomes the first argument.
		// We replace the BoundMethodObj on the stack with the actual method.
		vm.stack[calleeIdx] = c.Method

		// We need to insert the receiver into the arguments list on the stack.
		// The arguments are currently at vm.stack[calleeIdx+1 : calleeIdx+1+numArgs].

		// Let's make space for the receiver.
		vm.push(nil) // Grow stack by 1. The value doesn't matter.

		// Shift arguments to the right.
		copy(vm.stack[calleeIdx+2:], vm.stack[calleeIdx+1:calleeIdx+1+numArgs])

		// Insert the receiver as the first argument.
		vm.stack[calleeIdx+1] = c.Receiver

		// Now we can call the underlying method with one additional argument.
		return vm.handleCall(numArgs+1, currentTok)
	case *FunctionObj:
		if numArgs != c.Arity {
			return false, NewRuntimeError(fmt.Sprintf("Function '%s' expected %d arguments, but got %d", c.Name, c.Arity, numArgs), currentTok)
		}

		frame := &CallFrame{
			ReturnIP:  vm.ip,
			StackSlot: calleeIdx,
			EnvDepth:  len(vm.environments),
		}
		vm.frames = append(vm.frames, frame)
		vm.ip = *c.StartIP
		return true, nil // Pyle function was called, new frame was pushed.

	case *NativeFuncObj:
		result, err := c.Call(vm, args, nil)
		if err != nil {
			return false, err
		}
		vm.stack = vm.stack[:calleeIdx] // Pop callee and args
		vm.push(result)
		return false, nil // Native function was called, no new frame.

	default:
		return false, NewRuntimeError(fmt.Sprintf("Cannot call non-function type '%s'", callee.Type()), currentTok)
	}
}



func (vm *VM) currentInstruction() *Instruction {
	if vm.ip >= len(vm.bytecodeChunk) {
		return nil
	}
	return &vm.bytecodeChunk[vm.ip]
}

func (vm *VM) push(value Object) {
	vm.stack = append(vm.stack, value)
}

func (vm *VM) pop() (Object, Error) {
	if len(vm.stack) == 0 {
		return nil, NewRuntimeError("Stack underflow, cannot pop value", nil)
	}
	index := len(vm.stack) - 1
	value := vm.stack[index]
	vm.stack = vm.stack[:index]
	return value, nil
}

func (vm *VM) popCallFrame() (CallFrame, Error) {
	if len(vm.frames) == 0 {
		return CallFrame{}, NewRuntimeError("No call frames to pop", nil)
	}
	frame := vm.frames[len(vm.frames)-1]
	vm.frames = vm.frames[:len(vm.frames)-1]
	return *frame, nil
}

func (vm *VM) popEnv() Error {
	if len(vm.environments) == 0 {
		return NewRuntimeError("No environments to pop", nil)
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
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Undefined variable '%s'", name), currentTok))
			}
			vm.push(variable.Value)
		case OpSetGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if _, ok := vm.globals[name]; !ok {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Undefined variable '%s'", name), currentTok))
			}
			if vm.globals[name].IsConst {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Cannot assign to constant variable '%s'", name), currentTok))
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			vm.globals[name].Value = value
		case OpDefLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if len(vm.environments) == 0 {
				return ResErr[Object](NewRuntimeError("No active local scope active for OP_DEF_LOCAL", currentTok))
			}
			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_DEF_LOCAL", currentTok))
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Local variable '%s' already defined in this scope", name), currentTok))
			}

			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			currentScope[name] = &Variable{Name: name, Value: val, IsConst: false}
		case OpGetLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			found := false
			for i := len(vm.environments) - 1; i >= 0; i-- {
				scope := vm.environments[i]
				if variable, ok := scope[name]; ok {
					vm.push(variable.Value)
					found = true
					break
				}
			}
			if found {
				break
			}

			if variable, ok := vm.globals[name]; ok {
				vm.push(variable.Value)
			} else {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Undefined variable '%s'", name), currentTok))
			}
		case OpDefConstLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			if len(vm.environments) == 0 {
				return ResErr[Object](NewRuntimeError("No active local scope active for OP_DEF_CONST_LOCAL", currentTok))
			}
			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_DEF_CONST_LOCAL", currentTok))
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Local variable '%s' already defined in this scope", name), currentTok))
			}

			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			currentScope[name] = &Variable{Name: name, Value: val, IsConst: true}
		case OpSetLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_SET_LOCAL", currentTok))
			}

			valToAssign, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			assigned := false
			for i := len(vm.environments) - 1; i >= 0; i-- {
				scope := vm.environments[i]
				if variable, ok := scope[name]; ok {
					if variable.IsConst {
						return ResErr[Object](NewRuntimeError(fmt.Sprintf("Cannot assign to const local variable '%s'", name), currentTok))
					}
					variable.Value = valToAssign
					assigned = true
					break
				}
			}
			if assigned {
				break
			}

			if variable, ok := vm.globals[name]; ok {
				if variable.IsConst {
					return ResErr[Object](NewRuntimeError(fmt.Sprintf("Cannot assign to const global variable '%s'", name), currentTok))
				}
				variable.Value = valToAssign
			} else {
				return ResErr[Object](NewRuntimeError(fmt.Sprintf("Cannot assign to undefined variable '%s'", name), currentTok))
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
			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_RETURN (no return value).", currentTok))
			}
			returnVal, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if len(vm.frames) == 0 {
				// If no frames, this is the end of the main script
				return ResOk(returnVal)
			}

			// Pop the current frame
			frame, err := vm.popCallFrame()
			if err != nil {
				return ResErr[Object](err)
			}

			// Restore the instruction pointer
			vm.ip = frame.ReturnIP

			// Restore the environment
			for len(vm.environments) > frame.EnvDepth {
				if err := vm.popEnv(); err != nil {
					return ResErr[Object](err)
				}
			}

			// Restore the stack to its state before the call, then push the return value
			vm.stack = vm.stack[:frame.StackSlot]
			vm.push(returnVal)

		case OpBuildKwargs:
			numKwargs := *operand.(*int)
			if len(vm.stack) < numKwargs+1 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_BUILD_KWARGS", currentTok))
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

			// Use the AttributeGetter interface for robust access
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

			// Fallback for types that don't implement AttributeGetter or attribute not found
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
			if len(vm.stack) < 2 {
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
			if len(vm.stack) < 2 {
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
			if len(vm.stack) == 0 {
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
			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_NOT", currentTok))
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: !value.IsTruthy()})

		case OpBuildRange:
			if len(vm.stack) < 3 {
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
			if len(vm.stack) == 0 {
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

			if len(vm.stack) == 0 {
				return ResErr[Object](NewRuntimeError("Stack underflow for OP_ITER_NEXT_OR_JUMP", currentTok))
			}

			iterator, ok := vm.stack[len(vm.stack)-1].(Iterator)
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
			if len(vm.stack) < numElms {
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
			if len(vm.stack) < numProps*2 {
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
			if len(vm.stack) < 2 {
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
			if len(vm.stack) < 3 {
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
	if len(vm.stack) > 0 {
		lastVal = vm.stack[len(vm.stack)-1]
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
	if len(vm.stack) < 2 {
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
	if len(vm.stack) < 2 {
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