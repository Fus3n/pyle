package pyle

import (
	"fmt"
	"strconv"
	"strings"
)

func isFunc(obj Object) bool {
	switch obj.(type) {
	case *FunctionObj, *ClosureObj, *NativeFuncObj, *BoundMethodObj:
		return true
	default:
		return false
	}
}

// BuiltinMethods holds the native methods for Pyle's built-in types.
var BuiltinMethods map[string]map[string]*NativeFuncObj
var BuiltinMethodDocs map[string]map[string]*DocstringObj

// --- String Methods ---
func methodStringLen(receiver StringObj) (int, error) {
	return len(receiver.Value), nil
}
func methodStringTrimSpace(receiver StringObj) string {
	return strings.TrimSpace(receiver.Value)
}
func methodStringReplace(receiver StringObj, old string, new string) string {
	return strings.ReplaceAll(receiver.Value, old, new)
}
func methodStringSplit(receiver StringObj, sep string) []string {
	return strings.Split(receiver.Value, sep)
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
func methodStringFormat(receiver StringObj, args ...Object) Object {
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
				return ReturnError("unmatched '{' in format string")
			}
			end += p + 1

			if argIndex >= len(args) {
				return ReturnError("not enough arguments for format string")
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
				return ReturnError(err.Error())
			}
			builder.WriteString(formatted)

			lastIndex = end + 1
		} else { // s[p] == '}'
			if p+1 < len(s) && s[p+1] == '}' {
				builder.WriteByte('}')
				lastIndex = p + 2
				continue
			}
			return ReturnError("single '}' encountered in format string")
		}
	}

	return ReturnValue(StringObj{Value: builder.String()})
}
func methodStringContains(receiver StringObj, substr StringObj) BooleanObj {
	return BooleanObj{Value: strings.Contains(receiver.Value, substr.Value)}
}
func methodStringHasPrefix(receiver StringObj, prefix StringObj) BooleanObj {
	return BooleanObj{Value: strings.HasPrefix(receiver.Value, prefix.Value)}
}
func methodStringHasSuffix(receiver StringObj, suffix StringObj) BooleanObj {
	return BooleanObj{Value: strings.HasSuffix(receiver.Value, suffix.Value)}
}
func methodStringToLower(receiver StringObj) StringObj {
	return StringObj{Value: strings.ToLower(receiver.Value)}
}
func methodStringToUpper(receiver StringObj) StringObj {
	return StringObj{Value: strings.ToUpper(receiver.Value)}
}
func methodStringIndexOf(receiver StringObj, substr StringObj) NumberObj {
	return NumberObj{Value: float64(strings.Index(receiver.Value, substr.Value)), IsInt: true}
}
func methodStringRepeat(receiver StringObj, count NumberObj) Object {
	if !count.IsInt || count.Value < 0 {
		return ReturnError("count must be a non-negative integer")
	}
	return ReturnValue(StringObj{Value: strings.Repeat(receiver.Value, int(count.Value))})
}
func methodStringAsciiAt(receiver StringObj, index NumberObj) Object {
	if !index.IsInt {
		return ReturnError("index must be an integer")
	}
	idx := int(index.Value)
	if idx < 0 || idx >= len(receiver.Value) {
		return ReturnErrorf("index out of bounds: %d", idx)
	}
	return ReturnValue(NumberObj{Value: float64(receiver.Value[idx]), IsInt: true})
}

// --- Array Methods ---
func methodArrayLen(receiver *ArrayObj) int {
	return len(receiver.Elements)
}
func methodArrayAppend(receiver *ArrayObj, value Object) Object {
	receiver.Elements = append(receiver.Elements, value)
	return receiver
}
func methodArrayPop(receiver *ArrayObj) Object {
	if len(receiver.Elements) == 0 {
		return NullObj{}
	}
	last := receiver.Elements[len(receiver.Elements)-1]
	receiver.Elements = receiver.Elements[:len(receiver.Elements)-1]
	return last
}
func methodArrayReverse(receiver *ArrayObj) Object{
	// in place
	for i, j := 0, len(receiver.Elements)-1; i < j; i, j = i+1, j-1 {
		receiver.Elements[i], receiver.Elements[j] = receiver.Elements[j], receiver.Elements[i]
	}
	return receiver
}
func methodArrayFilter(vm *VM, receiver *ArrayObj, fn Object) (Object, error) {
	if _, ok := fn.(NullObj); ok {
		return receiver, nil
	}

	if !isFunc(fn) {
		return ReturnErrorf("expected a function for filter, got %s", fn.Type()), nil
	}

	if len(receiver.Elements) == 0 {
		return ReturnValue(&ArrayObj{Elements: []Object{}}), nil
	}

	// Fast-path metadata for natives
	nativeFunc, isNativeFunc := fn.(*NativeFuncObj)

	filtered := make([]Object, 0, len(receiver.Elements))
	for _, elem := range receiver.Elements {
		var result Object
		var err Error

		if isNativeFunc && nativeFunc.DirectCall != nil {
			// Native direct call fast path: arity must be 1
			if nativeFunc.Arity != 1 {
				return nil, fmt.Errorf("filter function must have arity of 1")
			}
			if directFn, ok := nativeFunc.DirectCall.(NativeFunc1); ok {
				result, err = directFn(vm, elem)
			} else {
				result, err = vm.callPyleFuncFromNative(fn, []Object{elem})
			}
		} else if isNativeFunc && nativeFunc.ReflectCall != nil {
			// Reflection native fast path
			result, err = nativeFunc.ReflectCall(vm, []Object{elem})
		} else {
			// Closures, Pyle functions, bound methods
			result, err = vm.callPyleFuncFromNative(fn, []Object{elem})
		}
		if err != nil {
			return nil, err
		}

		// Boolean specialization
		if b, ok := result.(BooleanObj); ok {
			if b.Value {
				filtered = append(filtered, elem)
			}
		} else if result.IsTruthy() {
			filtered = append(filtered, elem)
		}
	}
	return ReturnValue(&ArrayObj{Elements: filtered}), nil
}

func methodArrayMap(vm *VM, receiver *ArrayObj, fn Object) (Object, error) {
	if _, ok := fn.(NullObj); ok {
		// No-op if fn is null
		return receiver, nil
	}

	// Ensure the passed object is a callable function/closure/native/bound method
	nativeFunc, isNativeFunc := fn.(*NativeFuncObj)
	if !isFunc(fn) {
		return ReturnErrorf("expected a function for map, got %s", fn.Type()), nil
	}

	mapped := make([]Object, len(receiver.Elements))
	for i, elem := range receiver.Elements {
		var result Object
		var err Error

		// FAST PATH for native functions
		if isNativeFunc && nativeFunc.DirectCall != nil {
			if nativeFunc.Arity != 1 {
				return nil, fmt.Errorf("map function must have arity of 1")
			}
			if directFn, ok := nativeFunc.DirectCall.(NativeFunc1); ok {
				result, err = directFn(vm, elem)
			} else {
				// Fallback to generic call if direct type mismatch
				result, err = vm.callPyleFuncFromNative(fn, []Object{elem})
			}
		} else if isNativeFunc && nativeFunc.ReflectCall != nil {
			// Reflection native fast path
			result, err = nativeFunc.ReflectCall(vm, []Object{elem})
		} else {
			// SLOW PATH: closures, Pyle functions, bound methods
			result, err = vm.callPyleFuncFromNative(fn, []Object{elem})
		}

		if err != nil {
			return nil, err
		}
		mapped[i] = result
	}
	return ReturnValue(&ArrayObj{Elements: mapped}), nil
}
func methodArrayToTuple(receiver *ArrayObj) Object {
	elements := make([]Object, len(receiver.Elements))
	copy(elements, receiver.Elements)
	return &TupleObj{Elements: elements}
}
func methodArrayJoin(receiver *ArrayObj, sep string) string {
	var sb strings.Builder
	for i, elem := range receiver.Elements {
		if i > 0 {
			sb.WriteString(sep)
		}
		sb.WriteString(elem.String())
	}
	return sb.String()
}

// --- Map Methods ---
func methodMapLen(receiver *MapObj) (int, error) {
	count := 0
	for _, bucket := range receiver.Pairs {
		count += len(bucket)
	}
	return count, nil
}
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

// --- Error Methods ---
func methodErrorMessage(receiver ErrorObj) (string, error) {
	return receiver.Message, nil
}

func methodErrorToString(receiver ErrorObj) (string, error) {
	return receiver.String(), nil
}

func init() {
	BuiltinMethods = make(map[string]map[string]*NativeFuncObj)
	BuiltinMethodDocs = make(map[string]map[string]*DocstringObj)

	// Helper to create native functions and panic on error
	mustCreate := func(name string, fn any, doc *DocstringObj) *NativeFuncObj {
		nativeFn, err := CreateNativeFunction(name, fn, doc)
		if err != nil {
			panic(err)
		}
		return nativeFn
	}

	// --- String Docs & Methods ---
	BuiltinMethodDocs["string"] = map[string]*DocstringObj{
		"len": {Description: "len() -> int\n\nReturns the number of characters in the string."},
		"replace": {
			Description: "replace(old, new) -> string\n\nReturns a new string with all occurrences of 'old' replaced by 'new'.",
			Params: []ParamDoc{
				{"old", "The substring to be replaced."},
				{"new", "The substring to replace with."},
			},
			Returns: "A new string with replacements made.",
		},
		// .. more needed
	}
	BuiltinMethods["string"] = map[string]*NativeFuncObj{
		"len":        mustCreate("len", methodStringLen, BuiltinMethodDocs["string"]["len"]),
		"trimSpace":  mustCreate("trimSpace", methodStringTrimSpace, nil),
		"replace":    mustCreate("replace", methodStringReplace, BuiltinMethodDocs["string"]["replace"]),
		"split":      mustCreate("split", methodStringSplit, nil),
		"format":     mustCreate("format", methodStringFormat, nil),
		"contains":   mustCreate("contains", methodStringContains, nil),
		"startsWith": mustCreate("startsWith", methodStringHasPrefix, nil),
		"endsWith":   mustCreate("endsWith", methodStringHasSuffix, nil),
		"toLower":    mustCreate("toLower", methodStringToLower, nil),
		"toUpper":    mustCreate("toUpper", methodStringToUpper, nil),
		"indexOf":    mustCreate("indexOf", methodStringIndexOf, nil),
		"repeat":     mustCreate("repeat", methodStringRepeat, nil),
		"asciiAt":    mustCreate("asciiAt", methodStringAsciiAt, nil),
		// suggest some more functions

	}

	// --- Array Docs & Methods ---
	BuiltinMethodDocs["array"] = map[string]*DocstringObj{
		"len":    {Description: "len() -> int\n\nReturns the number of elements in the array."},
		"append": {Description: "append(value)\n\nAppends a value to the end of the array in-place."},
		"join": {
			Description: "join(separator) -> string\n\nJoins the elements of the array into a string using the specified separator.",
			Params: []ParamDoc{
				{"separator", "The string to use as a separator between elements."},
			},
			Returns: "A new string with the joined elements.",
		},
	}
	BuiltinMethods["array"] = map[string]*NativeFuncObj{
		"len":     mustCreate("len", methodArrayLen, BuiltinMethodDocs["array"]["len"]),
		"append":  mustCreate("append", methodArrayAppend, BuiltinMethodDocs["array"]["append"]),
		"pop":     mustCreate("pop", methodArrayPop, nil),
		"reverse": mustCreate("reverse", methodArrayReverse, nil),
		"filter":  mustCreate("filter", methodArrayFilter, nil),
		"map":     mustCreate("map", methodArrayMap, nil),
		"toTuple": mustCreate("toTuple", methodArrayToTuple, nil),
		"join":    mustCreate("join", methodArrayJoin, BuiltinMethodDocs["array"]["join"]),
	}

	// --- Map Docs & Methods ---
	BuiltinMethodDocs["map"] = map[string]*DocstringObj{
		"len":  {Description: "len() -> int\n\nReturns the number of key-value pairs in the map."},
		"keys": {Description: "keys() -> iterator\n\nReturns an iterator over the map's keys."},
	}
	BuiltinMethods["map"] = map[string]*NativeFuncObj{
		"len":    mustCreate("len", methodMapLen, BuiltinMethodDocs["map"]["len"]),
		"keys":   mustCreate("keys", methodMapKeys, BuiltinMethodDocs["map"]["keys"]),
		"values": mustCreate("values", methodMapValues, nil),
		"items":  mustCreate("items", methodMapItems, nil),
		"has":    mustCreate("has", methodMapHas, nil),
	}

	// --- Error Docs & Methods ---
	BuiltinMethodDocs["error"] = map[string]*DocstringObj{
		"message": {Description: "message() -> string\n\nReturns the error message."},
		"toString": {Description: "toString() -> string\n\nReturns the string representation of the error."},
	}
	BuiltinMethods["error"] = map[string]*NativeFuncObj{
		"message":  mustCreate("message", methodErrorMessage, BuiltinMethodDocs["error"]["message"]),
		"toString": mustCreate("toString", methodErrorToString, BuiltinMethodDocs["error"]["toString"]),
	}
}
