package main

import (
	"os"
	"pylevm/pyle"
)

func main() {
	// disassmbled := pyle.DisassembleBytecode(bytecodeChunk)
	// fmt.Println(disassmbled)
	// // save
	// err = os.WriteFile("test.pyled", []byte(disassmbled), 0644)
	// if err != nil {
	// 	panic(err)
	// }

	vm := pyle.NewVM()
	e := vm.LoadBuiltins()
	if e != nil {
		panic(e)
	}
	srcName := "examples/basic.pyle"
	if len(os.Args) > 1 {
		srcName = os.Args[1]
	}
	err := pyle.RunScript(vm, srcName)
	if err != nil {
		panic(err)
	}
}