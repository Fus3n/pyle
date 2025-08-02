package pyle

import "fmt"

type Error interface {
	error
	GetToken() *Token
}

type InterpreterError struct {
	Type  string // Add this field
	Msg   string
	Token *Token
}

func (e *InterpreterError) Error() string {
	if e.Token != nil {
		return fmt.Sprintf("%T: %s at %s", e, e.Msg, e.Token.GetFileLoc())
	}
	return fmt.Sprintf("%T: %s", e, e.Msg)
}

func (e *InterpreterError) GetToken() *Token {
	return e.Token
}

type LexerError struct {
	*InterpreterError
}

func NewLexerError(msg string, token *Token) *LexerError {
	return &LexerError{
		InterpreterError: &InterpreterError{Msg: msg, Token: token},
	}
}

type ParserError struct {
	*InterpreterError
}

func NewParserError(msg string, token *Token) *ParserError {
	return &ParserError{
		InterpreterError: &InterpreterError{Msg: msg, Token: token},
	}
}

type RuntimeError struct {
	*InterpreterError
}

func NewRuntimeError(msg string, token *Token) *RuntimeError {
	return &RuntimeError{
		InterpreterError: &InterpreterError{Msg: msg, Token: token},
	}
}

type Result[T any] struct {
	Value T
	Err Error
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

func (r Result[T]) Unwrap() (T, error) {
	if r.IsErr() {
		var zero T
		return zero, r.Err
	}
	return r.Value, nil
}