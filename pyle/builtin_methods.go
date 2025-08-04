package pyle

import (
	"fmt"
	"strconv"
	"strings"
)

func isFunc(obj Object) bool {
	switch obj.(type) {
	case *FunctionObj, *NativeFuncObj, *BoundMethodObj:
		return true
	default:
		return false
	}
}

// BuiltinMethods holds the native methods for Pyle's built-in types.
var BuiltinMethods map[string]map[string]*NativeFuncObj

// --- String Methods ---
func methodStringLen(receiver StringObj) (int, error) {
	return len(receiver.Value), nil
}
func methodStringTrimSpace(receiver StringObj) (string, error) {
	return strings.TrimSpace(receiver.Value), nil
}
func methodStringReplace(receiver StringObj, old string, new string) (string, error) {
	return strings.ReplaceAll(receiver.Value, old, new), nil
}
func methodStringSplit(receiver StringObj, sep string) ([]string, error) {
	return strings.Split(receiver.Value, sep), nil
}

func formatArgument(arg Object, spec string) (string, error) {
	if spec == "" || spec == "s" {
		return arg.String(), nil
	}

	if spec == "d" {
		num, ok := arg.(NumberObj)
		if !ok || !num.IsInt {
			return "", fmt.Errorf("expected an integer for 'd' format specifier, got %s", arg.Type())
		}
		return strconv.FormatInt(int64(num.Value), 10), nil
	}

	if strings.HasSuffix(spec, "f") {
		num, ok := arg.(NumberObj)
		if !ok {
			return "", fmt.Errorf("expected a number for 'f' format specifier, got %s", arg.Type())
		}

		precision := -1 // default precision
		specWithoutF := spec[:len(spec)-1]
		if specWithoutF != "" {
			if !strings.HasPrefix(specWithoutF, ".") {
				return "", fmt.Errorf("invalid format specifier for 'f': %s", spec)
			}
			precisionStr := specWithoutF[1:]
			if p, err := strconv.Atoi(precisionStr); err == nil {
				precision = p
			} else {
				return "", fmt.Errorf("invalid precision for 'f' format specifier: %s", spec)
			}
		}
		return strconv.FormatFloat(num.Value, 'f', precision, 64), nil
	}

	return "", fmt.Errorf("unsupported format specifier: %s", spec)
}

func methodStringFormat(receiver StringObj, args ...Object) (string, error) {
	var builder strings.Builder
	var argIndex int
	s := receiver.Value
	lastIndex := 0

	for {
		p := strings.IndexAny(s[lastIndex:], "{}")
		if p == -1 {
			builder.WriteString(s[lastIndex:])
			break
		}
		p += lastIndex

		builder.WriteString(s[lastIndex:p])

		if s[p] == '{' {
			if p+1 < len(s) && s[p+1] == '{' {
				builder.WriteByte('{')
				lastIndex = p + 2
				continue
			}

			end := strings.IndexByte(s[p+1:], '}')
			if end == -1 {
				return "", fmt.Errorf("unmatched '{' in format string")
			}
			end += p + 1

			if argIndex >= len(args) {
				return "", fmt.Errorf("not enough arguments for format string")
			}
			arg := args[argIndex]
			argIndex++

			specWithColon := s[p+1 : end]
			var spec string
			if colonIndex := strings.Index(specWithColon, ":"); colonIndex != -1 {
				spec = specWithColon[colonIndex+1:]
			} else {
				spec = specWithColon
			}

			formatted, err := formatArgument(arg, spec)
			if err != nil {
				return "", err
			}
			builder.WriteString(formatted)

			lastIndex = end + 1
		} else { // s[p] == '}'
			if p+1 < len(s) && s[p+1] == '}' {
				builder.WriteByte('}')
				lastIndex = p + 2
				continue
			}
			return "", fmt.Errorf("single '}' encountered in format string")
		}
	}

	return builder.String(), nil
}

// --- Array Methods ---
func methodArrayLen(receiver *ArrayObj) (int, error) {
	return len(receiver.Elements), nil
}
func methodArrayAppend(receiver *ArrayObj, value Object) (Object, error) {
	receiver.Elements = append(receiver.Elements, value)
	return receiver, nil
}
func methodArrayPop(receiver *ArrayObj) (Object, error) {
	if len(receiver.Elements) == 0 {
		return NullObj{}, nil
	}	
	last := receiver.Elements[len(receiver.Elements)-1]
	receiver.Elements = receiver.Elements[:len(receiver.Elements)-1]
	return last, nil
}
func methodArrayReverse(receiver *ArrayObj) (Object, error) {
	// in place
	for i, j := 0, len(receiver.Elements)-1; i < j; i, j = i+1, j-1 {
		receiver.Elements[i], receiver.Elements[j] = receiver.Elements[j], receiver.Elements[i]
	}
	return receiver, nil
}
func methodArrayFilter(vm *VM, receiver *ArrayObj, fn Object) (Object, error) {
	if _, ok := fn.(NullObj); ok {
		// No-op if fn is null
		return receiver, nil
	}

	if !isFunc(fn) {
		return nil, fmt.Errorf("expected a function for filter, got %s", fn.Type())
	}

	var filtered []Object
	for _, elem := range receiver.Elements {
		// Use the new internal helper to handle the call
		result, err := vm.callPyleFuncFromNative(fn, []Object{elem})
		if err != nil {
			return nil, err
		}

		// Include the element if the function returned a truthy value
		if result.IsTruthy() {
			filtered = append(filtered, elem)
		}
	}
	return &ArrayObj{Elements: filtered}, nil
}

func methodArrayMap(vm *VM, receiver *ArrayObj, fn Object) (Object, error) {
	if _, ok := fn.(NullObj); ok {
		// No-op if fn is null
		return receiver, nil
	}
	// Ensure the passed object is a callable function
	if !isFunc(fn) {
		return nil, fmt.Errorf("expected a function for filter, got %s", fn.Type())
	}

	mapped := make([]Object, len(receiver.Elements))
	for i, elem := range receiver.Elements {
		result, err := vm.callPyleFuncFromNative(fn, []Object{elem})
		if err != nil {
			return nil, err
		}
		mapped[i] = result
	}
	return &ArrayObj{Elements: mapped}, nil

}

// --- Map Methods ---
func methodMapKeys(receiver *MapObj) (Object, error) {
	return NewMapIterator(receiver, MapIteratorModeKeys), nil
}
func methodMapValues(receiver *MapObj) (Object, error) {
	return NewMapIterator(receiver, MapIteratorModeValues), nil
}
func methodMapItems(receiver *MapObj) (Object, error) {
	return NewMapIterator(receiver, MapIteratorModeItems), nil
}
func methodMapHas(receiver *MapObj, key Object) (bool, error) {
	_, ok, err := receiver.Get(key)
	if err != nil {
		return false, err
	}
	return ok, nil
}

func init() {
	BuiltinMethods = make(map[string]map[string]*NativeFuncObj)

	mustCreate := func(name string, fn any) *NativeFuncObj {
		nativeFn, err := CreateNativeFunction(name, fn)
		if err != nil {
			panic(err)
		}
		return nativeFn
	}

	BuiltinMethods["string"] = map[string]*NativeFuncObj{
		"len":       mustCreate("len", methodStringLen),
		"trimSpace": mustCreate("trimSpace", methodStringTrimSpace),
		"replace":   mustCreate("replace", methodStringReplace),
		"split":     mustCreate("split", methodStringSplit),
		"format":    mustCreate("format", methodStringFormat),
	}

	BuiltinMethods["array"] = map[string]*NativeFuncObj{
		"len":    mustCreate("len", methodArrayLen),
		"append": mustCreate("append", methodArrayAppend),
		// other useful array methods
		"pop": mustCreate("pop", methodArrayPop),
		"reverse": mustCreate("reverse", methodArrayReverse),
		"filter":  mustCreate("filter", methodArrayFilter),
		"map": mustCreate("map", methodArrayMap),
	}

	BuiltinMethods["map"] = map[string]*NativeFuncObj{
		"keys":   mustCreate("keys", methodMapKeys),
		"values": mustCreate("values", methodMapValues),
		"items":  mustCreate("items", methodMapItems),
		"has":    mustCreate("has", methodMapHas),
	}
}
