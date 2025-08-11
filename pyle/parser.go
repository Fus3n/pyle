package pyle

import (
	"fmt"
	"strconv"
)

type Parser struct {
	tokens  []Token
	currIdx int
	srcName string
}

func NewParser(tokens []Token) *Parser {
	p := &Parser{
		tokens:  tokens,
		currIdx: 0,
	}
	if len(tokens) > 0 {
		p.srcName = tokens[0].Loc.FileName
	}
	return p
}

func (p *Parser) Parse() Result[*Block] {
	programBlock := &Block{Statements: make([]ASTNode, 0)}

	for !p.isAtEnd() {
		stmtResult := p.statement()
		if stmtResult.IsErr() {
			return ResErr[*Block](stmtResult.Err)
		}
		programBlock.Statements = append(programBlock.Statements, stmtResult.Value)

		// semicolon consumption
		if p.check(TokenSemiColon) {
			p.advance()
			continue
		}
	}

	return ResOk(programBlock)
}

// utils

// Check and advance the given token kind if it matches current, return Error with given message otherwise
func (p *Parser) consume(kind TokenType, msg string) Result[*Token] {
	if p.check(kind) {
		tok := p.advance()
		return ResOk(tok)
	}

	return ResErr[*Token](NewParserError(msg, p.current()))
}

// Match and advance if matched otherwise return false without advancing
func (p *Parser) match(types ...TokenType) bool {
	for _, t := range types {
		if p.check(t) {
			p.advance()
			return true
		}
	}
	return false
}

// Match keyword and advance if matched otherwise return false without advancing
func (p *Parser) matchKeyword(keyword string) bool {
	if p.check(TokenKeyword) && p.current().Value == keyword {
		p.advance()
		return true
	}
	return false
}

// check if current token kind is given kind or not without advancing
func (p *Parser) check(kind TokenType) bool {
	if p.isAtEnd() {
		return false
	}
	return p.current().Kind == kind
}

// advance token to next token and return the advanced token
func (p *Parser) advance() *Token {
	if !p.isAtEnd() {
		p.currIdx++
	}
	return p.previous()
}

// Checks if current token is TokenEOF or not, returns true if it is
func (p *Parser) isAtEnd() bool {
	return p.current().Kind == TokenEOF
}

// get current token without advancing
func (p *Parser) current() *Token {
	return &p.tokens[p.currIdx]
}

func (p *Parser) peekAt(offset int) *Token {
	index := p.currIdx + offset
	if index >= len(p.tokens) {
		return &p.tokens[len(p.tokens)-1] // Return EOF token if out of bounds
	}
	return &p.tokens[index]
}

// return previous token
func (p *Parser) previous() *Token {
	return &p.tokens[p.currIdx-1]
}

// grammar

func (p *Parser) statement() Result[ASTNode] {
	if p.match(TokenKeyword) {
		switch p.previous().Value {
		case "let", "const":
			declResult := p.variableDeclaration()
			if declResult.IsErr() {
				// Cast the error to the expected return type Result[ASTNode]
				return ResErr[ASTNode](declResult.Err)
			}
			// The value (a Stmt) is a valid ASTNode, so we can return it directly.
			return ResOk[ASTNode](declResult.Value)
		case "while":
			whileRes := p.whileStatement()
			if whileRes.IsErr() {
				return ResErr[ASTNode](whileRes.Err)
			}
			return ResOk[ASTNode](whileRes.Value)
		case "for":
			forRes := p.forInStatement()
			if forRes.IsErr() {
				return ResErr[ASTNode](forRes.Err)
			}
			return ResOk[ASTNode](forRes.Value)
		case "fn":
			fnRes := p.functionDefinition()
			if fnRes.IsErr() {
				return ResErr[ASTNode](fnRes.Err)
			}
			return ResOk[ASTNode](fnRes.Value)
		case "return":
			returnRes := p.returnStatement()
			if returnRes.IsErr() {
				return ResErr[ASTNode](returnRes.Err)
			}
			return ResOk[ASTNode](returnRes.Value)
		case "break":
			breakRes := p.breakStatement()
			if breakRes.IsErr() {
				return ResErr[ASTNode](breakRes.Err)
			}
			return ResOk[ASTNode](breakRes.Value)
		case "continue":
			continueRes := p.continueStatement()
			if continueRes.IsErr() {
				return ResErr[ASTNode](continueRes.Err)
			}
			return ResOk[ASTNode](continueRes.Value)
		case "if":
			ifRes := p.ifStatement()
			if ifRes.IsErr() {
				return ResErr[ASTNode](ifRes.Err)
			}
			return ResOk[ASTNode](ifRes.Value)
		}
	} else if p.match(TokenLCurlyBrace) {
		blockResult := p.block()
		if blockResult.IsErr() {
			return ResErr[ASTNode](blockResult.Err)
		}
		// The value (*Block) is a valid ASTNode.
		return ResOk[ASTNode](blockResult.Value)
	}

	if p.check(TokenIdent) {
		peeked := p.peekAt(1)
		if peeked.Kind == TokenAssign {
			varAssignRes := p.variableAssignment()
			if varAssignRes.IsErr() {
				return ResErr[ASTNode](varAssignRes.Err)
			}
			return ResOk[ASTNode](varAssignRes.Value)
		} else if peeked.Kind >= TokenPlusEquals && peeked.Kind <= TokenModEquals {
			compoundAssignRes := p.compoundAssignment()
			if compoundAssignRes.IsErr() {
				return ResErr[ASTNode](compoundAssignRes.Err)
			}
			return ResOk[ASTNode](compoundAssignRes.Value)
		}
	}

	exprResult := p.expression()
	if exprResult.IsErr() {
		return ResErr[ASTNode](exprResult.Err)
	}
	exprNode := exprResult.Value

	// index assign
	if indexExpr, ok := exprNode.(*IndexExpr); ok {
		if p.match(TokenAssign) {
			valRes := p.expression()
			if valRes.IsErr() {
				return ResErr[ASTNode](valRes.Err)
			}
			return ResOk[ASTNode](&IndexAssignStmt{
				Token:      indexExpr.Token,
				Collection: indexExpr.Collection,
				Index:      indexExpr.Index,
				Value:      valRes.Value,
			})
		}
	} else if dotExpr, ok := exprNode.(*DotExpr); ok {
		if p.match(TokenAssign) {
			valRes := p.expression()
			if valRes.IsErr() {
				return ResErr[ASTNode](valRes.Err)
			}
			return ResOk[ASTNode](&SetAttrStmt{
				Token: dotExpr.Token,
				Obj:   dotExpr.Obj,
				Attr:  &dotExpr.Attr,
				Value: valRes.Value,
			})
		}
	}

	p.match(TokenSemiColon)

	return ResOk[ASTNode](exprResult.Value)
}

// Opening bracket { must be handled and advanced first.
func (p *Parser) block() Result[*Block] {
	blockTok := p.previous()
	statements := make([]ASTNode, 0)

	for !p.check(TokenRCurlyBrace) && !p.isAtEnd() {
		stmtRes := p.statement()
		if stmtRes.IsErr() {
			return ResErr[*Block](stmtRes.Err)
		}
		statements = append(statements, stmtRes.Value)

	}

	tokRes := p.consume(TokenRCurlyBrace, "Expected '}' after block")
	if tokRes.IsErr() {
		return ResErr[*Block](tokRes.Err)
	}
	endTok := tokRes.Value

	return ResOk(&Block{Token: blockTok, Statements: statements, EndToken: endTok})
}

func (p *Parser) variableDeclaration() Result[Stmt] {
	keywordTok := p.previous()
	isConst := keywordTok.Value == "const"

	var varNames []*Token

	nameRes := p.consume(TokenIdent, "Expected variable name")
	if nameRes.IsErr() {
		return ResErr[Stmt](nameRes.Err)
	}

	varNames = append(varNames, nameRes.Value)

	for p.match(TokenComma) {
		nameRes := p.consume(TokenIdent, "Expected variable name")
		if nameRes.IsErr() {
			return ResErr[Stmt](nameRes.Err)
		}
		varNames = append(varNames, nameRes.Value)
	}

	var typeHint Expr
	if p.match(TokenColon) {
		typeRes := p.expression()
		if typeRes.IsErr() {
			return ResErr[Stmt](typeRes.Err)
		}
		typeHint = typeRes.Value
	}

	var initializers []Expr

	if p.match(TokenAssign) {
		// Parse one or more expressions separated by commas
		exprRes := p.expression()
		if exprRes.IsErr() {
			return ResErr[Stmt](exprRes.Err)
		}
		initializers = append(initializers, exprRes.Value)
		for p.match(TokenComma) {
			// stop if we see a semicolon or newline-like end; otherwise expect another expression
			if p.check(TokenSemiColon) || p.check(TokenRCurlyBrace) || p.check(TokenEOF) {
				break
			}
			exprRes := p.expression()
			if exprRes.IsErr() {
				return ResErr[Stmt](exprRes.Err)
			}
			initializers = append(initializers, exprRes.Value)
		}
	}

	// Optional semicolon.
	p.match(TokenSemiColon)

	return ResOk[Stmt](&VarDeclareStmt{
		Token:        keywordTok,
		Names:        varNames,
		Initializers: initializers,
		IsConst:      isConst,
		Type:         typeHint,
	})
}

func (p *Parser) variableAssignment() Result[Stmt] {
	varNameTok := p.advance()
	p.advance() // consume '='

	exprRes := p.expression()
	if exprRes.IsErr() {
		return ResErr[Stmt](exprRes.Err)
	}

	// Optional semicolon.
	p.match(TokenSemiColon)

	return ResOk[Stmt](&AssignStmt{
		Token: varNameTok,
		Name:  varNameTok,
		Value: exprRes.Value,
	})
}

func (p *Parser) compoundAssignment() Result[Stmt] {
	varNameTok := p.advance()
	opTok := p.advance()

	exprRes := p.expression()
	if exprRes.IsErr() {
		return ResErr[Stmt](exprRes.Err)
	}

	p.match(TokenSemiColon)

	return ResOk[Stmt](&CompoundAssignStmt{
		Token: varNameTok,
		Name:  varNameTok,
		Op:    opTok,
		Value: exprRes.Value,
	})
}

func (p *Parser) expression() Result[Expr] {
	return p.logical_or()
}

func (p *Parser) logical_or() Result[Expr] {
	exprRes := p.ranges()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.matchKeyword("or") {
		op := p.previous()
		rightRes := p.ranges()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &LogicalOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) ranges() Result[Expr] {
	leftRes := p.logical_and()
	if leftRes.IsErr() {
		return leftRes
	}

	left := leftRes.Value
	startTok := p.previous()

	if p.match(TokenColon) {
		endExprRes := p.logical_and()
		if endExprRes.IsErr() {
			return endExprRes
		}
		endExpr := endExprRes.Value

		var stepExpr *Expr = nil
		if p.match(TokenColon) {
			stepExprRes := p.logical_and()
			if stepExprRes.IsErr() {
				return stepExprRes
			}
			stepExpr = &stepExprRes.Value
		}

		return ResOk(Expr(
			&RangeSpecifier{
				Token: startTok,
				Start: left,
				End:   endExpr,
				Step:  stepExpr,
			},
		))
	}

	return ResOk(left)
}

func (p *Parser) logical_and() Result[Expr] {
	exprRes := p.equality()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.matchKeyword("and") {
		op := p.previous()
		rightRes := p.equality()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &LogicalOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) equality() Result[Expr] {
	exprRes := p.comparison()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.match(TokenNEQ, TokenEQ) {
		op := p.previous()
		rightRes := p.comparison()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &ComparisonOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) comparison() Result[Expr] {
	exprRes := p.term()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.match(TokenGT, TokenGTE, TokenLT, TokenLTE) {
		op := p.previous()
		rightRes := p.term()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &ComparisonOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) term() Result[Expr] {
	exprRes := p.factor()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.match(TokenPlus, TokenMinus) {
		op := p.previous()
		rightRes := p.factor()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &BinaryOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) factor() Result[Expr] {
	exprRes := p.unary()
	if exprRes.IsErr() {
		return exprRes
	}

	expr := exprRes.Value

	for p.match(TokenMul, TokenDiv, TokenMod) {
		op := p.previous()
		rightRes := p.unary()
		if rightRes.IsErr() {
			return rightRes
		}
		expr = &BinaryOp{
			Token: op,
			Left:  expr,
			Op:    op,
			Right: rightRes.Value,
		}
	}

	return ResOk(expr)
}

func (p *Parser) unary() Result[Expr] {
	if p.match(TokenMinus) || p.matchKeyword("not") {
		op := p.previous()
		valueRes := p.unary()
		if valueRes.IsErr() {
			return valueRes
		}
		unOp := UnaryOp{
			Token:   op,
			Op:      op,
			Operand: valueRes.Value,
		}

		return ResOk(Expr(&unOp))
	}

	return p.call()
}

func (p *Parser) call() Result[Expr] {
	exprRes := p.primary()
	if exprRes.IsErr() {
		return exprRes
	}

	currentExpr := exprRes.Value

	for {
		if p.match(TokenLParen) {
			lParenToken := p.previous()
			arguments := []Expr{}

			if !p.check(TokenRParen) {
				for {
					argRes := p.expression()
					if argRes.IsErr() {
						return argRes
					}
					arguments = append(arguments, argRes.Value)

					if !p.match(TokenComma) {
						break
					}
					if p.check(TokenRParen) {
						break
					}
				}
			}

			rParenRes := p.consume(TokenRParen, "Expected ')' after arguments.")
			if rParenRes.IsErr() {
				return ResErr[Expr](rParenRes.Err)
			}

			currentExpr = &CallExpr{
				Token:     lParenToken,
				Callee:    currentExpr,
				Arguments: arguments,
			}
		} else if p.match(TokenLSqBracket) {
			bracTok := p.previous()
			indexExprRes := p.expression()
			if indexExprRes.IsErr() {
				return indexExprRes
			}
			if !p.match(TokenRSqBracket) {
				return ResErr[Expr](NewParserError("Expected ']' after index expression.", p.current()))
			}
			currentExpr = &IndexExpr{
				Token:      bracTok,
				Collection: currentExpr,
				Index:      indexExprRes.Value,
			}
		} else if p.match(TokenDot) {
			dotTok := p.previous()
			attrTok := p.advance()
			currentExpr = &DotExpr{
				Token: dotTok,
				Obj:   currentExpr,
				Attr:  *attrTok,
			}
		} else {
			break
		}
	}

	return ResOk(currentExpr)
}

func (p *Parser) primary() Result[Expr] {
	switch p.current().Kind {
	case TokenIdent:
		p.advance()
		return ResOk[Expr](&VariableExpr{Token: p.previous(), Name: p.previous()})
	case TokenInt, TokenFloat:
		p.advance()
		prev := p.previous()
		val, _ := strconv.ParseFloat(prev.Value, 64)
		// possibly return the error later on
		return ResOk[Expr](&NumberExpr{Token: prev, Value: val, IsInt: prev.Kind == TokenInt})
	case TokenString:
		p.advance()
		return ResOk[Expr](&StringExpr{Token: p.previous(), Value: p.previous().Value})
	case TokenKeyword:
		switch p.current().Value {
		case "true", "false":
			tok := p.advance()
			return ResOk[Expr](&BooleanExpr{Token: tok, Value: tok.Value == "true"})
		case "null":
			return ResOk[Expr](&NullExpr{Token: p.advance()})
		case "fn":
			return p.functionExpr()
		}
	case TokenLSqBracket:
		p.advance()
		return p.array_literal()
	case TokenLCurlyBrace:
		p.advance()
		return p.map_literal()
	case TokenLParen:
		p.advance()
		exprRes := p.expression()
		if exprRes.IsErr() {
			return exprRes
		}
		consErr := p.consume(TokenRParen, "Expected ')' after expression.")
		if consErr.IsErr() {
			return ResErr[Expr](consErr.Err)
		}
		return ResOk(exprRes.Value)
	}

	return ResErr[Expr](
		NewParserError(
			fmt.Sprintf("Expected expression, but got %s instead.", p.current().Value),
			p.current(),
		),
	)
}

func (p *Parser) functionExpr() Result[Expr] {
	fnTok := p.advance() // token wasnt advance so using advance

	if !p.match(TokenLParen) {
		return ResErr[Expr](NewParserError("Expected '(' after 'fn' keyword.", p.current()))
	}
	params := []*Parameter{}
	if !p.check(TokenRParen) {
		for {
			paramNameRes := p.consume(TokenIdent, "Expected parameter name.")
			if paramNameRes.IsErr() {
				return ResErr[Expr](paramNameRes.Err)
			}
			paramName := paramNameRes.Value

			var paramType Expr = nil
			if p.match(TokenColon) {
				typeRes := p.expression()
				if typeRes.IsErr() {
					return ResErr[Expr](typeRes.Err)
				}
				paramType = typeRes.Value
			}

			params = append(params, &Parameter{Name: paramName, Type: paramType})

			if !p.match(TokenComma) {
				break
			}
			if p.check(TokenRParen) {
				break
			}
		}
	}

	if !p.match(TokenRParen) {
		return ResErr[Expr](NewParserError("Expected ')' after parameters.", p.current()))
	}

	var returnType Expr = nil
	if p.match(TokenArrow) {
		typeRes := p.expression()
		if typeRes.IsErr() {
			return ResErr[Expr](typeRes.Err)
		}
		returnType = typeRes.Value
	}

	if !p.match(TokenLCurlyBrace) {
		return ResErr[Expr](NewParserError("Expected '{' after function parameters.", p.current()))
	}

	bodyBlockRes := p.block()
	if bodyBlockRes.IsErr() {
		return ResErr[Expr](bodyBlockRes.Err)
	}
	bodyBlock := bodyBlockRes.Value

	return ResOk[Expr](&FunctionExpr{
		Token:      fnTok,
		Params:     params,
		Body:       bodyBlock,
		ReturnType: returnType,
	})
}

func (p *Parser) array_literal() Result[Expr] {
	if p.match(TokenRSqBracket) {
		return ResOk[Expr](&ArrayExpr{Token: p.previous(), Elements: []Expr{}})
	}

	elements := []Expr{}
	for {
		exprRes := p.expression()
		if exprRes.IsErr() {
			return exprRes
		}
		elements = append(elements, exprRes.Value)
		if !p.match(TokenComma) {
			break
		}
	}

	if !p.match(TokenRSqBracket) {
		return ResErr[Expr](NewParserError("Expected ']' after array elements.", p.current()))
	}

	return ResOk[Expr](&ArrayExpr{Token: p.previous(), Elements: elements})
}

func (p *Parser) whileStatement() Result[Stmt] {
	whileTok := p.previous()

	condExprRes := p.expression()
	if condExprRes.IsErr() {
		return ResErr[Stmt](condExprRes.Err)
	}

	if !p.match(TokenLCurlyBrace) {
		return ResErr[Stmt](NewParserError("Expected '{' after while condition.", p.current()))
	}

	bodyBlockRes := p.block()
	if bodyBlockRes.IsErr() {
		return ResErr[Stmt](bodyBlockRes.Err)
	}
	bodyBlock := bodyBlockRes.Value

	return ResOk[Stmt](&WhileStmt{
		Token: whileTok,
		Cond:  condExprRes.Value,
		Body:  bodyBlock,
	})
}

func (p *Parser) forInStatement() Result[Stmt] {
	forTok := p.previous()

	hasParen := p.match(TokenLParen)

	if !p.match(TokenIdent) {
		return ResErr[Stmt](NewParserError("Expected loop variable name.", p.current()))
	}

	loopVariable := p.previous()

	if !p.matchKeyword("in") {
		return ResErr[Stmt](NewParserError("Expected 'in' keyword.", p.current()))
	}

	iterableRes := p.expression()
	if iterableRes.IsErr() {
		return ResErr[Stmt](iterableRes.Err)
	}

	if hasParen {
		if !p.match(TokenRParen) {
			return ResErr[Stmt](NewParserError("Expected ')' after loop variable.", p.current()))
		}
	}

	if !p.match(TokenLCurlyBrace) {
		return ResErr[Stmt](NewParserError("Expected '{' after while condition.", p.current()))
	}

	bodyBlockRes := p.block()
	if bodyBlockRes.IsErr() {
		return ResErr[Stmt](bodyBlockRes.Err)
	}

	bodyBlock := bodyBlockRes.Value

	return ResOk[Stmt](&ForInStmt{
		Token:        forTok,
		LoopVariable: loopVariable,
		Iterable:     iterableRes.Value,
		Body:         *bodyBlock,
	})
}

func (p *Parser) functionDefinition() Result[Stmt] {
	fnTok := p.previous()

	if !p.match(TokenIdent) {
		return ResErr[Stmt](NewParserError("Expected function name.", p.current()))
	}
	fnName := p.previous()

	if !p.match(TokenLParen) {
		return ResErr[Stmt](NewParserError("Expected '(' after function name.", p.current()))
	}

	params := []*Parameter{}
	if !p.check(TokenRParen) {
		for {
			paramNameRes := p.consume(TokenIdent, "Expected parameter name.")
			if paramNameRes.IsErr() {
				return ResErr[Stmt](paramNameRes.Err)
			}
			paramName := paramNameRes.Value

			var paramType Expr = nil
			if p.match(TokenColon) {
				typeRes := p.expression()
				if typeRes.IsErr() {
					return ResErr[Stmt](typeRes.Err)
				}
				paramType = typeRes.Value
			}

			params = append(params, &Parameter{Name: paramName, Type: paramType})

			if !p.match(TokenComma) {
				break
			}
			if p.check(TokenRParen) {
				break
			}
		}
	}

	if !p.match(TokenRParen) {
		return ResErr[Stmt](NewParserError("Expected ')' after parameters.", p.current()))
	}

	var returnType Expr = nil
	if p.match(TokenArrow) {
		typeRes := p.expression()
		if typeRes.IsErr() {
			return ResErr[Stmt](typeRes.Err)
		}
		returnType = typeRes.Value
	}

	if !p.match(TokenLCurlyBrace) {
		return ResErr[Stmt](NewParserError("Expected '{' after function parameters.", p.current()))
	}

	bodyBlockRes := p.block()
	if bodyBlockRes.IsErr() {
		return ResErr[Stmt](bodyBlockRes.Err)
	}
	bodyBlock := bodyBlockRes.Value

	return ResOk[Stmt](&FunctionDefStmt{
		Token:      fnTok,
		Name:       fnName,
		Params:     params,
		Body:       bodyBlock,
		ReturnType: returnType,
	})
}

func (p *Parser) returnStatement() Result[Stmt] {
	returnTok := p.previous()

	var valueExpr Expr = nil
	if !p.check(TokenSemiColon) && !p.check(TokenRCurlyBrace) && !p.check(TokenEOF) {
		exprRes := p.expression()
		if exprRes.IsErr() {
			return ResErr[Stmt](exprRes.Err)
		}
		valueExpr = exprRes.Value
	}

	return ResOk[Stmt](&ReturnStmt{
		Token: returnTok,
		Value: &valueExpr,
	})
}

func (p *Parser) ifStatement() Result[Stmt] {
	ifTok := p.previous()

	condExprRes := p.expression()
	if condExprRes.IsErr() {
		return ResErr[Stmt](condExprRes.Err)
	}

	if !p.match(TokenLCurlyBrace) {
		return ResErr[Stmt](NewParserError("Expected '{' after if condition.", p.current()))
	}
	thenBranchRes := p.block()
	if thenBranchRes.IsErr() {
		return ResErr[Stmt](thenBranchRes.Err)
	}

	var elseBranchNode *Block = nil

	if p.matchKeyword("else") {
		elseTok := p.previous()

		if p.matchKeyword("if") {
			elseIfRes := p.ifStatement()
			if elseIfRes.IsErr() {
				return ResErr[Stmt](elseIfRes.Err)
			}
			elseBranchNode = &Block{
				Token:      elseTok,
				Statements: []ASTNode{elseIfRes.Value},
				EndToken:   elseTok,
			}
		} else {
			if !p.match(TokenLCurlyBrace) {
				return ResErr[Stmt](NewParserError("Expected '{' after if condition.", p.current()))
			}
			elseblockRes := p.block()
			if elseblockRes.IsErr() {
				return ResErr[Stmt](elseblockRes.Err)
			}
			elseBranchNode = elseblockRes.Value
		}
	}

	return ResOk[Stmt](&IfStmt{
		Token:      ifTok,
		Condition:  condExprRes.Value,
		ThenBranch: thenBranchRes.Value,
		ElseBranch: elseBranchNode,
	})
}

func (p *Parser) breakStatement() Result[Stmt] {
	breakTok := p.previous()

	p.match(TokenSemiColon) // optional ;

	return ResOk[Stmt](&BreakStmt{
		Token: breakTok,
	})
}

func (p *Parser) continueStatement() Result[Stmt] {
	continueTok := p.previous()

	p.match(TokenSemiColon) // optional ;

	return ResOk[Stmt](&ContinueStmt{
		Token: continueTok,
	})
}

func (p *Parser) map_literal() Result[Expr] {
	lcurlyTok := p.previous()

	if p.check(TokenRCurlyBrace) {
		p.advance() // consume '}' for empty map
		return ResOk[Expr](
			&MapExpr{
				Token:      lcurlyTok,
				Properties: make([]MapProperty, 0),
			},
		)
	}

	var props []MapProperty

	for {
		var keyNode Expr
		isComputed := false

		// Check for computed property `[expr]`
		if p.match(TokenLSqBracket) {
			isComputed = true
			keyRes := p.expression()
			if keyRes.IsErr() {
				return keyRes
			}
			keyNode = keyRes.Value
			if err := p.consume(TokenRSqBracket, "Expected ']' after computed property key."); err.IsErr() {
				return ResErr[Expr](err.Err)
			}
		} else {
			// For regular keys, we expect an identifier or a string.
			keyRes := p.primary()
			if keyRes.IsErr() {
				return keyRes
			}
			keyNode = keyRes.Value
		}

		consErr := p.consume(TokenColon, "Expected ':' after map property key.")
		if consErr.IsErr() {
			return ResErr[Expr](consErr.Err)
		}

		valueRes := p.expression()
		if valueRes.IsErr() {
			return valueRes
		}

		props = append(props, MapProperty{
			Key:        keyNode,
			Value:      valueRes.Value,
			IsComputed: isComputed,
		})

		if !p.match(TokenComma) {
			break
		}

		if p.check(TokenRCurlyBrace) {
			break
		}
	}

	consErr := p.consume(TokenRCurlyBrace, "Expected '}' after map properties.")
	if consErr.IsErr() {
		return ResErr[Expr](consErr.Err)
	}

	return ResOk[Expr](&MapExpr{
		Token:      lcurlyTok,
		Properties: props,
	})
}
