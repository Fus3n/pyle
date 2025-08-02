package pyle

import (
	"fmt"
)

type TokenType int

var KeywordConsts = []string{
	"and", "or", "not", "for", "in", "if", "else", "let",
	"true", "false", "while", "fn", "return", "break", "continue", "const", "null",
}

func IsKeyword(s string) bool {
	for _, keyword := range KeywordConsts {
		if keyword == s {
			return true
		}
	}
	return false
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
	}[t]
}

type Loc struct {
	Line     int  `json:"line"`
	ColStart int  `json:"colStart"`
	ColEnd   *int `json:"colEnd,omitempty"`
}

func NewLoc(line, colStart int, colEnd *int) Loc {
	return Loc{
		Line: line,
		ColStart: colStart,
		ColEnd: colEnd,
	}
}

func (l Loc) String() string {
	if l.ColEnd != nil {
		return fmt.Sprintf("%d:%d-%d", l.Line, l.ColStart, *l.ColEnd)
	}
	return fmt.Sprintf("%d:%d", l.Line, l.ColStart)
}

type Token struct {
	Kind       TokenType `json:"kind"`
	Value      string 	 `json:"value"`
	Loc        Loc		 `json:"loc"`
	SourceName string	 `json:"sourceName"`
}

// new token
func NewToken(kind TokenType, value string, loc Loc, sourceName string) Token {
	return Token{
		Kind: kind,
		Value: value,
		Loc: loc,
		SourceName: sourceName,
	}
}
func (t Token) GetFileLoc() string {
	return fmt.Sprintf("%s:%s", t.SourceName, t.Loc.String())
}

func (t Token) IsKeyword(value string) bool {
	return t.Kind == TokenKeyword && t.Value == value
}

func (t Token) String() string {
	return fmt.Sprintf("%s %s", t.Kind, t.Value)
}