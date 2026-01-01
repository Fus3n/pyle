package main

import (
	"fmt"
	"os"

	"github.com/Fus3n/pyle/pyle"

	"github.com/alexflint/go-arg"
)

var args struct {
	Input   string   `arg:"positional"`
	Help    bool     `arg:"-h,--help"`
	Diss  bool     `arg:"-d,--disassemble"`
}


func main() {
	vm := pyle.NewVM()
	e := vm.LoadBuiltins()
	if e != nil {
		panic(e)
	}
	
	p := arg.MustParse(&args)

	if args.Help || args.Input == "" {
		p.WriteHelp(os.Stdout)
		os.Exit(0)
	}

	srcName := args.Input
	source, err := os.ReadFile(srcName)
	if err != nil {
		fmt.Fprintf(os.Stderr, "error reading file: %s\n", err)
		os.Exit(1)
	}

	if args.Diss {
		err :=  pyle.DissassembleAndShow(vm, srcName, string(source))
		if err != nil {
			fmt.Fprintf(os.Stderr, "error dissassembling: %s\n", err)
			os.Exit(1)
		}
		os.Exit(0)
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
