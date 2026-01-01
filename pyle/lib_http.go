package pyle

import (
	"encoding/json"
	"fmt"
	"net/http"
)

// Helper to convert Pyle Objects to Go interface{} for JSON marshaling
type HttpResponse struct {
	w http.ResponseWriter
}

func (r *HttpResponse) Send(val Object) *ResultObject {
	if str, ok := val.(StringObj); ok {
		if _, err := fmt.Fprintf(r.w, "%s", str.Value); err != nil {
			return ReturnError(err.Error())
		}
		return ReturnOkNull()
	}

	goVal := ToGoValue(val)
	bytes, err := json.Marshal(goVal)
	if err != nil {
		return ReturnError(fmt.Sprintf("JSON Marshal Error: %v", err))
	}
	if _, err := r.w.Write(bytes); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func (r *HttpResponse) SendJson(val Object) *ResultObject {
	if _, ok := val.(*MapObj); !ok {
		return ReturnError("Invalid JSON type, expected map")
	}

	goVal := ToGoValue(val)
	bytes, err := json.Marshal(goVal)
	if err != nil {
		return ReturnError(fmt.Sprintf("JSON Marshal Error: %v", err))
	}
	if _, err := r.w.Write(bytes); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func (r *HttpResponse) SetHeader(key, val string) {
	r.w.Header().Set(key, val)
}

type HttpRequest struct {
	Method string
	URL    string
	Proto  string
	Host   string
}

func httpHandle(vm *VM, arg1, arg2 Object) (Object, Error) {
	path, ok := arg1.(StringObj)
	if !ok {
		return nil, NewRuntimeError("path must be a string", arg1.GetLocation())
	}
	
	callback := arg2

	http.HandleFunc(path.Value, func(w http.ResponseWriter, r *http.Request) {
		reqObj := &HttpRequest{
			Method: r.Method,
			URL:    r.URL.Path,
			Proto:  r.Proto,
			Host:   r.Host,
		}
		resObj := &HttpResponse{w: w}

		pyleArgs := []Object{
			&UserObject{Value: reqObj}, 
			&UserObject{Value: resObj},
		}

		res, err := vm.CallFunction(callback, pyleArgs)
		if err != nil {
			fmt.Printf("HTTP Handler Runtime Error: %v\n", err)
			http.Error(w, "Internal Server Error", 500)
			return
		}

		if result, ok := res.(*ResultObject); ok && result.Error != nil {
			fmt.Printf("HTTP Handler Logic Error: %v\n", result.Error.Message)
			http.Error(w, result.Error.Message, 500)
			return
		}
	})

	return NullObj{}, nil
}

func httpListen(vm *VM, arg Object) (Object, Error) {
	addr, ok := arg.(StringObj)
	if !ok {
		return nil, NewRuntimeError("address must be a string", arg.GetLocation())
	}

	fmt.Printf("Starting Pyle HTTP server on %s\n", addr.Value)
	
	err := http.ListenAndServe(addr.Value, nil)

	if err != nil {
		return ReturnError(fmt.Sprintf("Server error: %s", err)), nil
	}
	return ReturnOkNull(), nil
}

func CreateHttpModule(vm *VM) Object {
	httpModule := NewModule("http")
	
	handleDoc := NewDocstring("Registers a handler for the given path.", []ParamDoc{{"path", "URL path"}, {"handler", "Function(req, res)"}}, "null")
	listenDoc := NewDocstring("Starts the HTTP server.", []ParamDoc{{"addr", "Address to listen on (e.g. ':8080')"}}, "null")

	ModuleMustRegister(httpModule, "handle", httpHandle, handleDoc)
	ModuleMustRegister(httpModule, "listen", httpListen, listenDoc)
	
	return httpModule
}

func RegisterHttpModule(vm *VM) {
	vm.RegisterBuiltinModule("http", CreateHttpModule)
}
