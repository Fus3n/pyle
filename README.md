# Pyle: A "from-scratch" Python-like Language & VM

<p align="center">Dive into the internals of programming languages!</p>

Pyle is a dynamic, interpreted programming language, along with its own bytecode virtual machine, built entirely in Python. It's designed as a learning project, offering a hands-on way to understand how programming languages are tokenized, parsed, compiled into bytecode, and finally executed.

While not intended for production use, Pyle serves as a solid educational tool for anyone curious about language design and virtual machine architecture.

## Why Pyle?

The goal isn't to create the next big programming language, but to demystify the "magic" behind the ones we use every day. By building Pyle from the ground up, without relying on external parsing or compiling libraries, we get to explore:

*   **Lexical Analysis:** How raw code is turned into a stream of tokens.
*   **Parsing:** How tokens are structured into an Abstract Syntax Tree (AST).
*   **Compilation:** How an AST is translated into a lower-level instruction set (bytecode).
*   **Virtual Machines:** How a stack-based VM executes bytecode.
*   **Language Features:** The nuts and bolts of implementing variables, control flow, functions, and more.

## The Pyle Pipeline

When you run a Pyle script (`.pyle` file), it goes through several stages:

1.  **Lexer (Tokenizer):**
    The source code string is scanned character by character. The lexer groups these characters into a sequence of **tokens**. For example, `let x = 10 + 5;` might become: `KEYWORD(let)`, `IDENTIFIER(x)`, `OPERATOR(=)`, `NUMBER(10)`, `OPERATOR(+)`, `NUMBER(5)`, `SEMICOLON`.

2.  **Parser:**
    The stream of tokens from the lexer is fed into the parser. The parser checks if the sequence of tokens forms a valid program according to Pyle's grammar rules. If it's valid, the parser constructs an **Abstract Syntax Tree (AST)**. The AST is a tree-like representation of the code's structure, making it easier for the next stage (the compiler) to understand.

3.  **Compiler:**
    The compiler walks through the AST and translates it into **Pyle Bytecode**. This bytecode is a simpler, machine-like instruction set designed specifically for the Pyle Virtual Machine. Each instruction, or "opcode," tells the VM to perform a specific action, like pushing a value onto the stack, performing an addition, or jumping to a different instruction.

4.  **Pyle Bytecode - A Quick Look:**
    Bytecode is an intermediate representation of your program, more abstract than machine code but lower-level than your source code. It's portable (can run on any Pyle VM) and can be more efficient to interpret than directly walking the AST.

    For example, a line of Pyle like:
    ```pyle
    let a = 10 + 20
    ```
    Might be compiled into something conceptually like this (actual opcodes and operands may vary):
    ```
    OP_CONST 0     // Push constant at index 0 (10) onto the stack
    OP_CONST 1     // Push constant at index 1 (20) onto the stack
    OP_ADD         // Pop two values, add them, push result
    OP_DEF_GLOBAL 2 // Define global variable 'a' (index 2 in constants table) with the value from stack
    ```

5.  **Stack-Based Virtual Machine (VM):**
    The Pyle VM is the heart of the execution. It's a **stack-based** machine, meaning it uses a stack data structure to hold temporary values, arguments for operations, and function call information. The VM reads the bytecode instructions one by one and executes them. For `OP_ADD`, it would pop the top two values from the stack, add them, and push the result back onto the stack.

## Features

Pyle supports a growing set of features:

*   **Variables:**
    ```pyle
    let message = "Hello, Pyle!";
    const PI = 3.14159;
    echo(message);
    echo(PI);
    ```

*   **Data Types:**
    *   Numbers (integers and floats): `let count = 100; let price = 19.99;`
    *   Strings: `let name = "Pyle User";`
    *   Booleans: `let isActive = true; let isDone = false;`
    *   Arrays (Lists): `let numbers = [1, 2, 3, 4]; echo(numbers[0]);`

*   **Operators:**
    *   Arithmetic: `+`, `-`, `*`, `/`, `%`
    *   Logical: `and`, `or`, `not`
    *   Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
    *   Unary: `-` (negation), `not`
    ```pyle
    let sum = 10 + 5 * 2; // 20
    let isGreater = sum > 15; // true
    let isNotActive = not isActive;
    ```

*   **Control Flow:**
    *   `if/else` statements:
        ```pyle
        if score > 90 {
            echo("Grade: A");
        } else if score > 80 {
            echo("Grade: B");
        } else {
            echo("Grade: C or lower");
        }
        ```
    *   `while` loops:
        ```pyle
        let i = 0;
        while i < 3 {
            echo(i);
            i = i + 1;
        }
        ```
    *   `for..in` loops (currently with ranges):
        ```pyle
        for x in 0:5 { // Iterates from 0 up to (but not including) 5
            echo(x);
        }
        for y in 1:10:2 { // Iterates from 1 up to 10, with a step of 2
            echo(y);
        }
        ```
    *   `break` and `continue`:
        ```pyle
        for i in 0:10 {
            if i == 3 {
                continue; // Skip printing 3
            }
            if i == 7 {
                break; // Exit loop when i is 7
            }
            echo(i);
        }
        ```

*   **Functions:**
    *   Definition and calling:
        ```pyle
        fn greet(name) {
            return "Hello, " + name + "!";
        }
        let message = greet("World");
        echo(message);
        ```
    *   Keyword arguments:
        ```pyle
        fn introduce(name, age) {
            echo("My name is", name, "and I am", age, "years old.");
        }
        introduce(name: "Alex", age: 30);
        introduce(age: 25, name: "Jordan"); // Order doesn't matter for keywords
        ```
    *   Return statements (can return any expression, or nothing).

*   **Array Indexing and Assignment:**
    ```pyle
    let my_array = [10, 20, 30];
    echo(my_array[1]); // 20
    my_array[1] = 25;
    echo(my_array[1]); // 25
    ```

*   **Python Module Importing & Attribute Access:**
    You can import and use Python modules and their attributes/functions.
    ```pyle
    const math = importpy("math");
    echo("Pi from Python:", math.pi);
    echo("Square root of 16:", math.sqrt(16));

    const random = importpy("random");
    echo("Random int:", random.randint(1, 100));
    ```

*   **Built-in Functions:**
    *   `echo(...)`: Prints arguments to the console.
    *   `len(iterable)`: Returns the length of an array or string.
    *   `scan(prompt)`: Reads a line of input from the user.
    *   `perf_counter()`: Returns a high-resolution performance counter.
    *   `importpy(module_name)`: Imports a Python module.
    *   `get_attr(object, attribute_name_string)`: Gets an attribute from an object (useful for dynamic access or when attribute names are keywords).

## Getting Started

1.  Clone this repository.
2.  Ensure you have Python installed (Pyle is written in Python).
3.  Run a Pyle script:
    ```bash
    python main.py examples/source.pyle
    ```
    (Replace `examples/source.pyle` with the path to your Pyle file.)

    The `main.py` script will:
    *   Lex, parse, and compile your Pyle code.
    *   Print the disassembled bytecode to the console.
    *   Save the disassembled bytecode to `source.pyled`.
    *   Execute the bytecode using the Pyle VM and print the output.

## Example: Fibonacci

Here's a small example demonstrating functions and loops:

```pyle
fn fib(n) {
    if n <= 1 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

echo("Calculating fib(10):");
let result = fib(10);
echo("Result:", result);

echo("Fibonacci sequence up to 7:");
for i in 0:8 { // Will print fib(0) to fib(7)
    echo("fib(", i, ") =", fib(i));
}
```

## VS Code Extension

A basic [VS Code extension](extension/pyle-language-support) is included in the `extension/` directory, providing:
* Syntax highlighting.
* Autocompletion for keywords, built-in functions, user-defined functions, variables, and Python packages within `importpy("...")`.

## Learning Focus & Future

Pyle is, first and foremost, an educational endeavor. It's a playground for exploring language design concepts. There's always more to learn and implement!

This project might be used as a basis for a future YouTube series exploring these concepts in depth.

---
<p align="center">Happy Pyling!</p>

