package pyle

import (
	"fmt"
	"reflect"
)

func CreateNativeFunction(name string, fn any, doc *DocstringObj) (*NativeFuncObj, error) {
	fnValue := reflect.ValueOf(fn)
	fnType := fnValue.Type()

	if fnType.Kind() != reflect.Func {
		return nil, fmt.Errorf("Not a function: %v", fn)
	}

	// Check if the first argument is a *VM
	wantsVM := false
	argOffset := 0
	if fnType.NumIn() > 0 && fnType.In(0) == reflect.TypeOf((*VM)(nil)) {
		wantsVM = true
		argOffset = 1
	}

	// Pre-compute metadata
	metadata := &FunctionMetadata{
		Name:         name,
		Args:         make([]ArgSpec, fnType.NumIn()-argOffset),
		Returns:      make([]reflect.Type, fnType.NumOut()),
		IsVariadic:   fnType.IsVariadic(),
		FnValue:      fnValue,
		WantsVM:      wantsVM,
	}

	// Pre-process argument types
	for i := 0; i < len(metadata.Args); i++ {
		metadata.Args[i] = ArgSpec{
			Type: fnType.In(i + argOffset),
			Name: fmt.Sprintf("arg%d", i),
		}
	}

	// Pre-process return types
	for i := 0; i < fnType.NumOut(); i++ {
		metadata.Returns[i] = fnType.Out(i)
	}

	if fnType.NumOut() > 1 && fnType.Out(1).Implements(reflect.TypeOf((*error)(nil)).Elem()) {
		metadata.ReturnsError = true
	}

	// Pre-create the optimized call function
	callFunc := createCallFunc(metadata)

	return &NativeFuncObj{
		Name:     name,
		Arity:    len(metadata.Args),
		Doc:      doc,
		Metadata: metadata,
		Call:     callFunc,
	}, nil
}

func createCallFunc(metadata *FunctionMetadata) func(*VM, []Object, map[string]Object) (Object, Error) {
    argConverters := make([]func(Object) (reflect.Value, error), len(metadata.Args))
    for i, argSpec := range metadata.Args {
        argConverters[i] = createTypeConverter(argSpec.Type)
    }

    return func(vm *VM, args []Object, kwargs map[string]Object) (Object, Error) {
        if len(kwargs) > 0 {
			return nil, NewRuntimeError("keyword arguments not supported for native functions", nil)
		}

		// Arity check
		numArgs := len(args)
		expectedArity := len(metadata.Args)
		if metadata.IsVariadic {
			// For variadic functions, we need at least (Arity - 1) arguments.
			// The last arg in metadata.Args is the variadic slice itself.
			if numArgs < expectedArity-1 {
				return nil, NewRuntimeError(
					fmt.Sprintf("function '%s' expected at least %d arguments, but got %d",
						metadata.Name, expectedArity-1, numArgs), nil)
			}
		} else {
			// Exact match for non-variadic functions.
			if numArgs != expectedArity {
				return nil, NewRuntimeError(
					fmt.Sprintf("function '%s' expected %d arguments, but got %d",
						metadata.Name, expectedArity, numArgs), nil)
			}
		}

        in := make([]reflect.Value, 0, len(args)+1) // +1 for VM
        if metadata.WantsVM {
            in = append(in, reflect.ValueOf(vm))
        }
        
        // Convert regular arguments
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

        // Convert variadic arguments
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
	default:
		// Fallback for less common types
		return func(obj Object) (reflect.Value, error) {
			return convertVMObjectToGoValue(obj, targetType)
		}
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
        
    case reflect.Int, reflect.Int64, reflect.Int32, reflect.Int16, reflect.Int8:
        if numObj, ok := obj.(NumberObj); ok && numObj.IsInt {
            return reflect.ValueOf(int(numObj.Value)).Convert(targetType), nil
        }
        return reflect.Value{}, fmt.Errorf("expected int, got %s", obj.Type())
        
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

    switch value.Kind() {
    case reflect.String:
        return StringObj{Value: value.String()}, nil
    case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
        return CreateInt(int64(value.Int())), nil
    case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
        return CreateInt(int64(value.Uint())), nil
    case reflect.Float32, reflect.Float64:
        return NumberObj{Value: value.Float(), IsInt: false}, nil
    case reflect.Bool:
        return BooleanObj{Value: value.Bool()}, nil
    case reflect.Slice:
        elements := make([]Object, value.Len())
        for i := 0; i < value.Len(); i++ {
			res, err := convertGoValueToVMObject(value.Index(i))
			if err != nil {
				panic(err)
			}
            elements[i] = res
        }
        return &ArrayObj{Elements: elements}, nil
    default:
		return nil, fmt.Errorf("Unsupported Go type: %v", value.Kind())
    }
}
