package pyle

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"sync"
	"time"
)

type routeEntry struct {
	pattern    string
	segments   []string
	paramNames []int
	handler    Object
	methodMap  *MapObj
	isStatic   bool
	staticDir  string
}

var (
	routes   []routeEntry
	routesMu sync.RWMutex
)

func parsePattern(pattern string) ([]string, []int) {
	parts := strings.Split(strings.Trim(pattern, "/"), "/")
	var paramIndices []int
	for i, p := range parts {
		if strings.HasPrefix(p, ":") {
			paramIndices = append(paramIndices, i)
		}
	}
	return parts, paramIndices
}

func matchRoute(path string) (*routeEntry, *MapObj) {
	pathParts := strings.Split(strings.Trim(path, "/"), "/")

	routesMu.RLock()
	defer routesMu.RUnlock()

	// Prioritize static prefix matches
	for i := range routes {
		route := &routes[i]
		if route.isStatic {
			if strings.HasPrefix(path, route.pattern) {
				return route, nil
			}
			continue
		}
	}

	// Then match exact/parameterized routes
	for i := range routes {
		route := &routes[i]
		if route.isStatic {
			continue
		}
		if len(pathParts) != len(route.segments) {
			continue
		}

		params := NewMap()
		matched := true

		for j, seg := range route.segments {
			if strings.HasPrefix(seg, ":") {
				paramName := seg[1:]
				params.Set(StringObj{Value: paramName}, StringObj{Value: pathParts[j]})
			} else if seg != pathParts[j] {
				matched = false
				break
			}
		}

		if matched {
			return route, params
		}
	}
	return nil, nil
}

type HttpResponse struct {
	w          http.ResponseWriter
	r          *http.Request
	statusCode int
	written    bool
}

func NewHttpResponse(w http.ResponseWriter, r *http.Request) *HttpResponse {
	return &HttpResponse{w: w, r: r, statusCode: 200}
}

func (r *HttpResponse) Status(code int) *HttpResponse {
	r.statusCode = code
	return r
}

func (r *HttpResponse) writeHeader() {
	if !r.written {
		r.w.WriteHeader(r.statusCode)
		r.written = true
	}
}

func (r *HttpResponse) Send(val Object) *ResultObject {
	r.writeHeader()
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

	r.SetHeader("Content-Type", "application/json")
	r.writeHeader()

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

func (r *HttpResponse) SendFile(path string) *ResultObject {
	http.ServeFile(r.w, r.r, path)
	r.written = true
	return ReturnOkNull()
}

func (r *HttpResponse) SetHeader(key, val string) {
	r.w.Header().Set(key, val)
}

func (r *HttpResponse) Redirect(url string, code int) {
	r.w.Header().Set("Location", url)
	r.w.WriteHeader(code)
	r.written = true
}

func (r *HttpResponse) SetCookie(name, value string, options *MapObj) {
	cookie := &http.Cookie{
		Name:  name,
		Value: value,
		Path:  "/",
	}

	if options != nil {
		if maxAge, found, _ := options.Get(StringObj{Value: "maxAge"}); found {
			if n, ok := maxAge.(NumberObj); ok {
				cookie.MaxAge = int(n.Value)
			}
		}
		if path, found, _ := options.Get(StringObj{Value: "path"}); found {
			if s, ok := path.(StringObj); ok {
				cookie.Path = s.Value
			}
		}
		if httpOnly, found, _ := options.Get(StringObj{Value: "httpOnly"}); found {
			if b, ok := httpOnly.(BooleanObj); ok {
				cookie.HttpOnly = b.Value
			}
		}
		if secure, found, _ := options.Get(StringObj{Value: "secure"}); found {
			if b, ok := secure.(BooleanObj); ok {
				cookie.Secure = b.Value
			}
		}
		if sameSite, found, _ := options.Get(StringObj{Value: "sameSite"}); found {
			if s, ok := sameSite.(StringObj); ok {
				switch strings.ToLower(s.Value) {
				case "strict":
					cookie.SameSite = http.SameSiteStrictMode
				case "lax":
					cookie.SameSite = http.SameSiteLaxMode
				case "none":
					cookie.SameSite = http.SameSiteNoneMode
				}
			}
		}
		if expires, found, _ := options.Get(StringObj{Value: "expires"}); found {
			if n, ok := expires.(NumberObj); ok {
				cookie.Expires = time.Unix(int64(n.Value), 0)
			}
		}
	}

	http.SetCookie(r.w, cookie)
}

type HttpRequest struct {
	Method  string
	URL     string
	Proto   string
	Host    string
	Query   *MapObj
	Headers *MapObj
	Cookies *MapObj
	Params  *MapObj
	Body    string
}

func NewHttpRequest(r *http.Request, params *MapObj) *HttpRequest {
	query := NewMap()
	for key, values := range r.URL.Query() {
		if len(values) == 1 {
			query.Set(StringObj{Value: key}, StringObj{Value: values[0]})
		} else {
			arr := make([]Object, len(values))
			for i, v := range values {
				arr[i] = StringObj{Value: v}
			}
			query.Set(StringObj{Value: key}, &ArrayObj{Elements: arr})
		}
	}

	headers := NewMap()
	for key, values := range r.Header {
		if len(values) == 1 {
			headers.Set(StringObj{Value: key}, StringObj{Value: values[0]})
		} else {
			arr := make([]Object, len(values))
			for i, v := range values {
				arr[i] = StringObj{Value: v}
			}
			headers.Set(StringObj{Value: key}, &ArrayObj{Elements: arr})
		}
	}

	cookies := NewMap()
	for _, c := range r.Cookies() {
		cookies.Set(StringObj{Value: c.Name}, StringObj{Value: c.Value})
	}

	var body string
	if r.Body != nil {
		bodyBytes, err := io.ReadAll(r.Body)
		if err == nil {
			body = string(bodyBytes)
		}
		r.Body.Close()
	}

	if params == nil {
		params = NewMap()
	}

	return &HttpRequest{
		Method:  r.Method,
		URL:     r.URL.Path,
		Proto:   r.Proto,
		Host:    r.Host,
		Query:   query,
		Headers: headers,
		Cookies: cookies,
		Params:  params,
		Body:    body,
	}
}

func (req *HttpRequest) Json() *ResultObject {
	var data map[string]interface{}
	if err := json.Unmarshal([]byte(req.Body), &data); err != nil {
		return ReturnError(fmt.Sprintf("JSON parse error: %v", err))
	}
	return ReturnOk(goMapToPyleMap(data))
}

func goMapToPyleMap(data map[string]interface{}) *MapObj {
	result := NewMap()
	for key, val := range data {
		result.Set(StringObj{Value: key}, goValueToPyleObject(val))
	}
	return result
}

func goValueToPyleObject(val interface{}) Object {
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
		return goMapToPyleMap(v)
	case []interface{}:
		arr := make([]Object, len(v))
		for i, elem := range v {
			arr[i] = goValueToPyleObject(elem)
		}
		return &ArrayObj{Elements: arr}
	default:
		return StringObj{Value: fmt.Sprintf("%v", v)}
	}
}

func httpHandle(vm *VM, arg1, arg2 Object) (Object, Error) {
	pattern, ok := arg1.(StringObj)
	if !ok {
		return nil, NewRuntimeError("path must be a string", arg1.GetLocation())
	}

	segments, paramIndices := parsePattern(pattern.Value)
	entry := routeEntry{
		pattern:    pattern.Value,
		segments:   segments,
		paramNames: paramIndices,
	}

	if methodMap, isMap := arg2.(*MapObj); isMap {
		entry.methodMap = methodMap
	} else {
		entry.handler = arg2
	}

	routesMu.Lock()
	routes = append(routes, entry)
	routesMu.Unlock()

	return NullObj{}, nil
}

func httpStatic(vm *VM, route, dir StringObj) (Object, Error) {
	entry := routeEntry{
		pattern:   route.Value,
		isStatic:  true,
		staticDir: dir.Value,
	}

	routesMu.Lock()
	routes = append(routes, entry)
	routesMu.Unlock()

	return NullObj{}, nil
}

func collectAllowedMethods(methodMap *MapObj) []string {
	methods := []string{"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"}
	var allowed []string
	for _, m := range methods {
		if _, found, _ := methodMap.Get(StringObj{Value: m}); found {
			allowed = append(allowed, m)
		}
	}
	return allowed
}

func httpListen(vm *VM, addr StringObj) (Object, Error) {
	fmt.Printf("Starting Pyle HTTP server on %s\n", addr.Value)

	handler := http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		route, params := matchRoute(r.URL.Path)

		// Serve static files if matched route is static
		if route != nil && route.isStatic {
			fileServer := http.StripPrefix(route.pattern, http.FileServer(http.Dir(route.staticDir)))
			fileServer.ServeHTTP(w, r)
			return
		}

		if route == nil {
			http.NotFound(w, r)
			return
		}

		reqObj := NewHttpRequest(r, params)
		resObj := NewHttpResponse(w, r)

		pyleArgs := []Object{
			&UserObject{Value: reqObj},
			&UserObject{Value: resObj},
		}

		var handler Object
		if route.methodMap != nil {
			methodKey := StringObj{Value: strings.ToUpper(r.Method)}
			if h, found, _ := route.methodMap.Get(methodKey); found {
				handler = h
			} else if fallback, found, _ := route.methodMap.Get(StringObj{Value: "_"}); found {
				handler = fallback
			} else {
				allowed := collectAllowedMethods(route.methodMap)
				w.Header().Set("Allow", strings.Join(allowed, ", "))
				http.Error(w, "Method Not Allowed", http.StatusMethodNotAllowed)
				return
			}
		} else {
			handler = route.handler
		}

		res, err := vm.CallFunction(handler, pyleArgs)
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

	err := http.ListenAndServe(addr.Value, handler)
	if err != nil {
		return ReturnError(fmt.Sprintf("Server error: %s", err)), nil
	}
	return ReturnOkNull(), nil
}

func CreateHttpModule(vm *VM) Object {
	httpModule := NewModule("http")

	handleDoc := NewDocstring(
		"Registers a handler for the given path pattern. Supports path parameters like /users/:id",
		[]ParamDoc{
			{"path", "URL pattern (e.g. '/users/:id')"},
			{"handler", "Function(req, res) or {GET: fn, POST: fn, ...}"},
		},
		"null",
	)
	staticDoc := NewDocstring(
		"Serves static files from a directory at the given route prefix.",
		[]ParamDoc{
			{"route", "URL prefix (e.g. '/static')"},
			{"directory", "Local directory path (e.g. './public')"},
		},
		"null",
	)
	listenDoc := NewDocstring(
		"Starts the HTTP server.",
		[]ParamDoc{{"addr", "Address to listen on (e.g. ':8080')"}},
		"null",
	)

	ModuleMustRegister(httpModule, "handle", httpHandle, handleDoc)
	ModuleMustRegister(httpModule, "static", httpStatic, staticDoc)
	ModuleMustRegister(httpModule, "listen", httpListen, listenDoc)

	return httpModule
}

func RegisterHttpModule(vm *VM) {
	vm.RegisterBuiltinModule("http", CreateHttpModule)
}
