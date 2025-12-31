package pyle

import (
	"os"
	"time"
)

var programStartTime = time.Now()
var BuiltinModules map[string]map[string]any


// Time module
func nativeTimeNs() (int64, error) {
	return time.Now().UnixNano(), nil
}
func nativeTimeMs() (int64, error) {
	return time.Now().UnixMilli(), nil
}
func nativeTime() (float64, error) {
	return time.Duration(time.Now().UnixNano()).Seconds(), nil
}
func nativeSleep(seconds float64) {
	time.Sleep(time.Duration(seconds * float64(time.Second)))
}
func nativePerfCounter() (float64, error) {
	return time.Since(programStartTime).Seconds(), nil
}

// OS module
func nativeOsReadFile(path string) *ResultObject {
	data, err := os.ReadFile(path)
	if err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkString(string(data))
}

func nativeOsWriteFile(path string, content string) *ResultObject {
	if err := os.WriteFile(path, []byte(content), 0o644); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func nativeOsAppendFile(path string, content string) *ResultObject {
	f, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0o644)
	if err != nil {
		return ReturnError(err.Error())
	}
	defer f.Close()
	if _, err := f.WriteString(content); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func nativeOsRemove(path string) *ResultObject {
	if err := os.Remove(path); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func nativeOsExists(path string) *ResultObject {
	_, err := os.Stat(path)
	if err == nil {
		return ReturnOkBool(true)
	}
	if os.IsNotExist(err) {
		return ReturnOkBool(false)
	}
	return ReturnError(err.Error())
}

func nativeOsMkdir(path string) *ResultObject {
	if err := os.Mkdir(path, 0o755); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func nativeOsMkdirAll(path string) *ResultObject {
	if err := os.MkdirAll(path, 0o755); err != nil {
		return ReturnError(err.Error())
	}
	return ReturnOkNull()
}

func nativeOsListdir(path string) *ResultObject {
    entries, err := os.ReadDir(path)
    if err != nil {
        return ReturnError(err.Error())
    }
    elements := make([]Object, len(entries))
    for i, e := range entries {
        elements[i] = StringObj{Value: e.Name()} 
    }
    return ReturnOk(&ArrayObj{Elements: elements})
}

func nativeOsStat(path string) *ResultObject {
	info, err := os.Stat(path)
	if err != nil {
		return ReturnError(err.Error())
	}
	m := NewMap()
	_ = m.Set(StringObj{Value: "size"}, CreateInt(info.Size()))
	_ = m.Set(StringObj{Value: "mode"}, StringObj{Value: info.Mode().String()})
	_ = m.Set(StringObj{Value: "isDir"}, BooleanObj{Value: info.IsDir()})
	_ = m.Set(StringObj{Value: "mtime"}, NumberObj{Value: float64(info.ModTime().Unix()), IsInt: false})
	return ReturnOk(m) 
}

func init() {
	BuiltinModules = make(map[string]map[string]any)


	// time module
	BuiltinModules["time"] = map[string]any{
		"sleep":       nativeSleep,
		"time":        nativeTime,
		"timeNs":      nativeTimeNs,
		"timeMs":      nativeTimeMs,
		"perfCounter": nativePerfCounter,
	}

	// os module
	BuiltinModules["os"] = map[string]any{
		"readFile":   nativeOsReadFile,
		"writeFile":  nativeOsWriteFile,
		"appendFile": nativeOsAppendFile,
		"remove":     nativeOsRemove,
		"exists":     nativeOsExists,
		"mkdir":      nativeOsMkdir,
		"mkdirAll":   nativeOsMkdirAll,
		"listdir":    nativeOsListdir,
		"stat":       nativeOsStat,
	}


}
