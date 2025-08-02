package pyle

import (
	"fmt"
	"os"
	"strings"
	"unicode"
)

func RunScript(vm *VM, fileName string) error {
	code, ferr := os.ReadFile(fileName)
	if ferr != nil {
		return ferr
	}

	lexer := NewLexer(fileName, string(code))
	tokens, tokErr := lexer.Tokenize()
	if tokErr.IsErr() {
		return tokErr.Err
	}

	parser := NewParser(tokens)
	ast_block := parser.Parse()
	if ast_block.IsErr() {
		return ast_block.Err
	}

	// Walk(ast_block.Value, WalkFunc(func(node ASTNode) {
	// 	if _, ok := node.(*DotExpr); ok {
	// 		println("FOUND dot")
	// 	}
	// }))

	compiler := NewCompiler()
	bytecodeChunk, err := compiler.Compile(ast_block.Value)
	if err != nil {
		return err
	}

	result := vm.Interpret(bytecodeChunk.Code, bytecodeChunk.Constants)
	if result.IsErr() {
		return result.Err
	}

	return nil
}


func CreateInt(val int64) NumberObj {
	return NumberObj{Value: float64(val), IsInt: true}
}

func CreateFloat(val float64) NumberObj {
	return NumberObj{Value: val, IsInt: false}
}

func CreateString(val string) StringObj {
	return StringObj{Value: val}
}

func CreateNull() NullObj {
	return NullObj{}
}


func isAlnumChar(c rune) bool {
	return unicode.IsLetter(c) || unicode.IsDigit(c)
}

func Index[T comparable](slice []T, val T) int {
    for i, v := range slice {
        if v == val {
            return i
        }
    }
    return -1
}

func FormatHash(h uint32) string {
    return fmt.Sprintf("0x%x", h)
}

func DisassembleBytecode(chunk *BytecodeChunk) string {
	var parts []string

	parts = append(parts, "\n--------- Constants ---------\n")
	if len(chunk.Constants) > 0 {
		for i, constVal := range chunk.Constants {
			parts = append(parts, fmt.Sprintf("%04d: %v (type: %T)\n", i, constVal, constVal))
		}
	} else {
		parts = append(parts, "Constants list is empty.\n")
	}

	parts = append(parts, "\n--------- Disassembled Bytecode ---------\n")
	if len(chunk.Code) > 0 {
		for i, instruction := range chunk.Code {
			opcodeName := instruction.Op.String()
			line := fmt.Sprintf("%04d: %-18s", i, opcodeName)

			if instruction.Operand != nil {
				operandVal := instruction.Operand
				var displayVal interface{}
				if ptr, ok := operandVal.(*int); ok {
					displayVal = *ptr
				} else {
					displayVal = operandVal
				}
				line += fmt.Sprintf(" %-5v", displayVal)

				switch instruction.Op {
				case OpConst, OpDefGlobal, OpGetGlobal, OpSetGlobal:
					switch operandVal := operandVal.(type) {
					case *int:
						if *operandVal >= 0 && *operandVal < len(chunk.Constants) {
							line += fmt.Sprintf(" (%v)", chunk.Constants[*operandVal])
						} else {
							line += " (INVALID CONSTANT INDEX)"
						}

					default:
						line += " (INVALID CONSTANT TYPE)"
					}
				}
			}
			parts = append(parts, line+"\n")
		}
	} else {
		parts = append(parts, "Bytecode chunk is empty.\n")
	}

	return strings.Join(parts, "")
}
