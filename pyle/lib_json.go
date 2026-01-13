package pyle

import (
	"encoding/json"
	"fmt"
)

func jsonParse(vm *VM, str StringObj) (Object, Error) {
	var data interface{}
	err := json.Unmarshal([]byte(str.Value), &data)
	if err != nil {
		return ReturnError(fmt.Sprintf("JSON parse error: %v", err)), nil
	}

	return ReturnOk(goValueToPyleObjectJSON(data)), nil
}

func jsonStringify(vm *VM, arg Object) (Object, Error) {
	goVal := ToGoValue(arg)
	
	bytes, err := json.Marshal(goVal)
	if err != nil {
		return ReturnError(fmt.Sprintf("JSON stringify error: %v", err)), nil
	}

	return ReturnOk(StringObj{Value: string(bytes)}), nil
}

func goValueToPyleObjectJSON(val interface{}) Object {
	switch v := val.(type) {
	case string:
		return StringObj{Value: v}
	case float64:
		isInt := v == float64(int64(v))
		return NumberObj{Value: v, IsInt: isInt}
	case bool:
		return BooleanObj{Value: v}
	case nil:
		return NullObj{}
	case map[string]interface{}:
		result := NewMap()
		for key, val := range v {
			result.Set(StringObj{Value: key}, goValueToPyleObjectJSON(val))
		}
		return result
	case []interface{}:
		arr := make([]Object, len(v))
		for i, elem := range v {
			arr[i] = goValueToPyleObjectJSON(elem)
		}
		return &ArrayObj{Elements: arr}
	default:
		return StringObj{Value: fmt.Sprintf("%v", v)}
	}
}

func CreateJsonModule(vm *VM) Object {
	jsonModule := NewModule("json")

	parseDoc := NewDocstring("Parses a JSON string into a Pyle object (Map or Array).", []ParamDoc{{"text", "JSON string"}}, "Map | Array | basic type")
	stringifyDoc := NewDocstring("Converts a Pyle object into a JSON string.", []ParamDoc{{"value", "Value to stringify"}}, "string")

	ModuleMustRegister(jsonModule, "parse", jsonParse, parseDoc)
	ModuleMustRegister(jsonModule, "stringify", jsonStringify, stringifyDoc)

	return jsonModule
}

func RegisterJsonModule(vm *VM) {
	vm.RegisterBuiltinModule("json", CreateJsonModule)
}
