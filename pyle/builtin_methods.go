package pyle

import (
	"fmt"
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
		"len":  mustCreate("len", methodStringLen),
		"trimSpace": mustCreate("trimSpace", methodStringTrimSpace),
		"replace": mustCreate("replace", methodStringReplace),
		"split": mustCreate("split", methodStringSplit),
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
	}
}
