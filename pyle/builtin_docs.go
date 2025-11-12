package pyle

func init_docs() map[string]map[string]*DocstringObj {
	BuiltinMethodDocs := make(map[string]map[string]*DocstringObj)

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

	// --- Map Docs & Methods ---
	BuiltinMethodDocs["map"] = map[string]*DocstringObj{
		"len":  {Description: "len() -> int\n\nReturns the number of key-value pairs in the map."},
		"keys": {Description: "keys() -> iterator\n\nReturns an iterator over the map's keys."},
	}

	// --- Error Docs & Methods ---
	BuiltinMethodDocs["error"] = map[string]*DocstringObj{
		"message":  {Description: "message() -> string\n\nReturns the error message."},
		"toString": {Description: "toString() -> string\n\nReturns the string representation of the error."},
	}

	// --- Result Docs & Methods ---
	BuiltinMethodDocs["result"] = map[string]*DocstringObj{
		"unwrap":   {Description: "unwrap() -> T\n\nReturns the value inside the result, or panics if the result is an error."},
		"unwrapOr": {Description: "unwrapOr(default) -> T\n\nReturns the value inside the result, or the default value if the result is an error."},
		"catch":    {Description: "catch(handler) -> result\n\nCatches any errors in the result and returns the result of the handler function."},
	}

	// TODO: more docs needed

	return BuiltinMethodDocs
}
