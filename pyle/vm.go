package pyle

import (
	"fmt"
	"io"
	"math"
	"os"
	"strings"
	"sync"
)

type CallFrame struct {
	ReturnIP  int
	StackSlot int
	EnvDepth  int
	Closure   *ClosureObj
}

const InitialStackCapacity = 256

type VM struct {
	mu            sync.Mutex
	bytecodeChunk []Instruction
	constants     []Object
	ip            int
	stack         []Object
	sp            int // Stack pointer
	globals       map[string]*Variable
	environments  []map[string]*Variable
	frames        []*CallFrame
	Stdout        io.Writer

	moduleRegistry map[string]func(*VM) Object
}

func NewVM() *VM {
	return &VM{
		stack:        make([]Object, InitialStackCapacity),
		sp:           0,
		globals:      make(map[string]*Variable),
		environments: make([]map[string]*Variable, 0),
		frames:       make([]*CallFrame, 0),
		Stdout:       os.Stdout,
		moduleRegistry: make(map[string]func(*VM) Object),
	}
}

func (vm *VM) AddGlobal(name string, value Object) Error {
	vm.globals[name] = &Variable{Name: name, Value: value, IsConst: false}
	return nil
}

func (vm *VM) RegisterBuiltinModule(name string, constructor func(*VM) Object) {
	vm.moduleRegistry[name] = constructor
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
			return vm.runtimeError(Loc{}, "Error creating native function '%s' for module '%s': %v", funcName, name, err)
		}
		if err := module.Methods.Set(StringObj{Value: funcName}, nativeFunc); err != nil {
			return vm.runtimeError(Loc{}, "Error adding function '%s' to module '%s': %v", funcName, name, err)
		}
	}
	return vm.AddGlobal(name, module)
}

func (vm *VM) LoadBuiltins() error {
	for name, fn := range BuiltinFunctions {
		doc := BuiltinDocs[name]
		err := vm.RegisterGOFunction(name, fn, doc)
		if err != nil {
			return err
		}
	}

	// Register modules for lazy loading via 'use'
	for name, functions := range BuiltinModules {
		mName := name
		funcs := functions
		doc := BuiltinModuleDocs[mName]
		vm.RegisterBuiltinModule(mName, func(vm *VM) Object {
			module := NewModule(mName)
			module.Doc = doc
			for funcName, fn := range funcs {
				var fnDoc *DocstringObj
				docMap, ok := BuiltinMethodDocs[mName]
				if ok {
					fnDoc = docMap[funcName]
				}
				nativeFunc, _ := CreateNativeFunction(funcName, fn, fnDoc)
				module.Methods.Set(StringObj{Value: funcName}, nativeFunc)
			}
			return module
		})
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



func (vm *VM) Lock() {
	vm.mu.Lock()
}

func (vm *VM) Unlock() {
	vm.mu.Unlock()
}

func (vm *VM) CallFunction(callable Object, args []Object) (Object, Error) {
	vm.mu.Lock()
	defer vm.mu.Unlock()

	stackBottom := vm.sp
	vm.push(callable)
	for _, arg := range args {
		vm.push(arg)
	}

	if _, err := vm.handleCall(len(args), callable.GetLocation()); err != nil {
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

	pyleFuncCalled, err := vm.handleCall(len(args), callable.GetLocation())
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

func (vm *VM) handleCall(numArgs int, loc Loc) (bool, Error) {
	calleeIdx := vm.sp - 1 - numArgs
	if calleeIdx < 0 {
		return false, vm.runtimeError(loc, "Stack underflow during call setup")
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
				return false, vm.runtimeError(loc, "Method '%s' expected %d arguments, but got %d", nativeMethod.Name, nativeMethod.Arity, len(newArgs))
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
		return vm.handleCall(numArgs+1, loc)
	case *ClosureObj:
		if numArgs != c.Function.Arity {
			return false, vm.runtimeError(loc, "Function '%s' expected %d arguments, but got %d", c.Function.Name, c.Function.Arity, numArgs)
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
			return false, vm.runtimeError(loc, "Function '%s' expected %d arguments, but got %d", c.Name, c.Arity, numArgs)
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
				return false, vm.runtimeError(loc, "Function '%s' expected %d arguments, but got %d", c.Name, c.Arity, numArgs)
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
			case 3:
				if fn, ok := c.DirectCall.(NativeFunc3); ok {
					result, err = fn(vm, args[0], args[1], args[2])
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

		return false, vm.runtimeError(loc, "Cannot call uncallable native function '%s'", c.Name)

	default:
		return false, vm.runtimeError(loc, "Cannot call non-function type '%s'", callee.Type())
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
		return nil, vm.runtimeError(Loc{}, "Stack underflow, cannot pop value")
	}
	vm.sp--
	return vm.stack[vm.sp], nil
}

func (vm *VM) popCallFrame() (CallFrame, Error) {
	if len(vm.frames) == 0 {
		return CallFrame{}, vm.runtimeError(Loc{}, "No call frames to pop")
	}
	frame := vm.frames[len(vm.frames)-1]
	vm.frames = vm.frames[:len(vm.frames)-1]
	return *frame, nil
}

func (vm *VM) popEnv() Error {
	if len(vm.environments) == 0 {
		return vm.runtimeError(Loc{}, "No environments to pop")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Global variable '%s' already defined", name)
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
				return vm.runtimeErrorRes(currentTok.Loc, "Undefined variable '%s'", name)
			}
			vm.push(variable.Value)
		case OpSetGlobal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value
			if _, ok := vm.globals[name]; !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Undefined variable '%s'", name)
			}
			if vm.globals[name].IsConst {
				return vm.runtimeErrorRes(currentTok.Loc, "Cannot assign to constant variable '%s'", name)
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
				return vm.runtimeErrorRes(currentTok.Loc, "No active local scope active for OP_DEF_LOCAL")
			}
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_DEF_LOCAL")
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Local variable '%s' already defined in this scope", name)
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
				return vm.runtimeErrorRes(currentTok.Loc, "Undefined local variable '%s'", varScoped.Name)
			}

			if variable, ok := scope[varScoped.Name]; ok {
				vm.push(variable.Value)
			} else {
				return vm.runtimeErrorRes(currentTok.Loc, "Undefined local variable '%s'", varScoped.Name)
			}
		case OpDefConstLocal:
			nameIdx := *operand.(*int)
			name := vm.constants[nameIdx].(StringObj).Value

			if len(vm.environments) == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "No active local scope active for OP_DEF_CONST_LOCAL")
			}
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_DEF_CONST_LOCAL")
			}

			currentScope := vm.environments[len(vm.environments)-1]
			if _, ok := currentScope[name]; ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Local variable '%s' already defined in this scope", name)
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
				return vm.runtimeErrorRes(currentTok.Loc, "Undefined local variable '%s'", varScoped.Name)
			}

			if variable, ok := scope[varScoped.Name]; ok {
				if variable.IsConst {
					return vm.runtimeErrorRes(currentTok.Loc, "Cannot assign to const local variable '%s'", varScoped.Name)
				}
				variable.Value = valToAssign
			} else {
				return vm.runtimeErrorRes(currentTok.Loc, "Cannot assign to undefined local variable '%s'", varScoped.Name)
			}

		case OpUse:
			useInfo := operand.(*UseInfo)
			fullName := vm.constants[useInfo.ModuleIdx].(StringObj).Value

			parts := strings.Split(fullName, ".")
			baseName := parts[0]

			alias := baseName
			if useInfo.AliasIdx != -1 {
				alias = vm.constants[useInfo.AliasIdx].(StringObj).Value
			} else {
				alias = parts[len(parts)-1]
			}

			// Check if already in globals under this alias
			if _, loaded := vm.globals[alias]; loaded {
				continue
			}

			// Check registry
			if constructor, ok := vm.moduleRegistry[baseName]; ok {
				module := constructor(vm)
				var currentObj Object = module

				for i := 1; i < len(parts); i++ {
					attrName := parts[i]
					if getter, ok := currentObj.(AttributeGetter); ok {
						val, found, err := getter.GetAttribute(attrName)
						if err != nil {
							return ResErr[Object](err)
						}
						if !found {
							return vm.runtimeErrorRes(currentTok.Loc, "Module '%s' has no attribute '%s'", baseName, attrName)
						}
						currentObj = val
					} else {
						return vm.runtimeErrorRes(currentTok.Loc, "Cannot get attribute '%s' from non-object module part", attrName)
					}
				}

				vm.AddGlobal(alias, currentObj)
			} else {
				return vm.runtimeErrorRes(currentTok.Loc, "Module '%s' not found", baseName)
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_RETURN (no return value).")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_BUILD_KWARGS")
			}
		case OpCall:
			numArgs := *operand.(*int)
			if _, err := vm.handleCall(numArgs, currentTok.Loc); err != nil {
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
			return vm.runtimeErrorRes(currentTok.Loc, "type '%s' has no attribute '%s'", obj.Type(), name)
		case OpSetAttr:
			nameIdx := *operand.(*int)
			attrNameObj, ok := vm.constants[nameIdx].(StringObj)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Object attribute must be a string")
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
					return vm.runtimeErrorRes(currentTok.Loc, "%s", err.Error())
				}
			} else {
				return vm.runtimeErrorRes(currentTok.Loc, "Cannot set property on non-object type '%s'", obj.Type())
			}
		case OpTrue:
			vm.push(BooleanObj{Value: true})
		case OpFalse:
			vm.push(BooleanObj{Value: false})
		case OpNull:
			vm.push(NullObj{})
		case OpAnd:
			if vm.sp < 2 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_AND")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_OR")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_NEGATE")
			}

			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			if num, ok := value.(NumberObj); ok {
				num.Value = -num.Value
				vm.push(num)
			} else {
				return vm.runtimeErrorRes(currentTok.Loc, "Operand of unary OP_NEGATE must be a number")
			}
		case OpNot:
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_NOT")
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			vm.push(BooleanObj{Value: !value.IsTruthy()})

		case OpBuildRange:
			if vm.sp < 3 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_BUILD_RANGE")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Range start must be a number")
			}
			endObj, ok := end.(NumberObj)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Range end must be a number")
			}
			stepObj, ok := step.(NumberObj)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Range step must be a number")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_ITER_NEW")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_ITER_NEXT_OR_JUMP")
			}

			iterator, ok := vm.stack[vm.sp-1].(Iterator)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Object on stack is not an iterator")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_BUILD_LIST")
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
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_BUILD_MAP")
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
					return vm.runtimeErrorRes(currentTok.Loc, "%s", err.Error())
				}
			}
			vm.push(obj)
		case OpUnpack:
			expected := *operand.(*int)
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_UNPACK")
			}
			value, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			switch v := value.(type) {
			case *TupleObj:
				if len(v.Elements) != expected {
					return vm.runtimeErrorRes(currentTok.Loc, "unpack mismatch: expected %d values, got %d", expected, len(v.Elements))
				}
				for i := len(v.Elements) - 1; i >= 0; i-- {
					vm.push(v.Elements[i])
				}
			case *ArrayObj:
				if len(v.Elements) != expected {
					return vm.runtimeErrorRes(currentTok.Loc, "unpack mismatch: expected %d values, got %d", expected, len(v.Elements))
				}
				for i := len(v.Elements) - 1; i >= 0; i-- {
					vm.push(v.Elements[i])
				}
			case *ResultObject:
				if expected != 2 {
					return vm.runtimeErrorRes(currentTok.Loc, "unpack mismatch: expected %d values, got %d", expected, 2)
				}
				vm.push(v.Error)
				vm.push(v.Value)
			default:
				return vm.runtimeErrorRes(currentTok.Loc, "object of type '%s' is not unpackable", value.Type())
			}
		case OpUnwrap:
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_UNWRAP")
			}
			obj, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}

			resultObj, ok := obj.(*ResultObject)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Cannot unwrap non-Result type '%s'", obj.Type())
			}

			if resultObj.Error != nil {
				// If there's an error, panic
				return vm.runtimeErrorRes(currentTok.Loc, "%s", resultObj.Error.String())
			}
			vm.push(resultObj.Value)
		case OpUnwrapOrReturn:
			if vm.sp == 0 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_UNWRAP_OR_RETURN")
			}
			obj, err := vm.pop()
			if err != nil {
				return ResErr[Object](err)
			}
			resultObj, ok := obj.(*ResultObject)
			if !ok {
				return vm.runtimeErrorRes(currentTok.Loc, "Cannot use '?' on non-Result type '%s'", obj.Type())
			}
			if resultObj.Error != nil{
				// Early return the ResultObject itself 
				if len(vm.frames) == 0 {
					// Top level - behave like unwrap panic for now
					return vm.runtimeErrorRes(currentTok.Loc, "%s", resultObj.Error.String())
				}
				frame, perr := vm.popCallFrame()
				if perr != nil {
					return ResErr[Object](perr)
				}
				vm.ip = frame.ReturnIP
				for len(vm.environments) > frame.EnvDepth {
					if err := vm.popEnv(); err != nil {
						return ResErr[Object](err)
					}
				}
				vm.sp = frame.StackSlot
				vm.push(resultObj)
				continue
			}
			vm.push(resultObj.Value)
		case OpIndexGet:
			if vm.sp < 2 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_INDEX_GET")
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
					return vm.runtimeErrorRes(currentTok.Loc, "Unsupported index type '%s' for sequence", idx.Type())
				}

				switch seq := collection.(type) {
				case *ArrayObj:
					if index < 0 || index >= len(seq.Elements) {
						return vm.runtimeErrorRes(currentTok.Loc, "Array index out of bounds: %d", index)
					}
					vm.push(seq.Elements[index])
				case *TupleObj:
					if index < 0 || index >= len(seq.Elements) {
						return vm.runtimeErrorRes(currentTok.Loc, "Tuple index out of bounds: %d", index)
					}
					vm.push(seq.Elements[index])
				case StringObj:
					if index < 0 || index >= len(seq.Value) {
						return vm.runtimeErrorRes(currentTok.Loc, "String index out of bounds: %d", index)
					}
					vm.push(StringObj{Value: string(seq.Value[index])})
				}

			case *MapObj:
				val, found, err := coll.Get(idx)
				if err != nil {
					return vm.runtimeErrorRes(currentTok.Loc, "%s", err.Error())
				}
				if !found {
					vm.push(NullObj{})
				} else {
					vm.push(val)
				}
			default:
				return vm.runtimeErrorRes(currentTok.Loc, "Object of type '%s' does not support indexing", collection.Type())
			}
		case OpIndexSet:
			if vm.sp < 3 {
				return vm.runtimeErrorRes(currentTok.Loc, "Stack underflow for OP_INDEX_SET")
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
					return vm.runtimeErrorRes(currentTok.Loc, "Unsupported index type '%s' for array", idx.Type())
				}

				if index < 0 || index >= len(coll.Elements) {
					return vm.runtimeErrorRes(currentTok.Loc, "Array index out of bounds: %d", index)
				}
				coll.Elements[index] = value
			case *MapObj:
				if err := coll.Set(idx, value); err != nil {
					return vm.runtimeErrorRes(currentTok.Loc, "%s", err.Error())
				}
			default:
				return vm.runtimeErrorRes(currentTok.Loc, "Object of type '%s' does not support index assignment", collection.Type())
			}
		default:
			return vm.runtimeErrorRes(currentTok.Loc, "Unknown opcode %s", op)
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
			return 0, false, vm.runtimeError(idx.GetLocation(), "Sequence index must be an integer")
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

func (vm *VM) doBinaryOp(op OpCode, left, right Object, loc Loc) (Object, Error) {
	switch l := left.(type) {
	case NumberObj:
		r, ok := right.(NumberObj)
		if !ok {
			return nil, vm.runtimeError(loc, "unsupported operand type(s) for %s: 'number' and '%s'", op, right.Type())
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
				return nil, vm.runtimeError(loc, "Division by zero")
			}
			result = l.Value / r.Value
			isInt = false
		case OpModulo:
			if r.Value == 0 {
				return nil, vm.runtimeError(loc, "Modulo by zero")
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
				return nil, vm.runtimeError(loc, "unsupported operand type(s) for +: 'string' and '%s'", right.Type())
			}
		} else {
			return nil, vm.runtimeError(loc, "unsupported operand type(s) for %s: 'string'", op)
		}

	default:
		return nil, vm.runtimeError(loc, "unsupported operand type(s) for %s: '%s' and '%s'", op, left.Type(), right.Type())
	}
}

func (vm *VM) binaryOp(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return vm.runtimeError(currentTok.Loc, "Stack underflow for %s", op)
	}
	right, err := vm.pop()
	if err != nil {
		return err
	}
	left, err := vm.pop()
	if err != nil {
		return err
	}

	result, err := vm.doBinaryOp(op, left, right, currentTok.Loc)
	if err != nil {
		return err
	}

	vm.push(result)

	return nil
}

func (vm *VM) binaryOpCompare(op OpCode, currentTok *Token) Error {
	if vm.sp < 2 {
		return vm.runtimeError(currentTok.Loc, "Stack underflow for %s", op)
	}
	right, err := vm.pop()
	if err != nil {
		return err
	}
	left, err := vm.pop()
	if err != nil {
		return err
	}


	// *ErrorObj can be nil so we check that and make them proper null objects
	if errPtr, ok := left.(*ErrorObj); ok && errPtr == nil  {
		left = CreateNull()
	}
	if errPtr, ok := right.(*ErrorObj); ok && errPtr == nil  {
		right = CreateNull()
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
			return vm.runtimeError(currentTok.Loc, "type '%s' does not support ordering comparisons", left.Type())
		}
		vm.push(BooleanObj{Value: result})
		return nil
	}
	

	cmpResult, cmpErr := comparable.Compare(right)
	if cmpErr != nil {
		return vm.runtimeError(currentTok.Loc, "%s", cmpErr.Error())
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
			return vm.runtimeError(currentTok.Loc, "Undefined local variable '%s'", opand.Name)
		}

		if v, ok := scope[opand.Name]; ok {
			variable = v
		} else {
			return vm.runtimeError(currentTok.Loc, "Undefined local variable '%s'", opand.Name)
		}
	case *int:
		nameIdx := *opand
		name := vm.constants[nameIdx].(StringObj).Value
		if v, ok := vm.globals[name]; ok {
			variable = v
		} else {
			return vm.runtimeError(currentTok.Loc, "Undefined variable '%s'", name)
		}
	default:
		return vm.runtimeError(currentTok.Loc, "internal VM error: unsupported operand type for inplace op")
	}

	if variable.IsConst {
		return vm.runtimeError(currentTok.Loc, "Cannot assign to constant variable '%s'", variable.Name)
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
		return vm.runtimeError(currentTok.Loc, "internal VM error: unhandled inplace operator %s", op)
	}

	result, err := vm.doBinaryOp(binaryOpCode, left, right, currentTok.Loc)
	if err != nil {
		return err
	}

	variable.Value = result
	return nil
}

func (vm *VM) runtimeError(loc Loc, format string, args ...any) Error {
	return NewRuntimeError(fmt.Sprintf(format, args...), loc)
}

func (vm *VM) runtimeErrorRes(loc Loc, format string, args ...any) Result[Object] {
	return ResErr[Object](vm.runtimeError(loc, format, args...))
}
