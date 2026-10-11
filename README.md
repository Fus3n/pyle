<p align="center">
  <img src="assets/pyle-logo.png" alt="Pyle logo" width="140">
</p>
<p align="center"><b>Pyle is a fast, lightweight, embeddable dynamic scripting language for C++ 17+.</b></p>


## Overview

Pyle is a dynamic scripting language meant to sit inside a C++ program and drive gameplay logic, tooling, and small scripts without pulling in a heavier runtime. It embeds a stack-based VM that you can execute scripts on, and you can pass data in and out of it and bind your own native functions and types to it.

```js
struct Stack(items) {
    fn _init() { }

    fn push(v) { self.items.append(v) }
    fn pop() { return self.items.pop() }
}

let s = Stack(items: [])
s.push(1)
s.push(2)
printf("{} then {}\n", s.pop(), s.pop()) # 2 then 1
```

## Features

* Simple and familiar syntax
* Dynamic typing
* Functions and recursion
* **Higher-order functions** and **closures**
* **Fixed shape** structs with fields and methods
* Coroutines you can drive yourself, plus `async.run` and `async.all`
* Arrays and maps with indexing and mutation
* Lua-like error handling with `pcall`, `raise`, and `assert`
* Control flow with `if`, `elif`, `else`, `while`, `for`, `break`, and `return`
* Native integration for host functions such as `printf` and `os.time`
* Automatic memory management with garbage collection
* Stack-based virtual machine
* Runnable samples covering the language and the standard library in [`examples/`](examples)


## Building

The build uses [xmake](https://xmake.io). Any compiler it supports works: clang, gcc, and msvc. To pick one explicitly, configure the project and then build:

```sh
xmake f -m release
xmake build pyle
```

Add `--toolchain=clang`, `--toolchain=gcc`, or `--toolchain=msvc` to the configure line if you want to force a specific one rather than letting xmake detect it. `-p` and `-a` set the platform and architecture the same way they do in any xmake project. Leaving off the target name builds everything.

If you would rather drive the build from CMake, or you want clangd to understand the project, `xmake project -k cmake` writes out a `CMakeLists.txt` and `xmake project -k compile_commands` writes out a `compile_commands.json`.

The main targets are `pyle` for the interpreter, `pyle-lsp` for the language server, and `pyle_tests` for the test suite, which you can run with `xmake run pyle_tests`. There are also four embedding examples, `example_basic_embedding`, `example_function_binding`, `example_class_binding`, and `example_async_binding`, each covering one way of wiring the VM into a host program. The C++ sample below is the first of them, so you can read it here and then build it to see the same thing run.


## Embedding

Everything except the library itself is a frontend. The `pyle` interpreter, `pyle-lsp`, and the embedding examples all link against one static library target, `libpyle`, which is the whole language: lexer, parser, compiler, VM, garbage collector, and the core native functions. The heavier dependencies (simdjson, cpp-httplib, mbedtls, argparse, raylib) belong to the `pyle` binary and its modules, not to the library, so embedding does not pull any of them in.

To link it, compile everything under `libpyle/src` as C++17, add `libpyle/include` to your include path, and provide the headers for fmt and unordered_dense. Both of those are header-only, and building in-tree with xmake fetches them for you automatically. `xmake build libpyle` produces the static library; if your own project uses xmake you can point it at this repo and depend on the `libpyle` target directly, and if it uses CMake the generated `CMakeLists.txt` from the Building section already contains it.

## C++ Example

```cpp
#include <iostream>
#include <string>
#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp"

int main() {
    pyle::Pyle interpreter;
    // Register Pyle standard core functions & modules (like print, printf, format, os, etc.)
    pyle::register_core_natives(interpreter.vm);

    std::string code = R"(
        fn calculate_factorial(n) {
            if n <= 1 {
                return 1
            }
            return n * calculate_factorial(n - 1)
        }

        let num = 5
        let result = calculate_factorial(num)
        printf("Factorial of {}, is: {}", num, result)
    )";

    // execute takes: (source_code, disassemble_flag, virtual_filename)
    bool success = interpreter.execute(code, false, "factorial.pyl");
    if (!success) {
        std::cerr << "Script execution failed!\n";
        return 1;
    }
    return 0;
}
```

## Pyle Example

```js
struct Pos(x, y) {}

struct Player(name, health, pos) {
    fn _init(n) {
        self.name = n
        self.health = 100
        self.pos = Pos(x: 0, y: 0)
    }

    fn setHealth(new_health) => self.health = new_health
    fn damageBy(n) => self.health -= n 

    fn distanceFrom(other) {
        return Pos(
            x: self.pos.x - other.x,
            y: self.pos.y - other.y
        )
    }

    fn status() => format("Name: {}\nHealth: {}", self.name, self.health)
}



fn main() {
    let p = Player("John")
    # fixed shape structs so can't do this
    # p.new = 5
    print(p.status())
    p.damageBy(20)
    printf("{} has {} hp left.", p.name, p.health)
}

main()
```

## Language Samples

### Functions

```js
fn fib(n) {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

for i in [5, 8, 10] {
    print(fib(i))
}
```

### Closures

```js
fn make_counter() {
    let x = 0
    return fn() {
        x += 1
        return x
    }
}

let counter = make_counter()
print(counter()) # 1
print(counter()) # 2
print(counter()) # 3
```

### Arrays and loops

```js
let values = [1, 2, 3, 4]

let total = 0
for v in values {
    total += v
}
print(total)
```

### Imports

```js
let os = import("os")
let math = import("math")

print(os.listdir("."))
print(math.floor(3.7))
```

Import another script the same way, using its path. Modules that back onto native code can also be loaded from a compiled `.pyled` file, which is how `raylib` is brought in.

### Coroutines

A coroutine is a function you can suspend and step through yourself:

```js
let gen = Coro(fn() {
    let i = 0
    while i < 3 {
        yield i
        i += 1
    }
})

print(gen.resume()) # 0
print(gen.resume()) # 1
print(gen.resume()) # 2
print(gen.resume()) # nothing left, returns none
print(gen.state())  # dead
```

`async.run` drives one to completion and gives you its result, `async.all` runs several and collects their outcomes, and `waitfor` blocks on a task until it finishes:

```js
fn find_primes(lo, hi) {
    let out = []
    for n in lo..hi {
        let is_prime = true
        for d in 2..n {
            if n % d == 0 {
                is_prime = false
                break
            }
        }
        if is_prime {
            out.append(n)
        }
    }
    return out
}

let results = async.all([
    fn() { return find_primes(2, 50) },
    fn() { return find_primes(50, 100) }
])

for r in results {
    print(r.value)
}
```

### Error handling

Errors travel as values, so a failing call can be caught instead of ending the script:

```js
fn risky() {
    let a = 1
    return a + {}
}

let result = pcall(risky)
if not result.ok {
    print(result.error.type)    # TypeError
    print(result.error.message)
    print_trace(result.error)
}
```

`raise` throws one yourself, and `assert` gives you a quick way to check an invariant:

```js
fn divide(a, b) {
    assert(b != 0, "cannot divide by zero")
    return a / b
}
```

## Editor Support

A VS Code extension lives in `editors/vscode`. It gives you Pyle syntax highlighting plus hover, completion, go to definition, and signature help, all powered by the `pyle-lsp` server built alongside the interpreter. To try it, run `npm install` in that folder and launch it from VS Code with F5, which opens an Extension Development Host.

Once it is running you need to point it at the interpreter, since the extension does not know how you built or where you placed it. Open the command palette and run `Pyle: Select Interpreter Path`, then pick the `pyle` binary. The extension looks for a `std` folder next to the interpreter to find the standard library, and finds the `pyle-lsp` binary there too unless you override it with `Pyle: Select LSP Server Binary`. If you change the interpreter later, `Pyle: Restart LSP Server` reloads it.

You may notice the `std` folder is full of files ending in `.d.pyl`. These are type declaration files, and they are for the editor only. They describe the shapes of structs, the signatures of native functions, and the constants that modules export, which is how the extension knows `rl.InitWindow` takes three arguments or that `import("math")` has a `floor` on it. They exist because modules like `raylib`, `os`, and `color` are bound natively, so there is no readable source for the editor to scan. The interpreter never runs them, and it will refuse to import one, so treating them as ordinary scripts is a mistake.

## Status

Pyle is under active development. The language is pre-1.0, so expect syntax and features to shift between releases.
