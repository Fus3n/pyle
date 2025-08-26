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

func nativeScan(prompt string) (string, error) {
	fmt.Print(prompt)
	var input string
	_, err := fmt.Scanln(&input)
	if err != nil && err.Error() != "unexpected newline" {
		return "", err
	}
	return input, nil
}

func nativeType(obj Object) string {
	return obj.Type()
}

func nativeTuple(objs ...Object) *TupleObj {
	elems := make([]Object, len(objs))
	copy(elems, objs)
	return &TupleObj{Elements: elems}
}

func nativeInt(obj Object) Object {
	switch v := obj.(type) {
	case NumberObj:
		if v.IsInt {
			return ReturnValue(v)
		}
		return ReturnValue(NumberObj{Value: float64(int(v.Value)), IsInt: true})
	case StringObj:
		i, err := strconv.Atoi(v.Value)
		if err != nil {
			return ReturnErrorf("could not convert string '%s' to int", v.Value)
		}
		return ReturnValue(NumberObj{Value: float64(i), IsInt: true})
	case BooleanObj:
		if v.Value {
			return ReturnValue(NumberObj{Value: 1, IsInt: true})
		}
		return ReturnValue(NumberObj{Value: 0, IsInt: true})
	default:
		return ReturnErrorf("cannot convert type '%s' to int", obj.Type())
	}
}

func nativeFloat(obj Object) Object {
	switch v := obj.(type) {
	case NumberObj:
		return ReturnValue(NumberObj{Value: v.Value, IsInt: false})
	case StringObj:
		f, err := strconv.ParseFloat(v.Value, 64)
		if err != nil {
			return ReturnErrorf("could not convert string '%s' to float", v.Value)
		}
		return ReturnValue(NumberObj{Value: f, IsInt: false})
	case BooleanObj:
		if v.Value {
			return ReturnValue(NumberObj{Value: 1.0, IsInt: false})
		}
		return ReturnValue(NumberObj{Value: 0.0, IsInt: false})
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

func nativeArray(obj Object) Object {
	switch v := obj.(type) {
	case *ArrayObj:
		return ReturnValue(v)
	case *TupleObj:
		return ReturnValue(&ArrayObj{Elements: v.Elements})
	case StringObj:
		elements := make([]Object, len(v.Value))
		for i, char := range v.Value {
			elements[i] = StringObj{Value: string(char)}
		}
		return ReturnValue(&ArrayObj{Elements: elements})
	case *MapObj:
		elements := make([]Object, 0, len(v.Pairs))
		for _, bucket := range v.Pairs {
			for _, pair := range bucket {
				elements = append(elements, pair.Value)
			}
		}
		return ReturnValue(&ArrayObj{Elements: elements})
	case *RangeObj:
		elements := []Object{}
		for i := v.Start; (v.Step > 0 && i < v.End) || (v.Step < 0 && i > v.End); i += v.Step {
			elements = append(elements, NumberObj{Value: float64(i), IsInt: true})
		}
		return ReturnValue(&ArrayObj{Elements: elements})
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
		return ReturnValue(NumberObj{Value: float64(hashable.Hash()), IsInt: true})
	}
	return ReturnErrorf("object of type '%s' is not hashable", obj.Type())
}

func nativeExit() Object{
	os.Exit(0)
	return CreateNull()
}

func nativeAsciiCode(obj Object) Object {
	switch v := obj.(type) {
	case StringObj:
		if len(v.Value) == 0 {
			return ReturnError("string is empty")
		}
		if len(v.Value) > 1 {
			return ReturnError("string must be exactly one character")
		}
		return ReturnValue(NumberObj{Value: float64(v.Value[0]), IsInt: true})
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
	case ErrorObj:
		return nil, NewRuntimeError(v.Message, Loc{})
	default:
		return nil, NewRuntimeError(message.String(), Loc{})
	}
}

var Builtins = map[string]any{
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
}

var BuiltinDocs = map[string]*DocstringObj{
	"echo": {
		Description: "echo(...values) -> null\n\nPrints the given values to the console, separated by spaces.",
		Params:      []ParamDoc{{"values", "A variable number of objects to print."}},
		Returns:     "null",
	},
	"scan": {
		Description: "scan(prompt) -> string\n\nReads a line of input from the user after displaying a prompt.",
		Params:      []ParamDoc{{"prompt", "The string to display to the user."}},
		Returns:     "The user's input as a string.",
	},
	"type": {
		Description: "type(object) -> string\n\nReturns the type of an object as a string.",
		Params:      []ParamDoc{{"object", "The object to inspect."}},
		Returns:     "The type name as a string.",
	},
	"int": {
		Description: "int(object) -> (int, null)|(null, error)\n\nConverts an object to an integer. Returns (value, null) on success or (null, error) on failure.",
		Params:      []ParamDoc{{"object", "The object to convert."}},
		Returns:     "A tuple: (value, null) on success or (null, error) on failure.",
	},
	"float": {
		Description: "float(object) -> (float, null)|(null, error)\n\nConverts an object to a float. Returns (value, null) on success or (null, error) on failure.",
		Params:      []ParamDoc{{"object", "The object to convert."}},
		Returns:     "A tuple: (value, null) on success or (null, error) on failure.",
	},
	"tuple": {
		Description: "tuple(...elements) -> tuple\n\nCreates a new tuple containing the given elements.",
		Params:      []ParamDoc{{"elements", "A variable number of objects to include in the tuple."}},
		Returns:     "A new tuple object.",
	},
	"hash": {
		Description: "hash(object) -> (int, null)|(null, error)\n\nReturns the hash value of a hashable object. Returns (value, null) on success or (null, error) on failure.",
		Params:      []ParamDoc{{"object", "The object to hash."}},
		Returns:     "A tuple: (value, null) on success or (null, error) on failure.",
	},
	"asciiCode": {
		Description: "asciiCode(char_string) -> (int, null)|(null, error)\n\nReturns the ASCII code of a single-character string. Returns (value, null) on success or (null, error) on failure.",
		Params:      []ParamDoc{{"string", "A string containing exactly one character."}},
		Returns:     "A tuple: (value, null) on success or (null, error) on failure.",
	},
	"array": {
		Description: "array(object) -> (array, null)|(null, error)\n\nConverts an object to an array. Returns (value, null) on success or (null, error) on failure.",
		Params:      []ParamDoc{{"object", "The object to convert."}},
		Returns:     "A tuple: (value, null) on success or (null, error) on failure.",
	},
	"error": {
		Description: "error(message) -> error\n\nCreates a new error object with the given message.",
		Params:      []ParamDoc{{"message", "The error message string."}},
		Returns:     "A new error object.",
	},
	"panic": {
		Description: "panic(message) -> never returns\n\nStops execution and reports an error with the given message.",
		Params:      []ParamDoc{{"message", "The error message string."}},
		Returns:     "Never returns - execution stops.",
	},
}
