package pyle

import (
	"fmt"
	"os"
	"strconv"
	"strings"
	"time"
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

func nativeScan(prompt string) (string, error) {
	fmt.Print(prompt)
	var input string
	_, err := fmt.Scanln(&input)
	if err != nil && err.Error() != "unexpected newline" {
		return "", err
	}
	return input, nil
}

func nativeType(obj Object) (string, error) {
	return obj.Type(), nil
}

func nativeTuple(objs ...Object) (*TupleObj, error) {
	elems := make([]Object, len(objs))
	copy(elems, objs)
	return &TupleObj{Elements: elems}, nil
}

func nativePerfCounter() (float64, error) {
	return float64(time.Now().UnixNano()), nil
}

func nativeInt(obj Object) (NumberObj, error) {
	switch v := obj.(type) {
	case NumberObj:
		if v.IsInt {
			return v, nil
		}
		return NumberObj{Value: float64(int(v.Value)), IsInt: true}, nil
	case StringObj:
		i, err := strconv.Atoi(v.Value)
		if err != nil {
			return NumberObj{}, fmt.Errorf("could not convert string '%s' to int", v.Value)
		}
		return NumberObj{Value: float64(i), IsInt: true}, nil
	case BooleanObj:
		if v.Value {
			return NumberObj{Value: 1, IsInt: true}, nil
		}
		return NumberObj{Value: 0, IsInt: true}, nil
	default:
		return NumberObj{}, fmt.Errorf("cannot convert type '%s' to int", obj.Type())
	}
}

func nativeFloat(obj Object) (NumberObj, error) {
	switch v := obj.(type) {
	case NumberObj:
		return NumberObj{Value: v.Value, IsInt: false}, nil
	case StringObj:
		f, err := strconv.ParseFloat(v.Value, 64)
		if err != nil {
			return NumberObj{}, fmt.Errorf("could not convert string '%s' to float", v.Value)
		}
		return NumberObj{Value: f, IsInt: false}, nil
	case BooleanObj:
		if v.Value {
			return NumberObj{Value: 1.0, IsInt: false}, nil
		}
		return NumberObj{Value: 0.0, IsInt: false}, nil
	default:
		return NumberObj{}, fmt.Errorf("cannot convert type '%s' to float", obj.Type())
	}
}

func nativeString(obj Object) (StringObj, error) {
	return StringObj{Value: obj.String()}, nil
}

func nativeBool(obj Object) (BooleanObj, error) {
	return BooleanObj{Value: obj.IsTruthy()}, nil
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

func nativeHash(obj Object) (uint32, error) {
	if hashable, ok := obj.(Hashable); ok {
		return hashable.Hash(), nil
	} 
	return 0, fmt.Errorf("object of type '%s' is not hashable", obj.Type())
}

func nativeExit() (Object, error) {
	os.Exit(0)
	return CreateNull(), nil
}

func nativeAsciiCode(obj Object) (int, error) {
	switch v := obj.(type) {
	case StringObj:
		if len(v.Value) == 0 {
			return 0, fmt.Errorf("string is empty")
		}
		return int(v.Value[0]), nil
	default:
		return 0, fmt.Errorf("cannot convert type '%s' to ascii code", obj.Type())
	}
}

var Builtins = map[string]any{
	"echo":        builtinEcho,
	"scan":        nativeScan,
	"perfCounter": nativePerfCounter,
	"tuple":       nativeTuple,
	"type":        nativeType,
	"int":         nativeInt,
	"float":       nativeFloat,
	"string":      nativeString,
	"bool":        nativeBool,
	"expect":      nativeExpect,
	"hash":        nativeHash,
	"exit":        nativeExit,
	"asciiCode":   nativeAsciiCode,
}
