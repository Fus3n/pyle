package pyle

import (
	"fmt"
	"strings"
	"unicode"
)

type Lexer struct {
	source   string
	srcName  string
	currIdx  int
	currChar rune
	line     int
	col      int
	tokens   []Token
}

func NewLexer(srcName, source string) *Lexer {
	l := &Lexer{
		source:  source,
		srcName: srcName,
		currIdx: 0,
		line:    1,
		col:     1,
		tokens:  make([]Token, 0),
	}

	if len(source) > 0 {
		l.currChar = rune(source[0])
	}
	return l
}

func (l *Lexer) advance() {
	if l.currChar == '\n' {
		l.line++
		l.col = 0
	}
	l.currIdx++
	if l.currIdx < len(l.source) {
		l.currChar = rune(l.source[l.currIdx])
	} else {
		l.currChar = 0
	}
	l.col++
}

func (l *Lexer) hasChar() bool {
	return l.currChar != 0
}

func (l *Lexer) peek(offset int) rune {
	peekIdx := l.currIdx + offset
	if peekIdx < len(l.source) {
		return rune(l.source[peekIdx])
	}
	return 0
}

func (l *Lexer) getLoc(colStart *int) Loc {
	actualColStart := l.col
	if colStart != nil {
		actualColStart = *colStart
	}

	loc := Loc{Line: l.line, ColStart: actualColStart}
	if colStart != nil {
		colEnd := l.col - 1
		loc.ColEnd = &colEnd
	}
	return loc
}

func (l *Lexer) getToken(kind TokenType, value string, loc Loc) Token {
	return Token{
		Kind: kind,
		Value: value,
		Loc: loc,
		SourceName: l.srcName,
	}
}

func (l *Lexer) addToken(kind TokenType, value string, loc Loc) {
	token := Token{
		Kind:       kind,
		Value:      value,
		Loc:        loc,
		SourceName: l.srcName,
	}
	l.tokens = append(l.tokens, token)
}

func (l *Lexer) createError(msg string, loc Loc) Result[Token] {
	errTok := Token{
		Kind:       TokenError,
		Value:      msg,
		Loc:        loc,
		SourceName: l.srcName,
	}
	return ResErr[Token](NewLexerError(msg, &errTok))
}

func (l *Lexer) skipComment() (skipped bool, err Result[Token]) {
	if l.currChar == '/' && l.peek(1) == '/' {
		l.advance()
		l.advance()
		for l.hasChar() && l.currChar != '\n' {
			l.advance()
		}
		return true, Result[Token]{}
	} else if l.currChar == '/' && l.peek(1) == '*' {
		commentStartLine := l.line
		commentStartCol := l.col
		l.advance()
		l.advance()

		foundCommentEnd := false
		for l.hasChar() {
			if l.currChar == '*' && l.peek(1) == '/' {
				l.advance()
				l.advance()
				foundCommentEnd = true
				break
			}
			l.advance()
		}

		if !foundCommentEnd {
			errLoc := Loc{Line: commentStartLine, ColStart: commentStartCol}
			return true, l.createError("Unterminated comment", errLoc)
		}
		return true, Result[Token]{}
	}
	return false, Result[Token]{}
}

// singleSymbols  map
var singleSymbols = map[rune]TokenType{
	'(': TokenLParen,
	')': TokenRParen,
	'[': TokenLSqBracket,
	']': TokenRSqBracket,
	'{': TokenLCurlyBrace,
	'}': TokenRCurlyBrace,
	';': TokenSemiColon,
	'.': TokenDot,
	',': TokenComma,
	':': TokenColon,
}


func (l *Lexer) Tokenize() ([]Token, Result[Token]) {
	for l.hasChar() {
		if unicode.IsSpace(l.currChar) {
			l.advance()
			continue
		}

		if skipped, err := l.skipComment(); skipped {
			if err.IsErr() {
				return nil, err
			}
			continue
		}

		if tokType, ok := singleSymbols[l.currChar]; ok {
			l.addToken(tokType, string(l.currChar), l.getLoc(nil))
			l.advance()
			continue
		}

		switch l.currChar {
		case '+':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenPlusEquals, "+=", startLoc)
			} else {
				l.addToken(TokenPlus, "+", startLoc)
			}
		case '-':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenMinusEquals, "-=", startLoc)
			} else if l.peek(0) == '>' {
				l.advance()
				l.addToken(TokenArrow, "->", startLoc)
			} else {
				l.addToken(TokenMinus, "-", startLoc)
			}
		case '*':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenMulEquals, "*=", startLoc)
			} else {
				l.addToken(TokenMul, "*", startLoc)
			}
		case '/':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenDivEquals, "/=", startLoc)
			} else {
				l.addToken(TokenDiv, "/", startLoc)
			}
		case '%':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenModEquals, "%=", startLoc)
			} else {
				l.addToken(TokenMod, "%", startLoc)
			}
		case '>':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenGTE, ">=", startLoc)
			} else {
				l.addToken(TokenGT, ">", startLoc)
			}
		case '<':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenLTE, "<=", startLoc)
			} else {
				l.addToken(TokenLT, "<", startLoc)
			}
		case '=':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenEQ, "==", startLoc)
			} else {
				l.addToken(TokenAssign, "=", startLoc)
			}
		case '!':
			startLoc := l.getLoc(nil)
			l.advance()
			if l.peek(0) == '=' {
				l.advance()
				l.addToken(TokenNEQ, "!=", startLoc)
			} else {
				l.addToken(TokenBang, "!", startLoc)
			}
		default:
			if unicode.IsDigit(l.currChar) {
				res := l.parseNumber()
				if res.IsErr() {
					return nil, res
				}
				l.tokens = append(l.tokens, res.Value)		
			} else if l.currChar == '"' || l.currChar == '\'' {
				res := l.parseString()
				if res.IsErr() {
					return nil, res
				}
				l.tokens = append(l.tokens, res.Value)
			} else if unicode.IsLetter(l.currChar) || l.currChar == '_' {
				res := l.parseIdent()
				if res.IsErr() {
					return nil, res
				}
				l.tokens = append(l.tokens, res.Value)
			} else {
				// Should be error
				return nil, l.createError(
					fmt.Sprintf("Unexpected character '%c' at line %d, column %d", l.currChar, l.line, l.col),
					l.getLoc(nil),
				)
			}

		}

	}

	// Add EOF token
	eofToken := Token{
		Kind:       TokenEOF,
		Value:      "",
		Loc:        Loc{Line: l.line, ColStart: l.col},
		SourceName: l.srcName,
	}
	l.tokens = append(l.tokens, eofToken)

	return l.tokens, ResOk(Token{})
}

func (l *Lexer) parseNumber() Result[Token] {
	nums := []rune{}
	startCol := l.col
	dotCount := 0

	for l.hasChar() && (unicode.IsDigit(l.currChar) || l.currChar == '.' || l.currChar == '_') {
		isUnderline := l.currChar == '_'
		isDot := l.currChar == '.'

		if isUnderline {
			if len(nums) == 0 || !unicode.IsDigit(nums[len(nums)-1]) {
				return ResErr[Token](NewLexerError(
					fmt.Sprintf("Invalid number format: '_' must be between digits at line %d, column %d", l.line, l.col),
					&Token{Loc: Loc{Line: l.line, ColStart: l.col, ColEnd: &l.col}},
				))
			}
			l.advance()
			continue
		}

		if isDot {
			if dotCount > 0 {
				return l.createError(
					fmt.Sprintf("Invalid number: multiple decimal points at line %d, columns %d-%d", l.line, startCol, l.col),
					NewLoc(l.line, startCol, &l.col),
				)
			}
			dotCount++
		}

		nums = append(nums, l.currChar)
		l.advance()
	}

	colEnd := l.col - 1
	if len(nums) == 0 {
		colEnd = startCol
	}
	loc := Loc{Line: l.line, ColStart: startCol, ColEnd: &colEnd}
	numStr := string(nums)

	if len(numStr) == 0 {
		return ResErr[Token](NewLexerError("Expected a number", &Token{Loc: loc}))
	}
	if numStr == "." {
		return ResErr[Token](NewLexerError("Invalid number: isolated decimal point", &Token{Loc: loc}))
	}
	if strings.HasSuffix(numStr, "_") {
		return ResErr[Token](NewLexerError("Invalid number: cannot end with '_'", &Token{Loc: loc}))
	}

	tok_type := TokenInt
	if dotCount == 1 {
		tok_type = TokenFloat
		if strings.HasPrefix(numStr, ".") {
			numStr = "0" + numStr
		}
		if strings.HasSuffix(numStr, ".") {
			numStr = numStr + "0"
		}
	}

	return ResOk(l.getToken(tok_type, numStr, loc))
}

func (l *Lexer) parseString() Result[Token] {
	strs := []rune{}
	startCol := l.col
	startLine := l.line
	startQuote := l.currChar

	l.advance()

	for l.hasChar() && l.currChar != startQuote {
		strs = append(strs, l.currChar)
		l.advance()
	}

	if !l.hasChar() || l.currChar != startQuote {
		colEnd := l.col - 1
		if len(strs) == 0 {
			colEnd = l.col
		}
		return l.createError(
			fmt.Sprintf("Unterminated string literal at line %d, column %d", startLine, startCol),
			NewLoc(startLine, startCol, &colEnd),
		)
	}

	l.advance()
	loc := NewLoc(startLine, startCol, &l.col)
	return ResOk(l.getToken(TokenString, string(strs), loc))
}

func (l *Lexer) parseIdent() Result[Token] {
	identChars := []rune{}
	startCol := l.col

	identChars = append(identChars, l.currChar)
	l.advance()

	for l.hasChar() && (isAlnumChar(l.currChar) || l.currChar == '_') {
		identChars = append(identChars, l.currChar)
		l.advance()
	}

	identStr := string(identChars)
	loc := NewLoc(l.line, startCol, &l.col)

	tok_kind := TokenIdent
	if IsKeyword(identStr) {
		tok_kind = TokenKeyword
	}

	return ResOk(l.getToken(tok_kind, identStr, loc))
}
