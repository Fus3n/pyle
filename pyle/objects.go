package pyle

import (
	"bytes"
	"encoding/binary"
	"fmt"
	"hash/fnv"
	"hash/maphash"
	"reflect"
	"sort"
	"strconv"
	"strings"
)

// Interfaces

type Iterator interface {
	Object
	Next() (Object, bool)
}

type Object interface {
	GetLocation() Loc
	String() string
	Type() string
	IsTruthy() bool
	Iter() (Iterator, Error)
}

type Hashable interface {
	Hash() uint32
}

type Comparable interface {
	Compare(other Object) (int, error)
}

type AttributeGetter interface {
	GetAttribute(name string) (Object, bool, Error)
}

type DebugInfo struct {
	loc Loc
}

func (o DebugInfo) GetLocation() Loc {
	return o.loc
}

// Documentation Types

type ParamDoc struct {
	Name        string
	Description string
}

type DocstringObj struct {
	DebugInfo
	Description string
	Params      []ParamDoc
	Returns     string
}

func NewDocstring(description string, params []ParamDoc, returns string) *DocstringObj {
	return &DocstringObj{
		DebugInfo:  DebugInfo{},
		Description: description,
		Params:      params,
		Returns:     returns,
	}
}

func (d *DocstringObj) GetAttribute(name string) (Object, bool, Error) {
	switch name {
	case "description":
		return StringObj{Value: d.Description}, true, nil
	case "params":
		paramStrings := make([]Object, len(d.Params))
		for i, p := range d.Params {
			paramStrings[i] = StringObj{Value: fmt.Sprintf("%s: %s", p.Name, p.Description)}
		}
		return &ArrayObj{Elements: paramStrings}, true, nil
	case "returns":
		return StringObj{Value: d.Returns}, true, nil
	}
	return nil, false, nil
}

func (d *DocstringObj) String() string {
	var b strings.Builder
	b.WriteString(d.Description)
	if len(d.Params) > 0 {
		b.WriteString("\n\nParams:\n")
		for _, p := range d.Params {
			b.WriteString(fmt.Sprintf("  %s: %s\n", p.Name, p.Description))
		}
	}
	if d.Returns != "" {
		b.WriteString("\nReturns:\n")
		b.WriteString(fmt.Sprintf("  %s\n", d.Returns))
	}
	return b.String()
}
func (d *DocstringObj) Type() string   { return "docstring" }
func (d *DocstringObj) IsTruthy() bool { return d.Description != "" }
func (d *DocstringObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError("docstring object is not iterable", d.GetLocation())
}

// Core Types

type NumberObj struct {
	DebugInfo
	Value float64
	IsInt bool
}

func (n NumberObj) String() string {
	if n.IsInt {
		return strconv.FormatInt(int64(n.Value), 10)
	}
	return strconv.FormatFloat(n.Value, 'g', -1, 64)
}
func (n NumberObj) Type() string {
	if n.IsInt {
		return "int"
	}
	return "float"
}
func (n NumberObj) IsTruthy() bool { return n.Value != 0 }
func (n NumberObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", n.Type()), n.GetLocation())
}
func (n NumberObj) Hash() uint32 {
	h := fnv.New32a()
	binary.Write(h, binary.LittleEndian, n.Value)
	return h.Sum32()
}
func (n NumberObj) Compare(other Object) (int, error) {
	otherNum, ok := other.(NumberObj)
	if !ok {
		return strings.Compare(n.Type(), other.Type()), nil
	}
	if n.Value < otherNum.Value {
		return -1, nil
	}
	if n.Value > otherNum.Value {
		return 1, nil
	}
	return 0, nil
}

type StringObj struct {
	DebugInfo
	Value string
}

func (s StringObj) GetAttribute(name string) (Object, bool, Error) {
	if methods, ok := BuiltinMethods[s.Type()]; ok {
		if method, exists := methods[name]; exists {
			// The method is bound to the receiver `s`
			boundMethod := &BoundMethodObj{Receiver: s, Method: method}
			return boundMethod, true, nil
		}
	}
	return nil, false, nil
}
func (s StringObj) String() string { return s.Value }
func (s StringObj) Type() string   { return "string" }
func (s StringObj) IsTruthy() bool { return s.Value != "" }
func (s StringObj) Iter() (Iterator, Error) {
	return &StringIteratorObj{Value: s.Value, index: 0}, nil
}
func (s StringObj) Hash() uint32 {
	h := fnv.New32a()
	h.Write([]byte(s.Value))
	return h.Sum32()
}
func (s StringObj) Compare(other Object) (int, error) {
	otherStr, ok := other.(StringObj)
	if !ok {
		return strings.Compare(s.Type(), other.Type()), nil
	}
	return strings.Compare(s.Value, otherStr.Value), nil
}

type BooleanObj struct {
	DebugInfo
	Value bool
}

func (b BooleanObj) String() string { return strconv.FormatBool(b.Value) }
func (b BooleanObj) Type() string   { return "bool" }
func (b BooleanObj) IsTruthy() bool { return b.Value }
func (b BooleanObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", b.Type()), b.GetLocation())
}
func (b BooleanObj) Hash() uint32 {
	h := fnv.New32a()
	if b.Value {
		h.Write([]byte{1})
	} else {
		h.Write([]byte{0})
	}
	return h.Sum32()
}
func (b BooleanObj) Compare(other Object) (int, error) {
	otherBool, ok := other.(BooleanObj)
	if !ok {
		return strings.Compare(b.Type(), other.Type()), nil
	}
	if b.Value == otherBool.Value {
		return 0, nil
	}
	if b.Value {
		return 1, nil
	}
	return -1, nil
}

type NullObj struct{ DebugInfo }

func (n NullObj) String() string { return "null" }
func (n NullObj) Type() string   { return "null" }
func (n NullObj) IsTruthy() bool { return false }
func (n NullObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", n.Type()), n.GetLocation())
}
func (n NullObj) Hash() uint32 {
	return 0
}
func (n NullObj) Compare(other Object) (int, error) {
	_, ok := other.(NullObj)
	if !ok {
		return strings.Compare(n.Type(), other.Type()), nil
	}
	return 0, nil
}

type ErrorObj struct {
	DebugInfo
	Message string
}

func (e ErrorObj) String() string { return fmt.Sprintf("error: %s", e.Message) }
func (e ErrorObj) Type() string   { return "error" }
func (e ErrorObj) IsTruthy() bool { return true }
func (e ErrorObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", e.Type()), e.GetLocation())
}
func (e ErrorObj) Hash() uint32 {
	h := fnv.New32a()
	h.Write([]byte(e.Message))
	return h.Sum32()
}
func (e ErrorObj) Compare(other Object) (int, error) {
	otherErr, ok := other.(ErrorObj)
	if !ok {
		return strings.Compare(e.Type(), other.Type()), nil
	}
	return strings.Compare(e.Message, otherErr.Message), nil
}

func (e ErrorObj) GetAttribute(name string) (Object, bool, Error) {
	if methods, ok := BuiltinMethods[e.Type()]; ok {
		if method, exists := methods[name]; exists {
			boundMethod := &BoundMethodObj{Receiver: e, Method: method}
			return boundMethod, true, nil
		}
	}
	return nil, false, nil
}

// Container Types

type ArrayObj struct {
	DebugInfo
	Elements []Object
}

func (a *ArrayObj) GetAttribute(name string) (Object, bool, Error) {
	if methods, ok := BuiltinMethods[a.Type()]; ok {
		if method, exists := methods[name]; exists {
			boundMethod := &BoundMethodObj{Receiver: a, Method: method}
			return boundMethod, true, nil
		}
	}
	return nil, false, nil
}
func (a *ArrayObj) String() string {
	var elements []string
	for _, e := range a.Elements {
		elements = append(elements, e.String())
	}
	return "[" + strings.Join(elements, ", ") + "]"
}
func (a *ArrayObj) Type() string   { return "array" }
func (a *ArrayObj) IsTruthy() bool { return len(a.Elements) > 0 }
func (a *ArrayObj) Iter() (Iterator, Error) {
	return &ArrayIteratorObj{Array: a, index: 0}, nil
}

type ModuleObj struct {
	DebugInfo
	Name    string
	Methods *MapObj
	Doc     *DocstringObj
}

func NewModule(name string) *ModuleObj {
	return &ModuleObj{
		Name:    name,
		Methods: NewMap(),
		Doc:     nil,
	}
}
func (m *ModuleObj) GetAttribute(name string) (Object, bool, Error) {
	val, found, err := m.Methods.GetStr(name)
	if err != nil {
		return nil, false, NewRuntimeError(err.Error(), m.GetLocation())
	}
	if found {
		return val, true, nil
	}
	if name == "doc" && m.Doc != nil {
		return m.Doc, true, nil
	}
	return NullObj{}, true, nil
}
func (m *ModuleObj) String() string {
	return fmt.Sprintf("<module '%s'>", m.Name)
}
func (m *ModuleObj) Type() string   { return "module" }
func (m *ModuleObj) IsTruthy() bool { return true }
func (m *ModuleObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError("module object is not iterable", m.GetLocation())
}

// HashMap Implementation

type MapPair struct {
	Key   Object
	Value Object
}

type MapObj struct {
	DebugInfo
	Pairs map[uint32][]MapPair
}

func (o *MapObj) GetAttribute(name string) (Object, bool, Error) {
	// First, check for built-in map methods
	if methods, ok := BuiltinMethods[o.Type()]; ok {
		if method, exists := methods[name]; exists {
			boundMethod := &BoundMethodObj{Receiver: o, Method: method}
			return boundMethod, true, nil
		}
	}

	// If not a method, treat as key access
	val, found, err := o.Get(StringObj{Value: name})
	if err != nil {
		return nil, false, NewRuntimeError(err.Error(), o.GetLocation())
	}
	if found {
		return val, true, nil
	}

	return NullObj{}, true, nil // Return null if key not found
}

func NewMap() *MapObj {
	return &MapObj{
		Pairs: make(map[uint32][]MapPair),
	}
}

func (o *MapObj) GetStr(key string) (Object, bool, error) {
	return o.Get(StringObj{Value: key})
}

func (o *MapObj) Get(key Object) (Object, bool, error) {
	hashable, ok := key.(Hashable)
	if !ok {
		return nil, false, fmt.Errorf("type '%s' is not hashable and cannot be a map key", key.Type())
	}
	comparable, ok := key.(Comparable)
	if !ok {
		return nil, false, fmt.Errorf("type '%s' is not comparable and cannot be a map key", key.Type())
	}

	hash := hashable.Hash()
	bucket, found := o.Pairs[hash]
	if !found {
		return nil, false, nil
	}

	if len(bucket) == 1 {
		return bucket[0].Value, true, nil
	}

	for _, pair := range bucket {
		cmp, err := comparable.Compare(pair.Key)
		if err != nil {
			continue
		}
		if cmp == 0 {
			return pair.Value, true, nil
		}
	}

	return nil, false, nil
}

func (o *MapObj) Set(key Object, value Object) error {
	hashable, ok := key.(Hashable)
	if !ok {
		return fmt.Errorf("type '%s' is not hashable and cannot be a map key", key.Type())
	}
	comparable, ok := key.(Comparable)
	if !ok {
		return fmt.Errorf("type '%s' is not comparable and cannot be a map key", key.Type())
	}

	hash := hashable.Hash()
	bucket, _ := o.Pairs[hash]

	for i, pair := range bucket {
		cmp, err := comparable.Compare(pair.Key)
		if err != nil {
			continue
		}
		if cmp == 0 {
			bucket[i].Value = value
			return nil
		}
	}

	bucket = append(bucket, MapPair{Key: key, Value: value})
	o.Pairs[hash] = bucket
	return nil
}

func (o *MapObj) String() string {
	var out bytes.Buffer
	out.WriteString("{")

	var pairs []string
	for _, bucket := range o.Pairs {
		for _, pair := range bucket {
			pairs = append(pairs, fmt.Sprintf("%s: %s", pair.Key.String(), pair.Value.String()))
		}
	}
	sort.Strings(pairs)

	out.WriteString(strings.Join(pairs, ", "))
	out.WriteString("}")
	return out.String()
}

func (o *MapObj) Type() string   { return "map" }
func (o *MapObj) IsTruthy() bool { return len(o.Pairs) > 0 }
func (o *MapObj) Iter() (Iterator, Error) {
	return NewMapIterator(o, MapIteratorModeKeys), nil
}

// Map Iterator

type MapIteratorMode int

const (
	MapIteratorModeKeys MapIteratorMode = iota
	MapIteratorModeValues
	MapIteratorModeItems
)

type MapIteratorObj struct {
	DebugInfo
	TargetMap *MapObj
	hashes    []uint32
	hashIndex int
	pairIndex int
	Mode      MapIteratorMode
}

func NewMapIterator(target *MapObj, mode MapIteratorMode) *MapIteratorObj {
	hashes := make([]uint32, 0, len(target.Pairs))
	for h := range target.Pairs {
		hashes = append(hashes, h)
	}
	sort.Slice(hashes, func(i, j int) bool { return hashes[i] < hashes[j] })

	return &MapIteratorObj{
		TargetMap: target,
		hashes:    hashes,
		hashIndex: 0,
		pairIndex: 0,
		Mode:      mode,
	}
}

func (it *MapIteratorObj) String() string {
	var modeStr string
	switch it.Mode {
	case MapIteratorModeKeys:
		modeStr = "keys"
	case MapIteratorModeValues:
		modeStr = "values"
	case MapIteratorModeItems:
		modeStr = "items"
	}
	return fmt.Sprintf("<map_%s_iterator>", modeStr)
}
func (it *MapIteratorObj) Type() string            { return it.String() }
func (it *MapIteratorObj) IsTruthy() bool          { return true }
func (it *MapIteratorObj) Iter() (Iterator, Error) { return it, nil }
func (it *MapIteratorObj) Next() (Object, bool) {
	for it.hashIndex < len(it.hashes) {
		currentHash := it.hashes[it.hashIndex]
		bucket := it.TargetMap.Pairs[currentHash]

		if it.pairIndex < len(bucket) {
			pair := bucket[it.pairIndex]
			it.pairIndex++

			switch it.Mode {
			case MapIteratorModeKeys:
				return pair.Key, true
			case MapIteratorModeValues:
				return pair.Value, true
			case MapIteratorModeItems:
				return &TupleObj{Elements: []Object{pair.Key, pair.Value}}, true
			}
		}

		it.hashIndex++
		it.pairIndex = 0
	}

	return NullObj{}, false
}

// Other Types

type RangeObj struct {
	DebugInfo
	Start   int
	End     int
	Step    int
	current int
}

func (r *RangeObj) String() string {
	if r.Step == 1 {
		return fmt.Sprintf("range(%d, %d)", r.Start, r.End)
	}
	return fmt.Sprintf("range(%d, %d, %d)", r.Start, r.End, r.Step)
}
func (r *RangeObj) Type() string   { return "range" }
func (r *RangeObj) IsTruthy() bool { return r.Start != r.End }
func (r *RangeObj) Next() (Object, bool) {
	if (r.Step > 0 && r.current >= r.End) || (r.Step < 0 && r.current <= r.End) {
		return NullObj{}, false
	}
	val := r.current
	r.current += r.Step
	return NumberObj{Value: float64(val), IsInt: true}, true
}
func (o *RangeObj) Iter() (Iterator, Error) {
	return o, nil
}
func (o *RangeObj) Hash() uint32 {
	var h maphash.Hash
	h.Write([]byte{2})
	var buf [8]byte
	binary.BigEndian.PutUint64(buf[:], uint64(o.Start))
	h.Write(buf[:])
	binary.BigEndian.PutUint64(buf[:], uint64(o.End))
	h.Write(buf[:])
	binary.BigEndian.PutUint64(buf[:], uint64(o.Step))
	h.Write(buf[:])
	return uint32(h.Sum64())
}

type StringIteratorObj struct {
	DebugInfo
	Value string
	index int
}

func (s *StringIteratorObj) String() string { return "<string_iterator>" }
func (s *StringIteratorObj) Type() string   { return "string_iterator" }
func (s *StringIteratorObj) IsTruthy() bool { return true }
func (s *StringIteratorObj) Next() (Object, bool) {
	if s.index < len(s.Value) {
		char := string(s.Value[s.index])
		s.index++
		return StringObj{Value: char}, true
	}
	return NullObj{}, false
}
func (s *StringIteratorObj) Iter() (Iterator, Error) {
	return s, nil
}

type ArrayIteratorObj struct {
	DebugInfo
	Array *ArrayObj
	index int
}

func (t *ArrayIteratorObj) String() string { return "<array_iterator>" }
func (t *ArrayIteratorObj) Type() string   { return "array_iterator" }
func (t *ArrayIteratorObj) IsTruthy() bool { return true }
func (t *ArrayIteratorObj) Next() (Object, bool) {
	if t.index < len(t.Array.Elements) {
		element := t.Array.Elements[t.index]
		t.index++
		return element, true
	}
	return NullObj{}, false
}
func (o *ArrayIteratorObj) Iter() (Iterator, Error) {
	return o, nil
}

type TupleObj struct {
	DebugInfo
	Elements []Object
}

func (t *TupleObj) String() string {
	var elements []string
	for _, e := range t.Elements {
		elements = append(elements, e.String())
	}
	return "(" + strings.Join(elements, ", ") + ")"
}
func (t *TupleObj) Type() string   { return "tuple" }
func (t *TupleObj) IsTruthy() bool { return len(t.Elements) > 0 }
func (o *TupleObj) Iter() (Iterator, Error) {
	return &TupleIteratorObj{Tuple: o, index: 0}, nil
}
func (t *TupleObj) Hash() uint32 {
	var h uint32 = 0x811c9dc5
	for _, elem := range t.Elements {
		hashable, ok := elem.(Hashable)
		if !ok {
			panic(fmt.Sprintf("Type %s is not hashable, cannot be in a hashable tuple", elem.Type()))
		}
		elemHash := hashable.Hash()
		h ^= elemHash
		h *= 0x01000193
	}
	return h
}


type ResultObject struct {
	DebugInfo
	Value Object
	Error *ErrorObj
}

func (r *ResultObject) String() string {
	if r.Error != nil {
		return fmt.Sprintf("Result(%s, %s)", r.Value.String(), r.Error.String())
	}
	return fmt.Sprintf("Result(%s)", r.Value.String())
}

func (r *ResultObject) Type() string   { return "result" }
func (r *ResultObject) IsTruthy() bool { return r.Error != nil}
func (r *ResultObject) Iter() (Iterator, Error) {
	return &ResultIteratorObj{Result: r, index: 0}, nil
}


func (r *ResultObject) GetAttribute(name string) (Object, bool, Error) {
	// First, check for built-in map methods
	if methods, ok := BuiltinMethods[r.Type()]; ok {
		if method, exists := methods[name]; exists {
			boundMethod := &BoundMethodObj{Receiver: r, Method: method}
			return boundMethod, true, nil
		}
	}

	// TODO: add these to docs or have a better way to add attribute values and their docs.
	switch name {
	case "val":
		return r.Value, true, nil
	case "err":
		if r.Error == nil {
			return NullObj{}, true, nil
		}
		return r.Error, true, nil
	case "ok":
		return BooleanObj{Value: r.Error == nil}, true, nil
	case "isErr":
		return BooleanObj{Value: r.Error != nil}, true, nil
	}

	return NullObj{}, true, nil // Return null if key not found
}


// Weird quirk of result object
type ResultIteratorObj struct {
	DebugInfo
	Result *ResultObject
	index  int
}

func (t *ResultIteratorObj) String() string { return "<result_iterator>" }
func (t *ResultIteratorObj) Type() string   { return "result_iterator" }
func (t *ResultIteratorObj) IsTruthy() bool { return true }
func (t *ResultIteratorObj) Next() (Object, bool) {
	return NullObj{}, false
}
func (o *ResultIteratorObj) Iter() (Iterator, Error) {
	return o, nil
}


type TupleIteratorObj struct {
	DebugInfo
	Tuple *TupleObj
	index int
}

func (t *TupleIteratorObj) String() string { return "<tuple_iterator>" }
func (t *TupleIteratorObj) Type() string   { return "tuple_iterator" }
func (t *TupleIteratorObj) IsTruthy() bool { return true }
func (t *TupleIteratorObj) Next() (Object, bool) {
	if t.index < len(t.Tuple.Elements) {
		element := t.Tuple.Elements[t.index]
		t.index++
		return element, true
	}
	return NullObj{}, false
}
func (o *TupleIteratorObj) Iter() (Iterator, Error) {
	return o, nil
}

type ArgSpec struct {
	Type reflect.Type
	Name string
}

type FunctionMetadata struct {
	Name         string
	Args         []ArgSpec
	Returns      []reflect.Type
	IsVariadic   bool
	FnValue      reflect.Value
	ReturnsError bool
	WantsVM      bool
}

type NativeFuncObj struct {
	DebugInfo
	Name        string
	Arity       int
	Doc         *DocstringObj
	DirectCall  any                                         // Holds a direct, fast-path function pointer. e.g. NativeFunc1, NativeFunc2
	ReflectCall func(vm *VM, args []Object) (Object, Error) // Reflection-based fallback
}

func (f *NativeFuncObj) GetAttribute(name string) (Object, bool, Error) {
	if name == "doc" {
		if f.Doc != nil {
			return f.Doc, true, nil
		}
		return NullObj{}, true, nil
	}
	return nil, false, nil
}
func (f *NativeFuncObj) String() string { return fmt.Sprintf("<native fn %s>", f.Name) }
func (f *NativeFuncObj) Type() string   { return "native_function" }
func (f *NativeFuncObj) IsTruthy() bool { return true }
func (f *NativeFuncObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", f.Type()), f.GetLocation())
}
func (f *NativeFuncObj) Hash() uint32 {
	var ptr uintptr
	if f.DirectCall != nil {
		ptr = reflect.ValueOf(f.DirectCall).Pointer()
	} else if f.ReflectCall != nil {
		ptr = reflect.ValueOf(f.ReflectCall).Pointer()
	}

	h := fnv.New32a()
	binary.Write(h, binary.LittleEndian, ptr)
	return h.Sum32()
}
func (f *NativeFuncObj) Compare(other Object) (int, error) {
	otherFunc, ok := other.(*NativeFuncObj)
	if !ok {
		return strings.Compare(f.Type(), other.Type()), nil
	}

	var ptr1, ptr2 uintptr
	if f.DirectCall != nil {
		ptr1 = reflect.ValueOf(f.DirectCall).Pointer()
	} else if f.ReflectCall != nil {
		ptr1 = reflect.ValueOf(f.ReflectCall).Pointer()
	}

	if otherFunc.DirectCall != nil {
		ptr2 = reflect.ValueOf(otherFunc.DirectCall).Pointer()
	} else if otherFunc.ReflectCall != nil {
		ptr2 = reflect.ValueOf(otherFunc.ReflectCall).Pointer()
	}

	if ptr1 == ptr2 {
		return 0, nil
	}
	if ptr1 < ptr2 {
		return -1, nil
	}
	return 1, nil
}

type FunctionObj struct {
	DebugInfo
	Name         string
	Arity        int
	Doc          *DocstringObj
	StartIP      *int
	CaptureDepth int
}

func (f *FunctionObj) GetAttribute(name string) (Object, bool, Error) {
	if name == "doc" {
		if f.Doc != nil {
			return f.Doc, true, nil
		}
		return NullObj{}, true, nil
	}
	return nil, false, nil
}
func (f *FunctionObj) String() string { return fmt.Sprintf("<fn %s at %p>", f.Name, f) }
func (f *FunctionObj) Type() string   { return "function" }
func (f *FunctionObj) IsTruthy() bool { return true }
func (f *FunctionObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", f.Type()), f.GetLocation())
}
func (f *FunctionObj) Hash() uint32 {
	h := fnv.New32a()
	binary.Write(h, binary.LittleEndian, reflect.ValueOf(f.StartIP).Pointer())
	return h.Sum32()
}
func (f *FunctionObj) Compare(other Object) (int, error) {
	otherFunc, ok := other.(*FunctionObj)
	if !ok {
		return strings.Compare(f.Type(), other.Type()), nil
	}
	if f.StartIP == otherFunc.StartIP {
		return 0, nil
	}
	return 1, nil
}

// ClosureObj represents a function along with captured lexical environments.
// Captured holds references to environment maps from inner-most to outer-most.
type ClosureObj struct {
	DebugInfo
	Function *FunctionObj
	Captured []map[string]*Variable
}

func (c *ClosureObj) String() string { return fmt.Sprintf("<closure %s>", c.Function.Name) }
func (c *ClosureObj) Type() string   { return "closure" }
func (c *ClosureObj) IsTruthy() bool { return true }
func (c *ClosureObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", c.Type()), c.GetLocation())
}

type BoundMethodObj struct {
	DebugInfo
	Receiver Object
	Method   Object
}

func (b *BoundMethodObj) GetAttribute(name string) (Object, bool, Error) {
	if getter, ok := b.Method.(AttributeGetter); ok {
		return getter.GetAttribute(name)
	}
	return nil, false, nil
}

func (b *BoundMethodObj) String() string {
	return fmt.Sprintf("<bound method %s of %s>", b.Method.String(), b.Receiver.String())
}
func (b *BoundMethodObj) Type() string   { return "bound_method" }
func (b *BoundMethodObj) IsTruthy() bool { return true }
func (b *BoundMethodObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", b.Type()), b.GetLocation())
}

type KwargsObj struct {
	DebugInfo
	Value map[string]Object
}

func (k *KwargsObj) String() string { return fmt.Sprintf("kwargs(%v)", k.Value) }
func (k *KwargsObj) Type() string   { return "kwargs" }
func (k *KwargsObj) IsTruthy() bool { return true }
func (k *KwargsObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError(fmt.Sprintf("object of type '%s' is not iterable", k.Type()), k.GetLocation())
}
