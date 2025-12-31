# TODO: Completed is marked with x

## Language Features
- [x] Variable let and const support
- [x] `String` type
- [x] `Number` type (float64, int64)
- [x] `Boolean` type  
- [ ] `Struct` type
- [ ] `Enum` type
- [x] `Map` type
- [x] `Null` type
- [x]  Dynamic `Array` type
- [x] `While` loop
- [x] `For` in loop
- [x] `break` inside for and whole loop
- [x] `continue` inside for and while loop
- [x] Hashable types
- [ ] Save errors instead of returing so can print stack trace
- [x] Value unpacking into variable, multiple return types/Tuple unpacking
- [x] Design how should language acess object attributes or methods
- [x] Add method support for builtin types
- [x] For Comparable types create new comparable interface and streamline those process like equals, greater tahn function with functions
- [ ] For imports, return a pyle object from Go's side with values being functions
- [ ] Proper Structure to hold modules, importing file support
- [x] Optional type hints support

## Interpreter
- [ ] Apply Constant Folding Optimization
- [ ] Apply Instruction Fusing Optimization
- [ ] Apply Inline Caching Optimization
- [ ] Apply Peephole Optimization
- [x] Apply Faster Scope Resolution Optimization

## Compiler
- [ ] Add AST pass to optimize AST before bytecode generation
- [ ] Add a separate pass for bytecode optimization

## Known Bugs
- [x] Lambda/Closure function does not capture scope variables
- [x] Object does not take token, which is needed for printing proper error msg

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
- [x-] Proper Error printing for runtime errors
- [ ] Use Error map in VM
- [ ] Fix places where file name and location is not printed with error message

## LSP
- [-] LSP support (Broken)
- [ ] Fix user defined variables not showing
- [ ] Doc string for builtin functions
- [ ] Doc string for user defined functions