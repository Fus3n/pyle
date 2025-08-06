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

func nativeTimeNs() (int64, error) {
	return time.Now().UnixNano(), nil
}
func nativeTimeMs() (int64, error) {
	return time.Now().UnixMilli(), nil
}
func nativeTime() (int64, error) {
	return time.Now().Unix(), nil
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


func nativeArray(obj Object) (*ArrayObj, error) {
	switch v := obj.(type) {
	case *ArrayObj:
		return v, nil
	case *TupleObj:
		return &ArrayObj{Elements: v.Elements}, nil
	case StringObj:
		elements := make([]Object, len(v.Value))
		for i, char := range v.Value {
			elements[i] = StringObj{Value: string(char)}
		}
		return &ArrayObj{Elements: elements}, nil
	case *MapObj:
		elements := make([]Object, 0, len(v.Pairs))
		for _, bucket := range v.Pairs {
			for _, pair := range bucket {
				elements = append(elements, pair.Value)
			}
		}
		return &ArrayObj{Elements: elements}, nil
	case *RangeObj:
		elements := []Object{}
		for i := v.Start; (v.Step > 0 && i < v.End) || (v.Step < 0 && i > v.End); i += v.Step {
			elements = append(elements, NumberObj{Value: float64(i), IsInt: true})
		}
		return &ArrayObj{Elements: elements}, nil
	default:
		return nil, fmt.Errorf("cannot convert type '%s' to array", obj.Type())
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
		if len(v.Value) > 1 {
			return 0, fmt.Errorf("string must be exactly one character")
		}
		return int(v.Value[0]), nil
	default:
		return 0, fmt.Errorf("cannot convert type '%s' to ascii code", obj.Type())
	}
}

var Builtins = map[string]any{
	"echo":        builtinEcho,
	"scan":        nativeScan,
	"tuple":       nativeTuple,
	"type":        nativeType,
	"int":         nativeInt,
	"float":       nativeFloat,
	"string":      nativeString,
	"bool":        nativeBool,
	"array":       nativeArray,
	"expect":      nativeExpect,
	"hash":        nativeHash,
	"exit":        nativeExit,
	"asciiCode":   nativeAsciiCode,
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
	"tuple": {
		Description: "tuple(...elements) -> tuple\n\nCreates a new tuple containing the given elements.",
		Params:      []ParamDoc{{"elements", "A variable number of objects to include in the tuple."}},
		Returns:     "A new tuple object.",
	},
	"hash": {
		Description: "hash(object) -> int\n\nReturns the hash value of a hashable object.",
		Params:      []ParamDoc{{"object", "The object to hash."}},
		Returns:     "An integer representing the hash value.",
	},
	"asciiCode": {
		Description: "asciiCode(char_string) -> int\n\nReturns the ASCII code of a single-character string.",
		Params:      []ParamDoc{{"string", "A string containing exactly one character."}},
		Returns:     "The ASCII integer value of the character.",
	},
	"array": {
		Description: "array(object) -> array\n\nConverts an object to an array.",
		Params:      []ParamDoc{{"object", "The object to convert."}},
		Returns:     "An array object.",
	},
}
