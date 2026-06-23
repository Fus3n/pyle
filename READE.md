# Pyle

Pyle is a fast, lightweight, embeddable dynamic scripting language for C++.

## Overview

Pyle is designed to be simple to read, easy to embed, and practical for small tools, scripting, gameplay logic, and rapid development. It includes first-class functions, closures, structured data types, arrays, native function integration, and automatic memory management.

## Features

* Simple and familiar syntax
* Dynamic typing
* Functions and recursion
* Higher-order functions and closures
* Structs with fields and methods
* Arrays, strings, indexing, and mutation
* Control flow with `if`, `elif`, `while`, and `return`
* Native integration for host functions such as `printf` and `clock`
* Automatic memory management with garbage collection
* Stack-based virtual machine

## Example

```pyle
struct Pos(x, y) {}

struct Player(name, health, pos) {
    fn _init(n) {
        self.name = n
        self.health = 100
        self.pos = Pos(x: 0, y: 0)
    }

    fn setHealth(new_health) {
        self.health = new_health
    }

    fn damageBy(n) {
        self.health -= n
    }

    fn distanceFrom(other) {
        return Pos(
            x: self.pos.x - other.x,
            y: self.pos.y - other.y
        )
    }

    fn status() {
        return format("Name: {}\nHealth: {}", self.name, self.health)
    }
}

fn main() {
    let p = Player("John")
    print(p.status())
    p.damageBy(20)
    printf("{} has {} hp left.", p.name, p.health)
}

main()
```

## Language Samples

### Functions

```pyle
fn fib(n) {
    if n <= 1 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}
```

### Closures

```pyle
fn make_counter() {
    let x = 0
    fn counter() {
        x += 1
        return x
    }
    return counter
}
```

### Arrays and loops

```pyle
fn sum(values) {
    let total = 0
    let i = 0
    while i < values.size() {
        total += values[i]
        i += 1
    }
    return total
}
```

## Status

Pyle is under active development.

## Notes

The syntax and feature set may evolve as the language grows.
