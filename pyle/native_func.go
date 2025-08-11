package pyle

import (
	"fmt"
	"reflect"
)

// --- Fast Path Function Types ---
type NativeFunc0 func(vm *VM) (Object, Error)
type NativeFunc1 func(vm *VM, arg Object) (Object, Error)
type NativeFunc2 func(vm *VM, arg1, arg2 Object) (Object, Error)

func CreateNativeFunction(name string, fn any, doc *DocstringObj) (*NativeFuncObj, error) {
	if directCall, arity, ok := createDirectCall(fn); ok {
		return &NativeFuncObj{
			Name:       name,
			Arity:      arity,
			Doc:        doc,
			DirectCall: directCall,
		}, nil
	}

	metadata, err := analyzeFunction(fn)
	if err != nil {
		return nil, fmt.Errorf("could not create native function '%s': %w", name, err)
	}
	metadata.Name = name

	return &NativeFuncObj{
		Name:        name,
		Arity:       len(metadata.Args),
		Doc:         doc,
		ReflectCall: createReflectCall(metadata),
	}, nil
}

func createDirectCall(fn any) (any, int, bool) {
	switch f := fn.(type) {
	case func() (NumberObj, error):
		return NativeFunc0(func(vm *VM) (Object, Error) {
			res, err := f()
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 0, true

	case func() (int64, error):
		return NativeFunc0(func(vm *VM) (Object, Error) {
			res, err := f()
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return CreateInt(res), nil
		}), 0, true
	case func() int64:
		return NativeFunc0(func(vm *VM) (Object, Error) {
			res := f()
			return CreateInt(res), nil
		}), 0, true

	case func() (float64, error):
		return NativeFunc0(func(vm *VM) (Object, Error) {
			res, err := f()
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return NumberObj{Value: res, IsInt: false}, nil
		}), 0, true

	case func(float64):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			num, ok := arg.(NumberObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a number argument, got %s", arg.Type()), nil)
			}
			f(num.Value)
			return NullObj{}, nil
		}), 1, true

	case func(Object) (int, error):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			res, err := f(arg)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return CreateInt(int64(res)), nil
		}), 1, true

	case func(StringObj) (string, error):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			receiver, ok := arg.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg.Type()), nil)
			}
			res, err := f(receiver)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return StringObj{Value: res}, nil
		}), 1, true

	case func(StringObj, string) (string, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			sep, ok := arg2.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string separator, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, sep.Value)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return StringObj{Value: res}, nil
		}), 2, true

	case func(StringObj, string) ([]string, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			sep, ok := arg2.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string separator, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, sep.Value)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			elements := make([]Object, len(res))
			for i, s := range res {
				elements[i] = StringObj{Value: s}
			}
			return &ArrayObj{Elements: elements}, nil
		}), 2, true

	case func(StringObj, StringObj) (BooleanObj, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			sep, ok := arg2.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string argument, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, sep)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 2, true

	case func(*ArrayObj, Object) (Object, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(*ArrayObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected an array receiver, got %s", arg1.Type()), nil)
			}
			res, err := f(receiver, arg2)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 2, true

	case func(StringObj) (int, error):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			receiver, ok := arg.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg.Type()), nil)
			}
			res, err := f(receiver)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return CreateInt(int64(res)), nil
		}), 1, true

	case func(StringObj, StringObj) (StringObj, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			arg, ok := arg2.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string argument, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, arg)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 2, true

	case func(StringObj, NumberObj) (StringObj, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			arg, ok := arg2.(NumberObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a number argument, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, arg)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 2, true

	case func(StringObj, NumberObj) (NumberObj, error):
		return NativeFunc2(func(vm *VM, arg1, arg2 Object) (Object, Error) {
			receiver, ok := arg1.(StringObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a string receiver, got %s", arg1.Type()), nil)
			}
			arg, ok := arg2.(NumberObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a number argument, got %s", arg2.Type()), nil)
			}
			res, err := f(receiver, arg)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 2, true

	case func(*ArrayObj) (Object, error):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			receiver, ok := arg.(*ArrayObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected an array receiver, got %s", arg.Type()), nil)
			}
			res, err := f(receiver)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}), 1, true

	case func(*MapObj) (int, error):
		return NativeFunc1(func(vm *VM, arg Object) (Object, Error) {
			receiver, ok := arg.(*MapObj)
			if !ok {
				return nil, NewRuntimeError(fmt.Sprintf("expected a map receiver, got %s", arg.Type()), nil)
			}
			res, err := f(receiver)
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return CreateInt(int64(res)), nil
		}), 1, true

	default:
		return nil, 0, false
	}
}

func analyzeFunction(fn any) (*FunctionMetadata, error) {
	fnValue := reflect.ValueOf(fn)
	fnType := fnValue.Type()

	if fnType.Kind() != reflect.Func {
		return nil, fmt.Errorf("value is not a function")
	}

	wantsVM := false
	argOffset := 0
	if fnType.NumIn() > 0 && fnType.In(0) == reflect.TypeOf((*VM)(nil)) {
		wantsVM = true
		argOffset = 1
	}

	metadata := &FunctionMetadata{
		Args:       make([]ArgSpec, fnType.NumIn()-argOffset),
		Returns:    make([]reflect.Type, fnType.NumOut()),
		IsVariadic: fnType.IsVariadic(),
		FnValue:    fnValue,
		WantsVM:    wantsVM,
	}

	for i := 0; i < len(metadata.Args); i++ {
		metadata.Args[i] = ArgSpec{Type: fnType.In(i + argOffset)}
	}
	for i := 0; i < fnType.NumOut(); i++ {
		metadata.Returns[i] = fnType.Out(i)
	}
	if fnType.NumOut() > 1 && fnType.Out(1).Implements(reflect.TypeOf((*error)(nil)).Elem()) {
		metadata.ReturnsError = true
	}
	return metadata, nil
}

func createReflectCall(metadata *FunctionMetadata) func(*VM, []Object) (Object, Error) {
	argConverters := make([]func(Object) (reflect.Value, error), len(metadata.Args))
	for i, argSpec := range metadata.Args {
		argConverters[i] = createTypeConverter(argSpec.Type)
	}

	return func(vm *VM, args []Object) (Object, Error) {
		numArgs := len(args)
		expectedArity := len(metadata.Args)
		if metadata.IsVariadic {
			if numArgs < expectedArity-1 {
				return nil, NewRuntimeError(
					fmt.Sprintf("function '%s' expected at least %d arguments, but got %d",
						metadata.Name, expectedArity-1, numArgs), nil)
			}
		} else {
			if numArgs != expectedArity {
				return nil, NewRuntimeError(
					fmt.Sprintf("function '%s' expected %d arguments, but got %d",
						metadata.Name, expectedArity, numArgs), nil)
			}
		}

		in := make([]reflect.Value, 0, len(args)+1)
		if metadata.WantsVM {
			in = append(in, reflect.ValueOf(vm))
		}

		numRegularArgs := len(metadata.Args)
		if metadata.IsVariadic {
			numRegularArgs--
		}
		for i := 0; i < numRegularArgs; i++ {
			converted, err := argConverters[i](args[i])
			if err != nil {
				return nil, NewRuntimeError(fmt.Sprintf("argument %d: %v", i+1, err), nil)
			}
			in = append(in, converted)
		}

		if metadata.IsVariadic {
			variadicType := metadata.Args[len(metadata.Args)-1].Type.Elem()
			for i := numRegularArgs; i < len(args); i++ {
				converted, err := createTypeConverter(variadicType)(args[i])
				if err != nil {
					return nil, NewRuntimeError(fmt.Sprintf("variadic argument %d: %v", i+1, err), nil)
				}
				in = append(in, converted)
			}
		}

		results := metadata.FnValue.Call(in)

		if metadata.ReturnsError {
			if len(results) > 1 && !results[1].IsNil() {
				if err, ok := results[1].Interface().(error); ok {
					return nil, NewRuntimeError(err.Error(), nil)
				}
			}
			res, err := convertGoValueToVMObject(results[0])
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}

			return res, nil
		}

		if len(results) > 0 {
			res, err := convertGoValueToVMObject(results[0])
			if err != nil {
				return nil, NewRuntimeError(err.Error(), nil)
			}
			return res, nil
		}

		return NullObj{}, nil
	}
}

// Pre-compiled type converters for common types
func createTypeConverter(targetType reflect.Type) func(Object) (reflect.Value, error) {
	if targetType == reflect.TypeOf((*Object)(nil)).Elem() {
		return func(obj Object) (reflect.Value, error) {
			return reflect.ValueOf(obj), nil // Pass the Object directly
		}
	}

	// Fast paths for common types
	switch targetType.Kind() {
	case reflect.String:
		return func(obj Object) (reflect.Value, error) {
			if strObj, ok := obj.(StringObj); ok {
				return reflect.ValueOf(strObj.Value), nil
			}
			return reflect.Value{}, fmt.Errorf("expected string, got %s", obj.Type())
		}
	case reflect.Int:
		return func(obj Object) (reflect.Value, error) {
			if numObj, ok := obj.(NumberObj); ok && numObj.IsInt {
				return reflect.ValueOf(int(numObj.Value)), nil
			}
			return reflect.Value{}, fmt.Errorf("expected integer, got %s", obj.Type())
		}
	case reflect.Int64:
		return func(obj Object) (reflect.Value, error) {
			if numObj, ok := obj.(NumberObj); ok && numObj.IsInt {
				return reflect.ValueOf(int64(numObj.Value)), nil
			}
			return reflect.Value{}, fmt.Errorf("expected integer, got %s", obj.Type())
		}
	case reflect.Float64:
		return func(obj Object) (reflect.Value, error) {
			if numObj, ok := obj.(NumberObj); ok {
				return reflect.ValueOf(numObj.Value), nil
			}
			return reflect.Value{}, fmt.Errorf("expected number, got %s", obj.Type())
		}
	case reflect.Bool:
		return func(obj Object) (reflect.Value, error) {
			if boolObj, ok := obj.(BooleanObj); ok {
				return reflect.ValueOf(boolObj.Value), nil
			}
			return reflect.Value{}, fmt.Errorf("expected boolean, got %s", obj.Type())
		}
	case reflect.Ptr:
		// Fast paths for common pointer types
		switch targetType {
		case reflect.TypeOf((*ArrayObj)(nil)):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(*ArrayObj); ok {
					return reflect.ValueOf(val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected array, got %s", obj.Type())
			}
		case reflect.TypeOf((*MapObj)(nil)):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(*MapObj); ok {
					return reflect.ValueOf(val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected map, got %s", obj.Type())
			}
		case reflect.TypeOf((*StringObj)(nil)):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(StringObj); ok {
					return reflect.ValueOf(&val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected string, got %s", obj.Type())
			}
		}
	case reflect.Struct:
		// Fast paths for common struct types
		switch targetType {
		case reflect.TypeOf(StringObj{}):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(StringObj); ok {
					return reflect.ValueOf(val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected string, got %s", obj.Type())
			}
		case reflect.TypeOf(NumberObj{}):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(NumberObj); ok {
					return reflect.ValueOf(val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected number, got %s", obj.Type())
			}
		case reflect.TypeOf(BooleanObj{}):
			return func(obj Object) (reflect.Value, error) {
				if val, ok := obj.(BooleanObj); ok {
					return reflect.ValueOf(val), nil
				}
				return reflect.Value{}, fmt.Errorf("expected boolean, got %s", obj.Type())
			}
		}
	default:
		// Fallback for less common types
		return func(obj Object) (reflect.Value, error) {
			return convertVMObjectToGoValue(obj, targetType)
		}
	}

	// Fallback for less common types
	return func(obj Object) (reflect.Value, error) {
		return convertVMObjectToGoValue(obj, targetType)
	}
}

// Convert VM Object to Go value using reflection
func convertVMObjectToGoValue(obj Object, targetType reflect.Type) (reflect.Value, error) {
	// Hybrid approach: Fast path for common types, slower reflection for the rest.
	switch targetType.Kind() {
	case reflect.Ptr:
		// Fast path for common pointer types
		switch targetType {
		case reflect.TypeOf((*ArrayObj)(nil)):
			if val, ok := obj.(*ArrayObj); ok {
				return reflect.ValueOf(val), nil
			}
		case reflect.TypeOf((*MapObj)(nil)):
			if val, ok := obj.(*MapObj); ok {
				return reflect.ValueOf(val), nil
			}
		case reflect.TypeOf((*StringObj)(nil)):
			if val, ok := obj.(StringObj); ok {
				return reflect.ValueOf(&val), nil
			}
		default:
			// Generic fallback for other pointer types
			val := reflect.ValueOf(obj)
			if val.Type().AssignableTo(targetType) {
				return val, nil
			}
		}
		return reflect.Value{}, fmt.Errorf("cannot convert %s to pointer type %s", obj.Type(), targetType)

	case reflect.Struct:
		// Fast path for common struct types
		switch targetType {
		case reflect.TypeOf(StringObj{}):
			if val, ok := obj.(StringObj); ok {
				return reflect.ValueOf(val), nil
			}
		case reflect.TypeOf(NumberObj{}):
			if val, ok := obj.(NumberObj); ok {
				return reflect.ValueOf(val), nil
			}
		case reflect.TypeOf(BooleanObj{}):
			if val, ok := obj.(BooleanObj); ok {
				return reflect.ValueOf(val), nil
			}
		default:
			// Generic fallback for other struct types
			val := reflect.ValueOf(obj)
			if val.Type().AssignableTo(targetType) {
				return val, nil
			}
		}
		return reflect.Value{}, fmt.Errorf("cannot convert %s to struct type %s", obj.Type(), targetType)

	case reflect.String:
		if strObj, ok := obj.(StringObj); ok {
			return reflect.ValueOf(strObj.Value), nil
		}
		return reflect.Value{}, fmt.Errorf("expected string, got %s", obj.Type())

	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		if numObj, ok := obj.(NumberObj); ok && numObj.IsInt {
			return reflect.ValueOf(int64(numObj.Value)).Convert(targetType), nil
		}
		return reflect.Value{}, fmt.Errorf("expected int, got %s", obj.Type())

	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
		if numObj, ok := obj.(NumberObj); ok && numObj.IsInt {
			return reflect.ValueOf(uint64(numObj.Value)).Convert(targetType), nil
		}
		return reflect.Value{}, fmt.Errorf("expected uint, got %s", obj.Type())

	case reflect.Float64, reflect.Float32:
		if numObj, ok := obj.(NumberObj); ok {
			return reflect.ValueOf(numObj.Value).Convert(targetType), nil
		}
		return reflect.Value{}, fmt.Errorf("expected float, got %s", obj.Type())

	case reflect.Bool:
		if boolObj, ok := obj.(BooleanObj); ok {
			return reflect.ValueOf(boolObj.Value), nil
		}
		return reflect.Value{}, fmt.Errorf("expected boolean, got %s", obj.Type())
	case reflect.Slice:
		if arrayObj, ok := obj.(*ArrayObj); ok {
			sliceType := targetType.Elem()
			goSlice := reflect.MakeSlice(targetType, len(arrayObj.Elements), len(arrayObj.Elements))
			for i, elem := range arrayObj.Elements {
				val, err := convertVMObjectToGoValue(elem, sliceType)
				if err != nil {
					return reflect.Value{}, fmt.Errorf("error converting slice element %d: %v", i, err)
				}
				goSlice.Index(i).Set(val)
			}
			return goSlice, nil
		}
		return reflect.Value{}, fmt.Errorf("expected array, got %s", obj.Type())
	default:
		return reflect.Value{}, fmt.Errorf("unsupported Go type: %v", targetType.Kind())
	}
}

// Convert Go value to VM Object
func convertGoValueToVMObject(value reflect.Value) (Object, error) {
	// check if the value is already a Pyle Object.
	if value.IsValid() && value.CanInterface() {
		if obj, ok := value.Interface().(Object); ok {
			return obj, nil
		}
	}

	if !value.IsValid() {
		return NullObj{}, nil
	}

	switch value.Kind() {
	case reflect.Ptr, reflect.Interface, reflect.Map:
		if value.IsNil() {
			return NullObj{}, nil
		}
	}

	switch value.Kind() {
	case reflect.String:
		return StringObj{Value: value.String()}, nil
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		return CreateInt(value.Int()), nil
	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
		return CreateInt(int64(value.Uint())), nil
	case reflect.Float32, reflect.Float64:
		return NumberObj{Value: value.Float(), IsInt: false}, nil
	case reflect.Bool:
		return BooleanObj{Value: value.Bool()}, nil
	case reflect.Slice:
		// Fast path for string slices
		if value.Type().Elem().Kind() == reflect.String {
			elements := make([]Object, value.Len())
			for i := 0; i < value.Len(); i++ {
				elements[i] = StringObj{Value: value.Index(i).String()}
			}
			return &ArrayObj{Elements: elements}, nil
		}

		// General slice handling
		elements := make([]Object, value.Len())
		for i := 0; i < value.Len(); i++ {
			res, err := convertGoValueToVMObject(value.Index(i))
			if err != nil {
				return nil, fmt.Errorf("error converting slice element %d: %v", i, err)
			}
			elements[i] = res
		}
		return &ArrayObj{Elements: elements}, nil
	case reflect.Array:
		elements := make([]Object, value.Len())
		for i := 0; i < value.Len(); i++ {
			res, err := convertGoValueToVMObject(value.Index(i))
			if err != nil {
				return nil, fmt.Errorf("error converting array element %d: %v", i, err)
			}
			elements[i] = res
		}
		return &ArrayObj{Elements: elements}, nil
	default:
		return nil, fmt.Errorf("Unsupported Go type: %v", value.Kind())
	}
}
