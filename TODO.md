# TODO: Completed is marked with x

## Language Features
- [x] Variable let and const support
- [x] `String` type
- [x] `Number` type (float64, int64)
- [x] `Boolean` type  
- [x] `Object` type  
- [ ] `Map` type
- [x] `Null` type
- [x]  Dynamic `Array` type
- [x] `While` loop
- [x] `For` in loop
- [x] `break` inside for and whole loop
- [x] `continue` inside for and while loop
- [x] Hashable types
- [ ] Save errors instead of returing so can print stack trace
- [ ] Value unpacking into variable, multiple return types/Tuple unpacking
- [x] Design how should language acess object attributes or methods
- [x] Add method support for builtin types
- [ ] For Comparable types create new comparable interface and streamline those process like equals, greater tahn function with functions
- [ ] For imports return a pyle object from Go's side with values being functions

## Interpreter
- [ ] Apply Constant Folding Optimization
- [ ] Apply Instruction Fusing Optimization
- [ ] Apply Inline Caching Optimization
- [ ] Apply Peephole Optimization
- [ ] Apply Faster Scope Resolution Optimization

## Compiler
- [ ] Add AST pass to optimize AST before bytecode generation
- [ ] Add a separate pass for bytecode optimization

## Known Bugs
- [ ] Lambda/Closure function does not capture scope variables
- [ ] Object does not take token, which is needed for printing proper error msg

## Operators
- [x] All binary operators support
- [x] Unary operator `(-, not)` support
- [x] `"+"` binary op support for string
- [x] Logical operators (and, or)
- [x] Comparision Operators `(==, !=, >, >=, <, <=)`
- [x] Compound Operators `(+=, -=, *=, /=, %=)`

## Array Operations
- [x] Array & String indexing
- [x] Array index assign
- [ ] Range/slice support on index get
- [ ] Range/slice suppport on index assign, to extend

## Function Support
- [x] Native function calling support & Builtins
- [x] Pyle function creation support
- [x] Pyle function calling support

## Map System
- [x] Supported computed values as object `keys`
- [x] Dot operator `get` support for attributes
- [x] Object attributes/methods system
- [x] Object this/self support

## Error
- [ ] Stack trace
- [ ] Proper Error printing for runtime errors
- [ ] Use Error map in VM
- [ ] Fix places where file name and location is not printed with error message

## LSP
- [x] LSP support (Broken)
- [ ] Fix user defined variables not showing
- [ ] Doc string for builtin functions
- [ ] Doc string for user defined functions