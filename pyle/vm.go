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
    Closure   *ClosureObj
}

const InitialStackCapacity = 256

type VM struct {
	bytecodeChunk []Instruction
	constants     []Object
	ip            int
	stack         []Object
	sp            int // Stack pointer
	currToken     *Token
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
		return vm.runtimeError("Global variable '%s' already defined %s", name, vm.currToken.GetFileLoc())
	}
	vm.globals[name] = &Variable{Name: name, Value: value, IsConst: false}
	return nil
}

func (vm *VM) RegisterModule(name string, funcs map[string]any, doc *DocstringObj) Error {
	module := NewModule(name)
	module.Doc = doc
	for funcName, fn := range funcs {
		var fnDoc *DocstringObj
		docMap, ok := BuiltinMethodDocs[name]
		if ok {
			fnDoc = docMap[funcName]
		}
		nativeFunc, err := CreateNativeFunction(funcName, fn, fnDoc)
		if err != nil {
			return vm.runtimeError("Error creating native function '%s' for module '%s': %v %s", funcName, name, err, vm.currToken.GetFileLoc())
		}
		if err := module.Methods.Set(StringObj{Value: funcName}, nativeFunc); err != nil {
			return vm.runtimeError("Error adding function '%s' to module '%s': %v %s", funcName, name, err, vm.currToken.GetFileLoc())
		}
	}
	return vm.AddGlobal(name, module)
}


func (vm *VM) LoadBuiltins() error {
	for name, fn := range Builtins {
		doc := BuiltinDocs[name]
		err := vm.RegisterGOFunction(name, fn, doc)
		if err != nil {
			return err
		}
	}

	for name, functions := range BuiltinModules {
		if err := vm.RegisterModule(name, functions, BuiltinModuleDocs[name]); err != nil {
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
		return false, vm.runtimeError("Stack underflow during call setup at %s", currentTok.GetFileLoc())
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
				return false, vm.runtimeError("Method '%s' expected %d arguments, but got %d %s", nativeMethod.Name, nativeMethod.Arity, len(newArgs), currentTok.GetFileLoc())
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
    case *ClosureObj:
        if numArgs != c.Function.Arity {
            return false, vm.runtimeError("Function '%s' expected %d arguments, but got %d %s", c.Function.Name, c.Function.Arity, numArgs, currentTok.GetFileLoc())
        }
        frame := &CallFrame{
            ReturnIP:  vm.ip,
            StackSlot: calleeIdx,
            EnvDepth:  len(vm.environments),
            Closure:   c,
        }
        vm.frames = append(vm.frames, frame)
        vm.ip = *c.Function.StartIP
        return true, nil
    case *FunctionObj:
        if numArgs != c.Arity {
			return false, vm.runtimeError("Function '%s' expected %d arguments, but got %d %s", c.Name, c.Arity, numArgs, currentTok.GetFileLoc())
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
				return false, vm.runtimeError("Function '%s' expected %d arguments, but got %d %s", c.Name, c.Arity, numArgs, currentTok.GetFileLoc())
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
			case 2:
				if fn, ok := c.DirectCall.(NativeFunc2); ok {
					result, err = fn(vm, args[0], args[1])
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

		return false, vm.runtimeError("Cannot call uncallable native function '%s' at %s", c.Name, currentTok.GetFileLoc())

	default:
		return false, vm.runtimeError("Cannot call non-function type '%s' at %s", callee.Type(), currentTok.GetFileLoc())
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
		vm.currToken = currInstr.Token
		currentTok := vm.currToken

		switch op {
		case OpPop:
			_, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
        case OpConst:
			val := vm.constants[*operand.(*int)]
			if fn, ok := val.(*FunctionObj); ok {
				if fn.CaptureDepth > 0 {
					numToCapture := fn.CaptureDepth
					if numToCapture > len(vm.environments) {
						// This should not happen in correct code, but as a safeguard:
						numToCapture = len(vm.environments)
					}
					captured := make([]map[string]*Variable, 0, numToCapture)
					// Capture from inner-most (end of slice) to outer-most
					for i := 0; i < numToCapture; i++ {
						captured = append(captured, vm.environments[len(vm.environments)-1-i])
					}
					vm.push(&ClosureObj{Function: fn, Captured: captured})
				} else {
					vm.push(fn) // No environments to capture, push raw function
				}
			} else {
				vm.push(val)
			}
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
			if _, ok := vm.globals[name]; ok {
				return vm.runtimeErrorRes("Global variable '%s' already defined at %s", name, currentTok.GetFileLoc())
			}
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
				return vm.runtimeErrorRes("Undefined variable '%s' at %s", name, currentTok.GetFileLoc())
			}
			vm.push(variable.Value)
		case OpSetGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if _, ok := vm.globals[name]; !ok {
				return vm.runtimeErrorRes("Undefined variable '%s' at %s", name, currentTok.GetFileLoc())
			}
			if vm.globals[name].IsConst {
				return vm.runtimeErrorRes("Cannot assign to constant variable '%s' at %s", name, currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("No active local scope active for OP_DEF_LOCAL")
			}
			if vm.sp == 0 {
				return vm.runtimeErrorRes("Stack underflow for OP_DEF_LOCAL")
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return vm.runtimeErrorRes("Local variable '%s' already defined in this scope at %s", name, currentTok.GetFileLoc())
			}

			val, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			currentScope[name] = &Variable{Name: name, Value: val, IsConst: false}
        case OpGetLocal:
			varScoped := operand.(*VariableScoped)
			var scope map[string]*Variable

			if len(vm.frames) > 0 {
				frame := vm.frames[len(vm.frames)-1]
				// Check if the variable is in the current function's environment stack
				if frame.EnvDepth <= len(vm.environments)-1-varScoped.Depth {
					scope = vm.environments[len(vm.environments)-1-varScoped.Depth]
				} else if frame.Closure != nil {
					// It's a captured variable (upvalue)
					upvalueIndex := varScoped.Depth - (len(vm.environments) - frame.EnvDepth)
					if upvalueIndex >= 0 && upvalueIndex < len(frame.Closure.Captured) {
						scope = frame.Closure.Captured[upvalueIndex]
					}
				}
			} else {
				// Global script scope
				scope = vm.environments[len(vm.environments)-1-varScoped.Depth]
			}

			if scope == nil {
				return vm.runtimeErrorRes("Undefined local variable '%s' at %s", varScoped.Name, currentTok.GetFileLoc())
			}

			if variable, ok := scope[varScoped.Name]; ok {
				vm.push(variable.Value)
			} else {
				return vm.runtimeErrorRes("Undefined local variable '%s' at %s", varScoped.Name, currentTok.GetFileLoc())
			}
		case OpDefConstLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			if len(vm.environments) == 0 {
				return vm.runtimeErrorRes("No active local scope active for OP_DEF_CONST_LOCAL")
			}
			if vm.sp == 0 {
				return vm.runtimeErrorRes("Stack underflow for OP_DEF_CONST_LOCAL")
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return vm.runtimeErrorRes("Local variable '%s' already defined in this scope at %s", name, currentTok.GetFileLoc())
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
			var scope map[string]*Variable

			if len(vm.frames) > 0 {
				frame := vm.frames[len(vm.frames)-1]
				// Check if the variable is in the current function's environment stack
				if frame.EnvDepth <= len(vm.environments)-1-varScoped.Depth {
					scope = vm.environments[len(vm.environments)-1-varScoped.Depth]
				} else if frame.Closure != nil {
					// It's a captured variable (upvalue)
					upvalueIndex := varScoped.Depth - (len(vm.environments) - frame.EnvDepth)
					if upvalueIndex >= 0 && upvalueIndex < len(frame.Closure.Captured) {
						scope = frame.Closure.Captured[upvalueIndex]
					}
				}
			} else {
				// Global script scope
				scope = vm.environments[len(vm.environments)-1-varScoped.Depth]
			}

			if scope == nil {
				return vm.runtimeErrorRes("Undefined local variable '%s' at %s", varScoped.Name, currentTok.GetFileLoc())
			}

			if variable, ok := scope[varScoped.Name]; ok {
				if variable.IsConst {
					return vm.runtimeErrorRes("Cannot assign to const local variable '%s' at %s", varScoped.Name, currentTok.GetFileLoc())
				}
				variable.Value = valToAssign
			} else {
				return vm.runtimeErrorRes("Cannot assign to undefined local variable '%s' at %s", varScoped.Name, currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_RETURN (no return value).")
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
				return vm.runtimeErrorRes("Stack underflow for OP_BUILD_KWARGS")
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
			return vm.runtimeErrorRes("type '%s' has no attribute '%s' at %s", obj.Type(), name, currentTok.GetFileLoc())
		case OpSetAttr:
			nameIdx := *operand.(*int)
			attrNameObj, ok := vm.constants[nameIdx].(StringObj)
			if !ok {
				return vm.runtimeErrorRes("Object attribute must be a string at %s", currentTok.GetFileLoc())
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
					return vm.runtimeErrorRes("%s at %s", err.Error(), currentTok.GetFileLoc())
				}
			} else {
				return vm.runtimeErrorRes("Cannot set property on non-object type '%s' at %s", obj.Type(), currentTok.GetFileLoc())
			}
		case OpTrue:
			vm.push(BooleanObj{Value: true})
		case OpFalse:
			vm.push(BooleanObj{Value: false})
		case OpNull:
			vm.push(NullObj{})
		case OpAnd:
			if vm.sp < 2 {
				return vm.runtimeErrorRes("Stack underflow for OP_AND at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_OR at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_NEGATE at %s", currentTok.GetFileLoc())
			}

			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if num, ok := value.(NumberObj); ok {
				num.Value = -num.Value
				vm.push(num)
			} else {
				return vm.runtimeErrorRes("Operand of unary OP_NEGATE must be a number at %s", currentTok.GetFileLoc())
			}
		case OpNot:
			if vm.sp == 0 {
				return vm.runtimeErrorRes("Stack underflow for OP_NOT at %s", currentTok.GetFileLoc())
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: !value.IsTruthy()})

		case OpBuildRange:
			if vm.sp < 3 {
				return vm.runtimeErrorRes("Stack underflow for OP_BUILD_RANGE at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Range start must be a number at %s", currentTok.GetFileLoc())
			}
			endObj, ok := end.(NumberObj)
			if !ok {
				return vm.runtimeErrorRes("Range end must be a number at %s", currentTok.GetFileLoc())
			}
			stepObj, ok := step.(NumberObj)
			if !ok {
				return vm.runtimeErrorRes("Range step must be a number at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_ITER_NEW at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_ITER_NEXT_OR_JUMP at %s", currentTok.GetFileLoc())
			}

			iterator, ok := vm.stack[vm.sp-1].(Iterator)
			if !ok {
				return vm.runtimeErrorRes("Object on stack is not an iterator at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_BUILD_LIST at %s", currentTok.GetFileLoc())
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
				return vm.runtimeErrorRes("Stack underflow for OP_BUILD_MAP at %s", currentTok.GetFileLoc())
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
					return vm.runtimeErrorRes("%s at %s", err.Error(), currentTok.GetFileLoc())
				}
			}
			vm.push(obj)
		case OpUnpack:
			expected := *operand.(*int)
			if vm.sp == 0 {
				return vm.runtimeErrorRes("Stack underflow for OP_UNPACK at %s", currentTok.GetFileLoc())
			}
			value, err := vm.pop()
			if err != nil { return ResErr[Object](err) }
			switch v := value.(type) {
			case *TupleObj:
				if len(v.Elements) != expected {
					return vm.runtimeErrorRes("unpack mismatch: expected %d values, got %d at %s", expected, len(v.Elements), currentTok.GetFileLoc())
				}
				for i := len(v.Elements) - 1; i >= 0; i-- { vm.push(v.Elements[i]) }
			case *ArrayObj:
				if len(v.Elements) != expected {
					return vm.runtimeErrorRes("unpack mismatch: expected %d values, got %d at %s", expected, len(v.Elements), currentTok.GetFileLoc())
				}
				for i := len(v.Elements) - 1; i >= 0; i-- { vm.push(v.Elements[i]) }
			default:
				return vm.runtimeErrorRes("object of type '%s' is not unpackable at %s", value.Type(), currentTok.GetFileLoc())
			}
		case OpIndexGet:
			if vm.sp < 2 {
				return vm.runtimeErrorRes("Stack underflow for OP_INDEX_GET at %s", currentTok.GetFileLoc())
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
					return vm.runtimeErrorRes("Unsupported index type '%s' for sequence at %s", idx.Type(), currentTok.GetFileLoc())
				}

				switch seq := collection.(type) {
				case *ArrayObj:
					if index < 0 || index >= len(seq.Elements) {
						return vm.runtimeErrorRes("Array index out of bounds: %d at %s", index, currentTok.GetFileLoc())
					}
					vm.push(seq.Elements[index])
				case *TupleObj:
					if index < 0 || index >= len(seq.Elements) {
						return vm.runtimeErrorRes("Tuple index out of bounds: %d at %s", index, currentTok.GetFileLoc())
					}
					vm.push(seq.Elements[index])
				case StringObj:
					if index < 0 || index >= len(seq.Value) {
						return vm.runtimeErrorRes("String index out of bounds: %d at %s", index, currentTok.GetFileLoc())
					}
					vm.push(StringObj{Value: string(seq.Value[index])})
				}

			case *MapObj:
				val, found, err := coll.Get(idx)
				if err != nil {
					return vm.runtimeErrorRes("%s at %s", err.Error(), currentTok.GetFileLoc())
				}
				if !found {
					vm.push(NullObj{})
				} else {
					vm.push(val)
				}
			default:
				return vm.runtimeErrorRes("Object of type '%s' does not support indexing at %s", collection.Type(), currentTok.GetFileLoc())
			}
		case OpIndexSet:
			if vm.sp < 3 {
				return vm.runtimeErrorRes("Stack underflow for OP_INDEX_SET at %s", currentTok.GetFileLoc())
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
					return vm.runtimeErrorRes("Unsupported index type '%s' for array at %s", idx.Type(), currentTok.GetFileLoc())
				}

				if index < 0 || index >= len(coll.Elements) {
					return vm.runtimeErrorRes("Array index out of bounds: %d at %s", index, currentTok.GetFileLoc())
				}
				coll.Elements[index] = value
			case *MapObj:
				if err := coll.Set(idx, value); err != nil {
					return vm.runtimeErrorRes("%s at %s", err.Error(), currentTok.GetFileLoc())
				}
			default:
				return vm.runtimeErrorRes("Object of type '%s' does not support index assignment at %s", collection.Type(), currentTok.GetFileLoc())
			}
		default:
			return vm.runtimeErrorRes("Unknown opcode %s at %s", op, currentTok.GetFileLoc())
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
			return 0, false, vm.runtimeError("Sequence index must be an integer")
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

func (vm *VM) doBinaryOp(op OpCode, left, right Object, currentTok *Token) (Object, Error) {
	switch l := left.(type) {
	case NumberObj:
		r, ok := right.(NumberObj)
		if !ok {
			return nil, vm.runtimeError("unsupported operand type(s) for %s: 'number' and '%s' at %s", op, right.Type(), currentTok.GetFileLoc())
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
				return nil, vm.runtimeError("Division by zero at %s", currentTok.GetFileLoc())
			}
			result = l.Value / r.Value
			isInt = false
		case OpModulo:
			if r.Value == 0 {
				return nil, vm.runtimeError("Modulo by zero at %s", currentTok.GetFileLoc())
			}
			if l.IsInt && r.IsInt {
				result = float64(int64(l.Value) % int64(r.Value))
				isInt = true
			} else {
				result = math.Mod(l.Value, r.Value)
				isInt = false
			}
		}
		return NumberObj{Value: result, IsInt: isInt}, nil

	case StringObj:
		if op == OpAdd {
			if r, ok := right.(StringObj); ok {
				return StringObj{Value: l.Value + r.Value}, nil
			} else {
				return nil, vm.runtimeError("unsupported operand type(s) for +: 'string' and '%s' at %s", right.Type(), currentTok.GetFileLoc())
			}
		} else {
			return nil, vm.runtimeError("unsupported operand type(s) for %s: 'string' at %s", op, currentTok.GetFileLoc())
		}

	default:
		return nil, vm.runtimeError("unsupported operand type(s) for %s: '%s' and '%s' at %s", op, left.Type(), right.Type(), currentTok.GetFileLoc())
	}
}

func (vm *VM) binaryOp(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return vm.runtimeError("Stack underflow for %s at %s", op, currentTok.GetFileLoc())
	}
	right, err := vm.pop()
	if err != nil {
		return err
	}
	left, err := vm.pop()
	if err != nil {
		return err
	}

	result, err := vm.doBinaryOp(op, left, right, currentTok)
	if err != nil {
		return err
	}

	vm.push(result)

	return nil
}

func (vm *VM) binaryOpCompare(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return vm.runtimeError("Stack underflow for %s at %s", op, currentTok.GetFileLoc())
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
			return vm.runtimeError("type '%s' does not support ordering comparisons", left.Type())
		}
		vm.push(BooleanObj{Value: result})
		return nil
	}

	cmpResult, cmpErr := comparable.Compare(right)
	if cmpErr != nil {
		return vm.runtimeError("%s at %s", cmpErr.Error(), currentTok.GetFileLoc())
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
	var variable *Variable

	switch opand := operand.(type) {
	case *VariableScoped:
		var scope map[string]*Variable

		if len(vm.frames) > 0 {
			frame := vm.frames[len(vm.frames)-1]
			// Check if the variable is in the current function's environment stack
			if frame.EnvDepth <= len(vm.environments)-1-opand.Depth {
				scope = vm.environments[len(vm.environments)-1-opand.Depth]
			} else if frame.Closure != nil {
				// It's a captured variable (upvalue)
				upvalueIndex := opand.Depth - (len(vm.environments) - frame.EnvDepth)
				if upvalueIndex >= 0 && upvalueIndex < len(frame.Closure.Captured) {
					scope = frame.Closure.Captured[upvalueIndex]
				}
			}
		} else {
			// Global script scope
			scope = vm.environments[len(vm.environments)-1-opand.Depth]
		}

		if scope == nil {
			return vm.runtimeError("Undefined local variable '%s'", opand.Name)
		}

		if v, ok := scope[opand.Name]; ok {
			variable = v
		} else {
			return vm.runtimeError("Undefined local variable '%s'", opand.Name)
		}
	case *int:
		nameIdx := *opand
		name := vm.constants[nameIdx].(StringObj).Value
		if v, ok := vm.globals[name]; ok {
			variable = v
		} else {
			return vm.runtimeError("Undefined variable '%s'", name)
		}
	default:
		return vm.runtimeError("internal VM error: unsupported operand type for inplace op")
	}

	if variable.IsConst {
		return vm.runtimeError("Cannot assign to constant variable '%s'", variable.Name)
	}

	right, err := vm.pop()
	if err != nil {
		return err
	}
	left := variable.Value

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
		return vm.runtimeError("internal VM error: unhandled inplace operator %s", op)
	}

	result, err := vm.doBinaryOp(binaryOpCode, left, right, currentTok)
	if err != nil {
		return err
	}

	variable.Value = result
	return nil
}

func (vm *VM) runtimeError(format string, args ...any) Error {
	return NewRuntimeError(fmt.Sprintf(format, args...), vm.currToken)
}

func (vm *VM) runtimeErrorRes(format string, args ...any) Result[Object] {
	return	ResErr[Object](vm.runtimeError(format, args...))
}
