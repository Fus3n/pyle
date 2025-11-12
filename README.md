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

### Type hinting

### Built-in Methods

### IO

### Error Handling

