package pyle

import "time"

var programStartTime = time.Now()
var BuiltinModules map[string]map[string]any
var BuiltinModuleDocs map[string]*DocstringObj

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

func init() {
	BuiltinModules = make(map[string]map[string]any)
	BuiltinModuleDocs = make(map[string]*DocstringObj)


	// Docs for builtin methods

	// time module
	BuiltinModules["time"] = map[string]any{
		"sleep":  nativeSleep,
		"time":   nativeTime,
		"timeNs": nativeTimeNs,
		"timeMs": nativeTimeMs,
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
}