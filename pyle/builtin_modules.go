package pyle

import (
	"os"
	"time"
)

var programStartTime = time.Now()
var BuiltinModules map[string]map[string]any
var BuiltinModuleDocs map[string]*DocstringObj

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
func nativeOsReadFile(path string) (string, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return "", err
	}
	return string(data), nil
}

func nativeOsWriteFile(path string, content string) (Object, error) {
	if err := os.WriteFile(path, []byte(content), 0o644); err != nil {
		return nil, err
	}
	return NullObj{}, nil
}

func nativeOsAppendFile(path string, content string) (Object, error) {
	f, err := os.OpenFile(path, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0o644)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	if _, err := f.WriteString(content); err != nil {
		return nil, err
	}
	return NullObj{}, nil
}

func nativeOsRemove(path string) (Object, error) {
	if err := os.Remove(path); err != nil {
		return nil, err
	}
	return NullObj{}, nil
}

func nativeOsExists(path string) (bool, error) {
	_, err := os.Stat(path)
	if err == nil {
		return true, nil
	}
	if os.IsNotExist(err) {
		return false, nil
	}
	return false, err
}

func nativeOsMkdir(path string) (Object, error) {
	if err := os.Mkdir(path, 0o755); err != nil {
		return nil, err
	}
	return NullObj{}, nil
}

func nativeOsMkdirAll(path string) (Object, error) {
	if err := os.MkdirAll(path, 0o755); err != nil {
		return nil, err
	}
	return NullObj{}, nil
}

func nativeOsListdir(path string) ([]string, error) {
	entries, err := os.ReadDir(path)
	if err != nil {
		return nil, err
	}
	names := make([]string, 0, len(entries))
	for _, e := range entries {
		names = append(names, e.Name())
	}
	return names, nil
}

func nativeOsStat(path string) (Object, error) {
	info, err := os.Stat(path)
	if err != nil {
		return nil, err
	}
	m := NewMap()
	_ = m.Set(StringObj{Value: "size"}, CreateInt(info.Size()))
	_ = m.Set(StringObj{Value: "mode"}, StringObj{Value: info.Mode().String()})
	_ = m.Set(StringObj{Value: "isDir"}, BooleanObj{Value: info.IsDir()})
	_ = m.Set(StringObj{Value: "mtime"}, NumberObj{Value: float64(info.ModTime().Unix()), IsInt: false})
	return m, nil
}

func init() {
	BuiltinModules = make(map[string]map[string]any)
	BuiltinModuleDocs = make(map[string]*DocstringObj)

	// Docs for builtin methods

	// time module
	BuiltinModules["time"] = map[string]any{
		"sleep":       nativeSleep,
		"time":        nativeTime,
		"timeNs":      nativeTimeNs,
		"timeMs":      nativeTimeMs,
		"perfCounter": nativePerfCounter,
	}

	BuiltinModuleDocs["time"] = NewDocstring("This module provides functions for working with time.", nil, "")
	BuiltinMethodDocs["time"] = map[string]*DocstringObj{
		"sleep": {
			Description: "replace(old, new) -> string\n\nReturns a new string with all occurrences of 'old' replaced by 'new'.",
			Params: []ParamDoc{
				{"int64", "The number of seconds to sleep."},
			},
		},
		"time": {
			Description: "time() -> float\n\nReturns the current Unix timestamp as a float (seconds since epoch).",
			Returns:     "float",
		},
		"timeMs": {Description: "timeMs() -> float\n\nReturns the current Unix timestamp in milliseconds as a float.",
			Returns: "float",
		},
		"timeNs": {
			Description: "timeNs() -> float\n\nReturns the current Unix timestamp in nanoseconds as a float.",
			Returns:     "float",
		},
		"perfCounter": {
			Description: "perfCounter() -> float\n\nReturns the value of a high-resolution performance counter as a float (seconds since program start).",
			Returns:     "float",
		},
	}

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

	BuiltinModuleDocs["os"] = NewDocstring("This module provides basic filesystem utilities.", nil, "")
	BuiltinMethodDocs["os"] = map[string]*DocstringObj{
		"readFile":   NewDocstring("readFile(path) -> string\n\nReads the entire file and returns its contents as a string.", []ParamDoc{{"path", "Filesystem path to read."}}, "string"),
		"writeFile":  NewDocstring("writeFile(path, content) -> null\n\nWrites content to file, creating/truncating it.", []ParamDoc{{"path", "Filesystem path."}, {"content", "Content to write."}}, "null"),
		"appendFile": NewDocstring("appendFile(path, content) -> null\n\nAppends content to file, creating it if needed.", []ParamDoc{{"path", "Filesystem path."}, {"content", "Content to append."}}, "null"),
		"remove":     NewDocstring("remove(path) -> null\n\nRemoves a file or empty directory.", []ParamDoc{{"path", "Path to remove."}}, "null"),
		"exists":     NewDocstring("exists(path) -> bool\n\nReturns whether a path exists.", []ParamDoc{{"path", "Path to check."}}, "bool"),
		"mkdir":      NewDocstring("mkdir(path) -> null\n\nCreates a directory with mode 0755.", []ParamDoc{{"path", "Directory path."}}, "null"),
		"mkdirAll":   NewDocstring("mkdirAll(path) -> null\n\nCreates a directory and all parents with mode 0755.", []ParamDoc{{"path", "Directory path."}}, "null"),
		"listdir":    NewDocstring("listdir(path) -> array\n\nLists names in a directory.", []ParamDoc{{"path", "Directory path."}}, "array"),
		"stat":       NewDocstring("stat(path) -> map\n\nReturns a map with keys: size (int), mode (string), isDir (bool), mtime (float seconds).", []ParamDoc{{"path", "Path to stat."}}, "map"),
	}
}
