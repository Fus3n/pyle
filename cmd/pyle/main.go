package main

import (
	"fmt"
	"io/ioutil"
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
	source, err := ioutil.ReadFile(srcName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading file: %s\n", err)
		os.Exit(1)
	}

	vmerr := pyle.RunScript(vm, srcName, string(source))
	if vmerr != nil {
		if pyleErr, ok := vmerr.(*pyle.PyleError); ok {
			fmt.Fprintln(os.Stderr, pyleErr.ShowSource(string(source)))
		} else {
			fmt.Fprintln(os.Stderr, vmerr.Error())
		}
		os.Exit(1)
	}
}
