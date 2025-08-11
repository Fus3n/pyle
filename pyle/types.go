package pyle

import (
	"fmt"
	"slices"
)

type TokenType int

var KeywordConsts = []string{
	"and", "or", "not", "for", "in", "if", "else", "let",
	"true", "false", "while", "fn", "return", "break", "continue", "const", "null",
}

func IsKeyword(s string) bool {
	return slices.Contains(KeywordConsts, s)
}

func GetAllKeywords() []string {
	return KeywordConsts
}

const (
	TokenInt TokenType = iota
	TokenFloat
	TokenString
	TokenIdent
	TokenKeyword
	TokenDot
	TokenLParen
	TokenRParen
	TokenLBracket
	TokenRBracket
	TokenLCurlyBrace
	TokenRCurlyBrace
	TokenLSqBracket
	TokenRSqBracket
	TokenComma
	TokenPlus
	TokenMinus
	TokenMul
	TokenDiv
	TokenMod
	TokenEOF
	TokenError
	TokenSemiColon
	TokenColon
	TokenGT
	TokenGTE
	TokenLT
	TokenLTE
	TokenEQ
	TokenBang
	TokenNEQ
	TokenAssign
	TokenPlusEquals
	TokenMinusEquals
	TokenMulEquals
	TokenDivEquals
	TokenModEquals
	TokenArrow
)

func (t TokenType) String() string {
	return []string{
		"TokenInt",
		"TokenFloat",
		"TokenString",
		"TokenIdent",
		"TokenKeyword",
		"TokenDot",
		"TokenLParen",
		"TokenRParen",
		"TokenLBracket",
		"TOkenRBracket",
		"TokenLCurlyBrace",
		"TokenRCurlyBrace",
		"TokenLSqBracket",
		"TokenRSqBracket",
		"TokenComma",
		"TokenPlus",
		"TokenMinus",
		"TokenMul",
		"TokenDiv",
		"TokenMod",
		"TokenEOF",
		"TokenError",
		"TokenSemiColon",
		"TokenColon",
		"TokenGT",
		"TokenGTE",
		"TokenLT",
		"TokenLTE",
		"TokenEQ",
		"TokenBang",
		"TokenNEQ",
		"TokenAssign",
		"TokenPlusEquals",
		"TokenMinusEquals",
		"TokenMulEquals",
		"TokenDivEquals",
		"TokenModEquals",
		"TokenArrow",
	}[t]
}

type Loc struct {
	FileName string `json:"fileName"`
	Line     int    `json:"line"`
	ColStart int    `json:"colStart"`
	ColEnd   *int   `json:"colEnd,omitempty"`
}

func NewLoc(fileName string, line, colStart int, colEnd *int) Loc {
	return Loc{
		FileName: fileName,
		Line:     line,
		ColStart: colStart,
		ColEnd:   colEnd,
	}
}

func (l Loc) String() string {
	if l.ColEnd != nil {
		return fmt.Sprintf("%d:%d-%d", l.Line, l.ColStart, *l.ColEnd)
	}
	return fmt.Sprintf("%d:%d", l.Line, l.ColStart)
}

type Token struct {
	Kind  TokenType `json:"kind"`
	Value string    `json:"value"`
	Loc   Loc       `json:"loc"`
}

// new token
func NewToken(kind TokenType, value string, loc Loc, sourceName string) Token {
	return Token{
		Kind:  kind,
		Value: value,
		Loc:   loc,
	}
}
func (t Token) GetFileLoc() string {
	return fmt.Sprintf("%s:%s", t.Loc.FileName, t.Loc.String())
}

func (t Token) IsKeyword(value string) bool {
	return t.Kind == TokenKeyword && t.Value == value
}

func (t Token) String() string {
	return fmt.Sprintf("%s %s", t.Kind, t.Value)
}
