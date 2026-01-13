package pyle

var BuiltinDocs map[string]*DocstringObj
var BuiltinModuleDocs map[string]*DocstringObj
var BuiltinMethodDocs map[string]map[string]*DocstringObj

func LoadDocs() {
	// Globals
	BuiltinDocs = map[string]*DocstringObj{
		"echo": NewDocstring(
			"Prints values to the console.",
			[]ParamDoc{
				{"values", "Objects to print."},
			},
			"null",
		),
		"scan": NewDocstring(
			"Reads a line of input from the user.",
			[]ParamDoc{
				{"prompt", "The string to display to the user."},
			},
			"string",
		),
		"type": NewDocstring(
			"Returns the type of an object as a string.",
			[]ParamDoc{
				{"object", "The object to inspect."},
			},
			"string",
		),
		"int": NewDocstring(
			"Converts an object to an integer.",
			[]ParamDoc{
				{"object", "The object to convert."},
			},
			"result<int>",
		),
		"float": NewDocstring(
			"Converts an object to a float.",
			[]ParamDoc{
				{"object", "The object to convert."},
			},
			"result<float>",
		),
		"tuple": NewDocstring(
			"Creates a new tuple containing the given elements.",
			[]ParamDoc{
				{"elements", "Objects to include in the tuple."},
			},
			"result<tuple>",
		),
		"hash": NewDocstring(
			"Returns the hash value of a hashable object.",
			[]ParamDoc{
				{"object", "The object to hash."},
			},
			"result<int>",
		),
		"asciiCode": NewDocstring(
			"Returns the ASCII code of a single-character string.",
			[]ParamDoc{
				{"string", "A string containing exactly one character."},
			},
			"result<int>",
		),
		"array": NewDocstring(
			"Converts an object to an array.",
			[]ParamDoc{
				{"object", "The object to convert."},
			},
			"result<array>",
		),
		"error": NewDocstring(
			"Creates a new error object with the given message.",
			[]ParamDoc{
				{"message", "The error message string."},
			},
			"error",
		),
		"panic": NewDocstring(
			"Stops execution and reports an error.",
			[]ParamDoc{
				{"message", "The error message string."},
			},
			"never",
		),
		"Ok": NewDocstring(
			"Wraps a value in a result object with no error.",
			[]ParamDoc{
				{"value", "The value to wrap."},
			},
			"result<T>",
		),
		"Err": NewDocstring(
			"Creates a result object representing an error.",
			[]ParamDoc{
				{"message|error", "A string message or error object."},
			},
			"result<T>",
		),
	}

	// Modules
	BuiltinModuleDocs = map[string]*DocstringObj{
		"time":     NewDocstring("This module provides functions for working with time.", nil, ""),
		"os":       NewDocstring("This module provides basic filesystem utilities.", nil, ""),
		"pylegame": NewDocstring("A high-performance 2D game engine module based on Ebitengine.", nil, ""),
		"http":     NewDocstring("This module provides a basic HTTP server for building web applications.", nil, ""),
		"random":   NewDocstring("This module provides functions for generating random numbers.", nil, ""),
		"json":     NewDocstring("This module provides functions for working with JSON data.", nil, ""),
	}

	BuiltinMethodDocs = make(map[string]map[string]*DocstringObj)

	// String Methods
	BuiltinMethodDocs["string"] = map[string]*DocstringObj{
		"len": NewDocstring(
			"Returns the number of characters in the string.",
			nil,
			"int",
		),
		"trimSpace": NewDocstring(
			"Returns a new string with leading and trailing white space removed.",
			nil,
			"string",
		),
		"split": NewDocstring(
			"Splits the string into a slice of substrings separated by the specified delimiter.",
			[]ParamDoc{
				{"delim", "The substring to split by."},
			},
			"[]string",
		),
		"format": NewDocstring(
			"Formats the string using the specified format string and arguments.",
			[]ParamDoc{
				{"format", "The format string."},
				{"args", "The arguments to format."},
			},
			"result<string>",
		),
		"contains": NewDocstring(
			"Returns true if the string contains the specified substring.",
			[]ParamDoc{
				{"substring", "The substring to search for."},
			},
			"bool",
		),
		"startsWith": NewDocstring(
			"Returns true if the string starts with the specified prefix.",
			[]ParamDoc{
				{"prefix", "The prefix to search for."},
			},
			"bool",
		),
		"endsWith": NewDocstring(
			"Returns true if the string ends with the specified suffix.",
			[]ParamDoc{
				{"suffix", "The suffix to search for."},
			},
			"bool",
		),
		"toLower": NewDocstring(
			"Returns a new string with all characters converted to lowercase.",
			nil,
			"string",
		),
		"toUpper": NewDocstring(
			"Returns a new string with all characters converted to uppercase.",
			nil,
			"string",
		),
		"indexOf": NewDocstring(
			"Returns the index of the first occurrence of the specified substring, or -1 if not found.",
			[]ParamDoc{
				{"substring", "The substring to search for."},
			},
			"int",
		),
		"replace": NewDocstring(
			"Returns a new string with all occurrences of 'old' replaced by 'new'.",
			[]ParamDoc{
				{"old", "The substring to be replaced."},
				{"new", "The substring to replace with."},
			},
			"string",
		),
		"repeat": NewDocstring(
			"Returns a new string with the specified string repeated the specified number of times.",
			[]ParamDoc{
				{"count", "The number of times to repeat the string."},
			},
			"string",
		),
		"asciiAt": NewDocstring(
			"Returns the ASCII value of the character at the specified index.",
			[]ParamDoc{
				{"index", "The index of the character to get the ASCII value of."},
			},
			"result<int>",
		),
	}

	// Array Methods
	BuiltinMethodDocs["array"] = map[string]*DocstringObj{
		"len": NewDocstring(
			"Returns the number of elements in the array.",
			nil,
			"int",
		),
		"append": NewDocstring(
			"Appends a value to the end of the array in-place.",
			[]ParamDoc{
				{"object", "The value to append to the array."},
			},
			"null",
		),
		"join": NewDocstring(
			"Joins the elements of the array into a string using the specified separator.",
			[]ParamDoc{
				{"separator", "The string to use as a separator between elements."},
			},
			"string",
		),
		"pop": NewDocstring(
			"Removes and returns the last element of the array.",
			nil,
			"object",
		),
		"reverse": NewDocstring(
			"Reverses the order of the elements in the array in-place.",
			nil,
			"array",
		),
		"filter": NewDocstring(
			"Returns a new array with elements that pass the test implemented by the provided function.",
			[]ParamDoc{
				{"fn", "The function to test each element of the array."},
			},
			"result<array>",
		),
		"map": NewDocstring(
			"Returns a new array with each element transformed by the provided function.",
			[]ParamDoc{
				{"fn", "The function to apply to each element of the array."},
			},
			"result<array>",
		),
		"toTuple": NewDocstring(
			"Converts the array to a tuple.",
			nil,
			"tuple",
		),
		"remove": NewDocstring(
			"Removes and returns the element at the specified index.",
			[]ParamDoc{
				{"index", "The index of the element to remove."},
			},
			"result<object>",
		),
		"clear": NewDocstring(
			"Removes all elements from the array.",
			nil,
			"null",
		),
	}

	// Map Methods
	BuiltinMethodDocs["map"] = map[string]*DocstringObj{
		"len": NewDocstring(
			"Returns the number of key-value pairs in the map.",
			nil,
			"int",
		),
		"keys": NewDocstring(
			"Returns an iterator over the map's keys.",
			nil,
			"iterator<T>",
		),
		"values": NewDocstring(
			"Returns an iterator over the map's values.",
			nil,
			"iterator<T>",
		),
		"items": NewDocstring(
			"Returns an iterator over the map's key-value pairs.",
			nil,
			"iterator<T>",
		),
		"has": NewDocstring(
			"Checks if the map has a key.",
			[]ParamDoc{
				{"key", "The key to check."},
			},
			"bool",
		),
	}

	// Error Methods
	BuiltinMethodDocs["error"] = map[string]*DocstringObj{
		"message": NewDocstring(
			"Returns the error message.",
			nil,
			"string",
		),
		"toString": NewDocstring(
			"Returns the string representation of the error.",
			nil,
			"string",
		),
	}

	// Result Methods
	BuiltinMethodDocs["result"] = map[string]*DocstringObj{
		"unwrap": NewDocstring(
			"Returns the value inside the result, or panics if the result is an error.",
			nil,
			"T",
		),
		"unwrapOr": NewDocstring(
			"Returns the value inside the result, or the default value if the result is an error.",
			[]ParamDoc{
				{"defaultValue", "The default value to return if the result is an error."},
			},
			"T",
		),
		"catch": NewDocstring(
			"Catches any errors in the result and returns the result of the handler function.",
			[]ParamDoc{
				{"fn", "The function to apply to the error if the result is an error."},
			},
			"result<T>",
		),
	}

	// Module Methods: Time
	BuiltinMethodDocs["time"] = map[string]*DocstringObj{
		"sleep": NewDocstring(
			"Pauses execution for the specified number of seconds.",
			[]ParamDoc{
				{"seconds", "The number of seconds to sleep."},
			},
			"null",
		),
		"time": NewDocstring(
			"Returns the current Unix timestamp as a float (seconds since epoch).",
			nil,
			"float",
		),
		"timeMs": NewDocstring(
			"Returns the current Unix timestamp in milliseconds as a float.",
			nil,
			"float",
		),
		"timeNs": NewDocstring(
			"Returns the current Unix timestamp in nanoseconds as a float.",
			nil,
			"float",
		),
		"perfCounter": NewDocstring(
			"Returns the value of a high-resolution performance counter as a float (seconds since program start).",
			nil,
			"float",
		),
	}

	// Module Methods: OS
	BuiltinMethodDocs["os"] = map[string]*DocstringObj{
		"readFile": NewDocstring(
			"Reads the entire file and returns its contents as a string.",
			[]ParamDoc{
				{"path", "Filesystem path to read."},
			},
			"string",
		),
		"writeFile": NewDocstring(
			"Writes content to file, creating/truncating it.",
			[]ParamDoc{
				{"path", "Filesystem path."},
				{"content", "Content to write."},
			},
			"null",
		),
		"appendFile": NewDocstring(
			"Appends content to file, creating it if needed.",
			[]ParamDoc{
				{"path", "Filesystem path."},
				{"content", "Content to append."},
			},
			"null",
		),
		"remove": NewDocstring(
			"Removes a file or empty directory.",
			[]ParamDoc{
				{"path", "Path to remove."},
			},
			"null",
		),
		"exists": NewDocstring(
			"Returns whether a path exists.",
			[]ParamDoc{
				{"path", "Path to check."},
			},
			"bool",
		),
		"mkdir": NewDocstring(
			"Creates a directory with mode 0755.",
			[]ParamDoc{
				{"path", "Directory path."},
			},
			"null",
		),
		"mkdirAll": NewDocstring(
			"Creates a directory and all parents with mode 0755.",
			[]ParamDoc{
				{"path", "Directory path."},
			},
			"null",
		),
		"listdir": NewDocstring(
			"Lists names in a directory.",
			[]ParamDoc{
				{"path", "Directory path."},
			},
			"array",
		),
		"stat": NewDocstring(
			"Returns a map with keys: size (int), mode (string), isDir (bool), mtime (float seconds).",
			[]ParamDoc{
				{"path", "Path to stat."},
			},
			"map",
		),
	}

	// Module Methods: Random
	BuiltinMethodDocs["random"] = map[string]*DocstringObj{
		"intn": NewDocstring(
			"Returns a random integer in [0, n).",
			[]ParamDoc{{"n", "Upper bound (exclusive)."}},
			"int",
		),
		"float": NewDocstring(
			"Returns a random float in [0.0, 1.0).",
			[]ParamDoc{},
			"float",
		),
		"int": NewDocstring(
			"Returns a random non-negative integer.",
			[]ParamDoc{},
			"int",
		),
		"rangeInt": NewDocstring(
			"Returns a random integer in [min, max).",
			[]ParamDoc{
				{"min", "Minimum value (inclusive)."},
				{"max", "Maximum value (exclusive)."},
			},
			"int",
		),
		"rangeFloat": NewDocstring(
			"Returns a random float in [min, max).",
			[]ParamDoc{
				{"min", "Minimum value (inclusive)."},
				{"max", "Maximum value (exclusive)."},
			},
			"float",
		),
	}

	// Module Methods: HTTP
	BuiltinMethodDocs["http"] = map[string]*DocstringObj{
		"handle": NewDocstring(
			"Registers a handler for the given path pattern. Supports path parameters like /users/:id",
			[]ParamDoc{
				{"path", "URL pattern (e.g. '/users/:id')"},
				{"handler", "Function(req, res) or {GET: fn, POST: fn, ...}"},
			},
			"null",
		),
		"static": NewDocstring(
			"Serves static files from a directory at the given route prefix.",
			[]ParamDoc{
				{"route", "URL prefix (e.g. '/static')"},
				{"directory", "Local directory path (e.g. './public')"},
			},
			"null",
		),
		"listen": NewDocstring(
			"Starts the HTTP server.",
			[]ParamDoc{{"addr", "Address to listen on (e.g. ':8080')"}},
			"null",
		),
	}

	// Module Methods: PyleGame
	BuiltinMethodDocs["pylegame"] = map[string]*DocstringObj{
		"init": NewDocstring(
			"Initializes the game window with the given size and title.",
			[]ParamDoc{
				{"width", "Window width in pixels."},
				{"height", "Window height in pixels."},
				{"title", "Window title string."},
			},
			"null",
		),
		"run": NewDocstring(
			"Starts the game loop. Takes an update function and a draw function.",
			[]ParamDoc{
				{"update_fn", "Function called every tick."},
				{"draw_fn", "Function called every frame (receives screen)."},
			},
			"result<null>",
		),
		"load_image": NewDocstring(
			"Loads an image from the specified file path.",
			[]ParamDoc{{"path", "Path to image file."}},
			"result<UserObject>",
		),
		"draw_image": NewDocstring(
			"Draws an image onto the screen at the specified coordinates.",
			[]ParamDoc{
				{"screen", "The target screen image."},
				{"image", "The image to draw."},
				{"x", "X coordinate."},
				{"y", "Y coordinate."},
			},
			"null",
		),
		"load_font": NewDocstring(
			"Loads a TTF or OTF font file.",
			[]ParamDoc{{"path", "Path to font file."}},
			"result<Font>",
		),
		"draw_text": NewDocstring(
			"Draws text using a specific font and color.",
			[]ParamDoc{
				{"screen", "The target screen image."},
				{"font", "The Font object to use."},
				{"text", "The string to draw."},
				{"x", "X coordinate."},
				{"y", "Y coordinate."},
				{"r, g, b, a", "Optional color components (0-255)."},
			},
			"null",
		),
		"measure_text": NewDocstring(
			"Returns the dimensions of the text if rendered with the given font.",
			[]ParamDoc{
				{"font", "The Font object."},
				{"text", "The text string."},
			},
			"map{w, h}",
		),
		"is_key_pressed": NewDocstring(
			"Returns true if the specified keyboard key is currently held down.",
			[]ParamDoc{{"key", "Key name (e.g. 'SPACE', 'A', 'UP')."}},
			"bool",
		),
		"get_fps": NewDocstring(
			"Returns the current actual frames per second.",
			nil,
			"float",
		),
		"draw_rect": NewDocstring(
			"Draws a filled rectangle using the vector engine.",
			[]ParamDoc{
				{"screen", "The target screen image."},
				{"x", "X position."},
				{"y", "Y position."},
				{"w", "Width."},
				{"h", "Height."},
				{"r, g, b, a", "Color components (0-255)."},
			},
			"null",
		),
		"debug_print": NewDocstring(
			"Draws a quick debug string at the top-left of the screen.",
			[]ParamDoc{
				{"screen", "The screen image."},
				{"text", "The text to print."},
			},
			"null",
		),
	}

	// Module Methods: Json
	BuiltinMethodDocs["json"] = map[string]*DocstringObj{
		"parse":     NewDocstring("Parses a JSON string into a Pyle object (Map or Array).", []ParamDoc{{"text", "JSON string"}}, "Map | Array | String | Number | Boolean | null"),
		"stringify": NewDocstring("Converts a Pyle object into a JSON string.", []ParamDoc{{"value", "Value to stringify"}}, "string"),
	}
}
