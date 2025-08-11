package main

import (
	"os"
	"pylevm/pyle"
)

func main() {
	vm := pyle.NewVM()
	e := vm.LoadBuiltins()
	if e != nil {
		panic(e)
	}
	srcName := "examples/basic.pyle"
	if len(os.Args) > 1 {
		srcName = os.Args[1]
	}
	vmerr := pyle.RunScript(vm, srcName)
	if vmerr != nil {
		panic(vmerr)
	}
}
