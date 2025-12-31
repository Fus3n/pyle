package pyle

import (
	"fmt"
	"os"
	"strconv"
	"strings"
)

func builtinEcho(vm *VM, args ...Object) (Object, error) {
	if len(args) == 0 {
		fmt.Fprintln(vm.Stdout)
		return NullObj{}, nil
	}

	strArgs := make([]string, len(args))
	for i, arg := range args {
		strArgs[i] = arg.String()
	}
	fmt.Fprintln(vm.Stdout, strings.Join(strArgs, " "))
	return NullObj{}, nil
}

func nativeScan(prompt string) *ResultObject {
	fmt.Print(prompt)
	var input string
	_, err := fmt.Scanln(&input)
	if err != nil && err.Error() != "unexpected newline" {
		return ReturnError(err.Error())
	}
	return ReturnOkString(input)
}

func nativeType(obj Object) string {
	return obj.Type()
}

func nativeTuple(objs ...Object) *TupleObj {
	elems := make([]Object, len(objs))
	copy(elems, objs)
	return &TupleObj{Elements: elems}
}

func nativeInt(obj Object) *ResultObject {
	switch v := obj.(type) {
	case NumberObj:
		if v.IsInt {
			return ReturnOk(v)
		}
		return ReturnOkInt(float64(int(v.Value)))
	case StringObj:
		i, err := strconv.Atoi(v.Value)
		if err != nil {
			return ReturnErrorf("could not convert string '%s' to int", v.Value)
		}
		return ReturnOkInt(float64(i))
	case BooleanObj:
		if v.Value {
			return ReturnOkInt(1)
		}
		return ReturnOkInt(0)
	default:
		return ReturnErrorf("cannot convert type '%s' to int", obj.Type())
	}
}

func nativeFloat(obj Object) *ResultObject {
	switch v := obj.(type) {
	case NumberObj:
		return ReturnOk(NumberObj{Value: v.Value, IsInt: false})
	case StringObj:
		f, err := strconv.ParseFloat(v.Value, 64)
		if err != nil {
			return ReturnErrorf("could not convert string '%s' to float", v.Value)
		}
		return ReturnOk(NumberObj{Value: f, IsInt: false})
	case BooleanObj:
		if v.Value {
			return ReturnOk(NumberObj{Value: 1.0, IsInt: false})
		}
		return ReturnOk(NumberObj{Value: 0.0, IsInt: false})
	default:
		return ReturnErrorf("cannot convert type '%s' to float", obj.Type())
	}
}

func nativeString(obj Object) StringObj {
	return StringObj{Value: obj.String()}
}

func nativeBool(obj Object) BooleanObj {
	return BooleanObj{Value: obj.IsTruthy()}
}

func nativeArray(obj Object) *ResultObject  {
	switch v := obj.(type) {
	case *ArrayObj:
		return ReturnOk(v)
	case *TupleObj:
		return ReturnOk(&ArrayObj{Elements: v.Elements})
	case StringObj:
		elements := make([]Object, len(v.Value))
		for i, char := range v.Value {
			elements[i] = StringObj{Value: string(char)}
		}
		return ReturnOk(&ArrayObj{Elements: elements})
	case *MapObj:
		elements := make([]Object, 0, len(v.Pairs))
		for _, bucket := range v.Pairs {
			for _, pair := range bucket {
				elements = append(elements, pair.Value)
			}
		}
		return ReturnOk(&ArrayObj{Elements: elements})
	case *RangeObj:
		elements := []Object{}
		for i := v.Start; (v.Step > 0 && i < v.End) || (v.Step < 0 && i > v.End); i += v.Step {
			elements = append(elements, NumberObj{Value: float64(i), IsInt: true})
		}
		return ReturnOk(&ArrayObj{Elements: elements})
	default:
		return ReturnErrorf("cannot convert type '%s' to array", obj.Type())
	}
}

func nativeExpect(condition Object, message Object) (Object, error) {
	if !condition.IsTruthy() {
		msg := "Assertion failed"
		if message != nil {
			msg = message.String()
		}
		// TODO: also print file name and location
		return nil, fmt.Errorf("%s", msg)
	}
	return NullObj{}, nil
}

func nativeHash(obj Object) Object {
	if hashable, ok := obj.(Hashable); ok {
		return ReturnOkInt(float64(hashable.Hash()))
	}
	return ReturnErrorf("object of type '%s' is not hashable", obj.Type())
}

func nativeExit() Object{
	os.Exit(0)
	return CreateNull()
}

func nativeAsciiCode(obj Object) *ResultObject {
	switch v := obj.(type) {
	case StringObj:
		if len(v.Value) == 0 {
			return ReturnError("string is empty")
		}
		if len(v.Value) > 1 {
			return ReturnError("string must be exactly one character")
		}
		return ReturnOkInt(float64(v.Value[0]))
	default:
		return ReturnErrorf("cannot convert type '%s' to ascii code", obj.Type())
	}
}

func nativeError(message Object) Object {
	return CreateError(message.String())
}

func nativePanic(message Object) (Object, error) {
	switch v := message.(type) {
	case StringObj:
		return nil, NewRuntimeError(v.Value, Loc{})
	case *ErrorObj:
		return nil, NewRuntimeError(v.Message, Loc{})
	default:
		return nil, NewRuntimeError(message.String(), Loc{})
	}
}

func nativeOk(value Object) Object {
	return ReturnOk(value)
}

func nativeErr(message Object) *ResultObject {
	switch v := message.(type) {
	case StringObj:
		return ReturnError(v.Value)
	case *ErrorObj:
		return ReturnError(v.Message)
	default:
		return ReturnError(message.String())
	}
}

var BuiltinFunctions = map[string]any{
	"echo":      builtinEcho,
	"scan":      nativeScan,
	"tuple":     nativeTuple,
	"type":      nativeType,
	"int":       nativeInt,
	"float":     nativeFloat,
	"string":    nativeString,
	"bool":      nativeBool,
	"array":     nativeArray,
	"expect":    nativeExpect,
	"hash":      nativeHash,
	"exit":      nativeExit,
	"asciiCode": nativeAsciiCode,
	"error":     nativeError,
	"panic":     nativePanic,
	// Capital letter to avoid conflict
	"Ok":        nativeOk,
	"Err":       nativeErr,
}


