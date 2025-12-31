# The Pyle Programming Language

Pyle is a dynamic programming language implemented in Go. It features a custom bytecode compiler and a stack-based virtual machine.

This project began as an educational exercise to gain a deeper understanding of language design, interpreters, and the internal workings of compilers by writing everything completely **from scratch**. The syntax is influenced by the simplicity and readability of languages like Python and JavaScript.

## Core Features

*   **Variables:** Supports mutable (`let`) and immutable (`const`) variable declarations.
*   **Data Types:** Includes integers, floats, strings, booleans, `null`, arrays, and maps.
*   **Control Flow:** Provides standard `if`/`else` statements, `for...in` loops, `while` loops, and `break`/`continue` statements.
*   **Functions:** Supports first-class functions, closures, and recursion.
*   **Built-in Methods:** Common methods are available on built-in types, such as `.map()` on arrays and `.format()` on strings.

## Code Showcase

Here are a few examples to demonstrate Pyle's syntax.

**Variables and Functions:**

```ts
// A recursive function to calculate Fibonacci numbers
fn fib(n: number) { // optional type annotation/hints
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

const result = fib(10)
echo("Fibonacci of 10 is:", result)
```

**Loops and Methods:**

```ts
// A function to get the ASCII codes for each character in a string
fn getAsciiCodes(str: string) {
    const words = str.split(" ")
    const result = []
    for word in words {
        const wordCodes = []
        for char in array(word) {
            wordCodes.append(asciiCode(char))
        }
        result.append(wordCodes)
    }
    return result
}

echo(getAsciiCodes("Hello Pyle"))
```

## How to Run

To execute a Pyle script, you can run the command-line tool from the root of the project:

```sh
go run ./cmd/pyle/ examples/basic.pyle
```


## Deep Dive

### Syntax

Pyle's syntax draws from Python and JavaScript while keeping things minimal. Blocks are delimited with curly braces, but most semicolons are optional. Variables use `let` for mutable and `const` for immutable declarations.

```ts
let count = 0
const name = "Pyle"
```

Functions are defined with `fn` and can be assigned to variables or passed around as first-class values.

```ts
fn greet(who) {
    echo("Hello, " + who)
}

const sayHi = fn(x) { echo("Hi " + x) }
```

Loops come in two flavors. The `for...in` loop iterates over arrays, strings, ranges, or map iterators. The `while` loop runs as long as a condition holds true.

```ts
for i in 0:10 { echo(i) }          // range from 0 to 9
for i in 0:10:2 { echo(i) }        // range with step of 2
for item in ["a", "b", "c"] { 
    echo(item) 
}

let count = 0
while count < 5 {
    count += 1
}
```

Maps use a JavaScript-like literal syntax. Bare identifiers become string keys, and computed keys go inside brackets.

```ts
const person = {
    name: "Alice",
    ["is_" + "active"]: true,
    [42]: "answer"
}
echo(person.name)
echo(person["is_active"])
```

### Type Hinting

Type hints are optional annotations that can be added to function parameters and help with documentation. They have no runtime effect and are purely informational.

```ts
fn add(a: number, b: number) {
    return a + b
}

fn greet(name: string) {
    echo("Hello, " + name)
}
```

### Built-in Methods

Pyle provides methods on its core types. Learn more in the [docs](https://Fus3n.github.io/pyle/).

```ts
const text = "Hello World"
echo(text.len())                    // 11
echo(text.split(" "))               // ["Hello", "World"]
echo("hi {}".format("there")?)       // "hi there"

const nums = [1, 2, 3]
nums.append(4)
echo(nums.map(fn(x) { return x * 2 }))  // result containing [2, 4, 6, 8]

const m = { a: 1, b: 2 }
for key in m.keys() { echo(key) }
```

### IO

The `os` module provides filesystem operations. You can read and write files, check if paths exist, create directories, and list directory contents.

```ts
os.writeFile("data.txt", "Hello, file!")
const content = os.readFile("data.txt")?
echo(content)

if os.exists("data.txt") {
    os.remove("data.txt")
}

os.mkdir("mydir")
echo(os.listdir("."))

const info = os.stat("somefile.txt")
echo(info.size, info.isDir)
```

User input comes from `scan()`, which displays a prompt and returns the entered line.

```ts
let name = scan("Enter your name: ")!
echo("Hello, " + name)
```

### Error Handling

Pyle uses a result-based error handling system. Functions that can fail return a `result` type which wraps either a value or an error. You can handle these results in several ways.

The `?` operator propagates errors. If the result contains an error, it immediately returns that error from the current function. Otherwise it unwraps the value.

```ts
fn loadData(path) {
    const content = os.readFile(path)?
    return Ok(content.split("\n"))
}
```

The `!` operator (unwrap) extracts the value or panics if there's an error. Use it when you're confident the operation will succeed.

```ts
const age = int(scan("Age: ")!)!
```

You can also deconstruct results into value and error pairs for explicit handling.

```ts
let value, err = int("not a number")
if err != null {
    echo("Conversion failed:", err)
}
```

Result objects also have methods: `.unwrap()` panics on error, `.unwrapOr(default)` returns a fallback value, and `.catch(fn)` lets you handle errors with a callback.

```ts
const num = int("abc").unwrapOr(0)
const data = os.readFile("missing.txt").catch(fn(e) {
    echo("Using default because:", e)
    return Ok("default content")
})
```

## Generating Documentation

To generate HTML documentation for the standard library:

```sh
go run ./cmd/pyledoc/
```