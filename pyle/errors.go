package pyle

import (
	"fmt"
	"strings"
)

type Error interface {
	error
	GetLocation() Loc
}

type ErrorType int
const (
	ErrorRuntime ErrorType = iota
	ErrorLexer
	ErrorParser
)

func (t ErrorType) String() string {
	return []string{
		"RuntimeError",
		"LexerError",
		"ParserError",
	}[t]
}

type PyleError struct {
	Type ErrorType
	Msg string
	Loc Loc
}

func (e *PyleError) Error() string {
	if e.Loc.FileName != "" {
		return fmt.Sprintf("%s: %s at %s:%s", e.Type.String(), e.Msg, e.Loc.FileName, e.Loc.String())
	}
	return fmt.Sprintf("%s: %s", e.Type.String(), e.Msg)
}

func (e *PyleError) GetLocation() Loc {
	return e.Loc
}

func (e *PyleError) ShowSource(source string) string {
	lines := strings.Split(source, "\n")
	if e.Loc.Line > 0 && e.Loc.Line <= len(lines) {
		line := lines[e.Loc.Line-1]
		colEnd := e.Loc.ColEnd
		if colEnd == nil {
			// Default to highlighting a single character if ColEnd is not set
			end := e.Loc.ColStart + 1
			colEnd = &end
		}
		underline := strings.Repeat(" ", e.Loc.ColStart) + strings.Repeat("^", *colEnd)
		return fmt.Sprintf("%s\n%s\n%s", e.Error(), line, underline)
	}
	return e.Error()
}


func NewLexerError(msg string, loc Loc) *PyleError {
	return &PyleError{Type: ErrorLexer, Msg: msg, Loc: loc}
}

func NewParserError(msg string, loc Loc) *PyleError {
	return &PyleError{Type: ErrorParser, Msg: msg, Loc: loc}
}

func NewRuntimeError(msg string, loc Loc) *PyleError {
	return &PyleError{Type: ErrorRuntime, Msg: msg, Loc: loc}
}

type Result[T any] struct {
	Value T
	Err   Error
}

func ResOk[T any](value T) Result[T] {
	return Result[T]{Value: value, Err: nil}
}

func ResErr[T any](err Error) Result[T] {
	return Result[T]{Err: err}
}

func (r Result[T]) IsOk() bool {
	return r.Err == nil
}

func (r Result[T]) IsErr() bool {
	return r.Err != nil
}
