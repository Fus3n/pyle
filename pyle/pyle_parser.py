from typing import Optional
from .pyle_ast import *
from .pyle_types import Loc, Token, TokenType as TT
from .pyle_errors import Result, ParseError


class Parser:
    
    def __init__(self, tokens: list[Token]) -> None:
        self.toks = tokens
        self.curr_tok: Token = self.toks[0] if len(self.toks) > 0 else Token(TT.EOF, "", Loc(0,0), "") 
        self.curr_tok_idx = 0
        self.source_name = self.toks[0].source_name if len(self.toks) > 0 else ""


    def _consume(self, tok_kind: TT) -> Result[Token]: 
        current_token_being_consumed = self.curr_tok
        if current_token_being_consumed.kind == tok_kind:
            self.curr_tok_idx += 1
            if self.curr_tok_idx < len(self.toks):
                self.curr_tok = self.toks[self.curr_tok_idx]
            else:
                last_loc = current_token_being_consumed.loc if current_token_being_consumed else Loc(0,0)
                self.curr_tok = Token(TT.EOF, "", last_loc, self.source_name) 
            return Result.ok(current_token_being_consumed)
        else:
            return Result.err(
                ParseError(f"Unexpected Token: expected {tok_kind.name} but found {self.curr_tok.kind.name}", self.curr_tok)
            )

    def is_kind(self, tok_type: TT):
        return self.curr_tok.kind == tok_type

    def _peek(self, offset: int = 1) -> Token | None:
        peek_idx = self.curr_tok_idx + offset
        if peek_idx < len(self.toks):
            return self.toks[peek_idx]
        return None
    
    def peek_match_type(self, current_type: TT, peek_type: TT) -> bool:
        if self.is_kind(current_type):
            peeked = self._peek(1)
            if peeked and peeked.kind == peek_type:
                return True
        return False

    def parse(self) -> Result[Block]:
        prog_block = Block(token=None) # Top-level block for the whole program
        
        while not self.is_kind(TT.EOF):
            stmt_res = self.statements()
            if stmt_res.is_err():
                return stmt_res # Propagate parsing error
            prog_block.statements.append(stmt_res.ok_val)
            
            # Optionally consume a semicolon if present after a statement
            if self.is_kind(TT.SEMICOLON):
                self._consume(TT.SEMICOLON)
            elif self.is_kind(TT.EOF):
                break
            elif self.is_kind(TT.R_CURLY_BRACE): 
                 return Result.err(ParseError("Unexpected '}' at top level.", self.curr_tok))

        return Result.ok(prog_block)
        
    def block(self) -> Result[Block]: 
        open_curly_res = self._consume(TT.L_CURLY_BRACE)
        if open_curly_res.is_err(): return open_curly_res

        block_node = Block(token=open_curly_res.ok_val) # Token is '{'

        while not self.is_kind(TT.R_CURLY_BRACE) and not self.is_kind(TT.EOF):
            stmt_res = self.statements()
            if stmt_res.is_err():
                return stmt_res
            block_node.statements.append(stmt_res.ok_val)

            if self.is_kind(TT.SEMICOLON):
                self._consume(TT.SEMICOLON)
            elif self.is_kind(TT.R_CURLY_BRACE) or self.is_kind(TT.EOF):
                break

        close_curly_res = self._consume(TT.R_CURLY_BRACE)
        if close_curly_res.is_err():
            start_loc_info = f"line {open_curly_res.ok_val.loc.line}" if open_curly_res.is_ok() else "unknown location"
            return Result.err(
                ParseError(f"Missing '}}' to close block started at {start_loc_info}.", 
                           token=self.curr_tok, # Current token where '}' was expected
                           source_name=self.source_name) 
            )
        return Result.ok(block_node)
        
    def statements(self) -> Result[Stmt | Expr]:
        if self.is_kind(TT.KEYWORD):
            if self.curr_tok.value in ("let", "const"):
                return self.parse_variable_def()
            elif self.curr_tok.value == "if":
                return self.parse_if_statement()
            elif self.curr_tok.value == "while":
                return self.parse_while_statement()
            elif self.curr_tok.value == "for":
                return self.parse_for_in_statement()
            elif self.curr_tok.value == "fn": 
                return self.parse_function_definition()
            elif self.curr_tok.value == "return": 
                return self.parse_return_statement()
            elif self.curr_tok.value == "break": 
                return self.break_statement() 
            elif self.curr_tok.value == "continue": 
                return self.continue_statement() 
        elif self.is_kind(TT.L_CURLY_BRACE): 
            return self.block()

        if self.is_kind(TT.IDENT):
            peeked = self._peek(1)
            if peeked and peeked.kind == TT.ASSIGN:
                return self.parse_variable_assign()
        
        expr_res = self.expr()
        if expr_res.is_err():
            return expr_res
        expr_node = expr_res.ok_val

        if isinstance(expr_node, IndexExpr) and self.is_kind(TT.ASSIGN):
            self._consume(TT.ASSIGN)
            value_res = self.expr()
            if value_res.is_err():
                return value_res
            return Result.ok(AssignIndexStmt(
                collection=expr_node.collection,
                index=expr_node.index,
                value=value_res.ok_val,
                token=expr_node.token
            ))

        return Result.ok(expr_node)
    
    def break_statement(self) -> Result[BreakStmt]:
        break_tok_res = self._consume(TT.KEYWORD)
        if break_tok_res.is_err(): return break_tok_res 

        if self.is_kind(TT.SEMICOLON):
            self._consume(TT.SEMICOLON)

        return Result.ok(BreakStmt(token=break_tok_res.ok_val))
        
    def continue_statement(self) -> Result[ContinueStmt]:
        continue_token_res = self._consume(TT.KEYWORD)
        if continue_token_res.is_err(): return continue_token_res

        if self.is_kind(TT.SEMICOLON):
            self._consume(TT.SEMICOLON)
            
        return Result.ok(ContinueStmt(token=continue_token_res.ok_val))
        
    def parse_function_definition(self) -> Result[FunctionDefStmt]:
        fn_token = self.curr_tok
        if self._consume(TT.KEYWORD).is_err() or fn_token.value != "fn": # Should be 'fn'
            return Result.err(ParseError("Expected 'fn' keyword.", fn_token))

        name_token = self.curr_tok
        if self._consume(TT.IDENT).is_err():
            return Result.err(ParseError("Expected function name after 'fn'.", name_token))

        if self._consume(TT.L_PAREN).is_err():
            return Result.err(ParseError(f"Expected '(' after function name '{name_token.value}'.", self.curr_tok))

        params: list[Token] = []
        if not self.is_kind(TT.R_PAREN): # If there are parameters
            while True:
                param_tok = self.curr_tok
                if self._consume(TT.IDENT).is_err():
                    return Result.err(ParseError("Expected parameter name or ')'.", param_tok))
                params.append(param_tok)

                if self.is_kind(TT.R_PAREN):
                    break
                if self._consume(TT.COMMA).is_err():
                    return Result.err(ParseError("Expected ',' or ')' in parameter list.", self.curr_tok))
        
        if self._consume(TT.R_PAREN).is_err():
            return Result.err(ParseError("Expected ')' after parameter list.", self.curr_tok))

        body_res = self.block() 
        if body_res.is_err():
            return Result.err(ParseError(f"Expected '{{' to start function body for '{name_token.value}'.", self.curr_tok))

        return Result.ok(FunctionDefStmt(token=fn_token, name=name_token, params=params, body=body_res.ok_val))

    def parse_return_statement(self) -> Result[ReturnStmt]:
        return_token = self.curr_tok
        if self._consume(TT.KEYWORD).is_err() or return_token.value != "return":
            return Result.err(ParseError("Expected 'return' keyword.", return_token))

        value_expr: Expr | None = None
        if not self.is_kind(TT.SEMICOLON) and \
           not self.is_kind(TT.R_CURLY_BRACE) and \
           not self.is_kind(TT.EOF):
            expr_res = self.expr()
            if expr_res.is_err():
                return expr_res
            value_expr = expr_res.ok_val
        
        return Result.ok(ReturnStmt(token=return_token, value=value_expr))

    def parse_range_specifier(self) -> Result[RangeSpecifier]: 
        start_expr_res = self.expr() 
        if start_expr_res.is_err(): return start_expr_res
        start_expr = start_expr_res.ok_val

        end_expr_res = self.logical_and_expr() 
        if end_expr_res.is_err():
            return Result.err(ParseError("Expected end expression for range.", self.curr_tok, underlying_error=end_expr_res.err_val))
        end_expr = end_expr_res.ok_val

        step_expr: Expr = None
        if self.is_kind(TT.COLON):
            self._consume(TT.COLON) 
            step_val_res = self.logical_and_expr()
            if step_val_res.is_err():
                 return Result.err(ParseError("Expected step expression for range after ':'.", self.curr_tok, underlying_error=step_val_res.err_val))
            step_expr = step_val_res.ok_val
            
        return Result.ok(RangeSpecifier(token=start_expr.token, start=start_expr, end=end_expr, step=step_expr))


    def parse_for_in_statement(self) -> Result[ForInStmt]:
        for_token_res = self._consume(TT.KEYWORD) 
        if for_token_res.is_err(): return for_token_res
        for_token = for_token_res.ok_val

        loop_var_token = self.curr_tok
        loop_var_res = self._consume(TT.IDENT) 
        if loop_var_res.is_err():
            return Result.err(ParseError("Expected identifier for loop variable after 'for'.", loop_var_token)) # Use loop_var_token for error context

        in_keyword_res = self._consume(TT.KEYWORD) 
        if in_keyword_res.is_err() or in_keyword_res.ok_val.value != "in":
            return Result.err(ParseError("Expected 'in' keyword after loop variable in 'for' loop.", self.curr_tok)) # Use current token

        iterable_res = self.expr()  
        if iterable_res.is_err():
            return iterable_res
        iterable_node = iterable_res.ok_val

        body_block_res = self.block() 
        if body_block_res.is_err(): 
            return Result.err(ParseError(f"Expected '{{' to start 'for' loop body.", token=self.curr_tok, underlying_error=body_block_res.err_val))


        return Result.ok(ForInStmt(
                                token=for_token,
                                loop_variable=loop_var_token, 
                                iterable=iterable_node, 
                                body=body_block_res.ok_val
                            )
                        )
    def parse_while_statement(self):
        while_token = self.curr_tok
        if (res := self._consume(TT.KEYWORD)).is_err() or while_token.value != "while": return res if res.is_err() else Result.err(ParseError("Expected 'while'.", while_token))


        condition_expr_res = self.expr() 
        if condition_expr_res.is_err():
            return condition_expr_res
        
        body_block_res = self.block()
        if body_block_res.is_err():
            return body_block_res


        return Result.ok(WhileStmt(
                            condition=condition_expr_res.ok_val,
                            body=body_block_res.ok_val,
                            token=while_token,
                        )
                    )

    def parse_variable_assign(self) -> Result[Stmt]:
        var_name_token = self.curr_tok 
        if (res := self._consume(TT.IDENT)).is_err(): return res

        assign_res = self._consume(TT.ASSIGN)
        if assign_res.is_err(): return assign_res

        expr_res = self.expr()
        if expr_res.is_err(): return expr_res

        return Result.ok(
            AssignStmt(name=var_name_token, value=expr_res.ok_val, token=var_name_token) 
        )

    def parse_variable_def(self) -> Result[Stmt]:
        let_token = self.curr_tok
        if (res := self._consume(TT.KEYWORD)).is_err() or let_token.value not in ("let", "const"): 
            return res if res.is_err() else Result.err(ParseError("Expected 'let' or 'const' keyword.", let_token))
        
        var_name_tok = self.curr_tok
        var_name_res = self._consume(TT.IDENT)
        if var_name_res.is_err(): return var_name_res

        assign_res = self._consume(TT.ASSIGN)
        if assign_res.is_err(): return assign_res

        expr_res = self.expr()
        if expr_res.is_err(): return expr_res
        
        return Result.ok(
            VarDeclareStmt(
                token=let_token, 
                name=var_name_tok, 
                initializer=expr_res.ok_val, 
                is_const=(let_token.value == "const")
            )
        )

    def parse_if_statement(self) -> Result[Stmt]:
        if_token = self.curr_tok 
        if (res := self._consume(TT.KEYWORD)).is_err() or if_token.value != "if":
            return res if res.is_err() else Result.err(ParseError("Expected 'if' keyword.", if_token))
        
        condition_expr_res = self.expr() 
        if condition_expr_res.is_err():
            return condition_expr_res

        then_branch_res = self.block()
        if then_branch_res.is_err():
             return Result.err(ParseError(f"Expected '{{' to start 'if' block body.", token=self.curr_tok))

        
        else_branch_node: Optional[Block] = None
        if self.is_kind(TT.KEYWORD) and self.curr_tok.value == "else":
            else_token = self.curr_tok
            self._consume(TT.KEYWORD) 
            
            if self.is_kind(TT.KEYWORD) and self.curr_tok.value == "if":
                else_if_stmt_res = self.parse_if_statement() 
                if else_if_stmt_res.is_err():
                    return else_if_stmt_res
                else_branch_node = Block(statements=[else_if_stmt_res.ok_val], token=else_token)
            else:
                else_block_res = self.block() 
                if else_block_res.is_err():
                    return Result.err(ParseError(f"Expected '{{' to start 'else' block body.", token=self.curr_tok, underlying_error=else_block_res.err_val))
                else_branch_node = else_block_res.ok_val
                
        return Result.ok(IfStmt(token=if_token, 
                                condition=condition_expr_res.ok_val, 
                                then_branch=then_branch_res.ok_val, 
                                else_branch=else_branch_node)
                        )

    def expr(self) -> Result[Expr]:
        left_res = self.ranges()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.curr_tok.is_keyword("or"):
            op_token = self.curr_tok
            if self._consume(TT.KEYWORD).is_err(): break 

            right_res = self.ranges()
            if right_res.is_err(): return right_res
            left = LogicalOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)

        return Result.ok(left)

    def ranges(self) -> Result[Expr]:
        left_res = self.logical_and_expr()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        if self.is_kind(TT.COLON):
            start_token = left.token 
            self._consume(TT.COLON) 
            
            end_expr_res = self.logical_and_expr() 
            if end_expr_res.is_err(): return end_expr_res
            end_expr = end_expr_res.ok_val

            step_expr: Expr | None = None
            if self.is_kind(TT.COLON):
                self._consume(TT.COLON) 
                step_expr_res = self.logical_and_expr() 
                if step_expr_res.is_err(): return step_expr_res
                step_expr = step_expr_res.ok_val
            
            return Result.ok(RangeSpecifier(token=start_token, start=left, end=end_expr, step=step_expr))
        return Result.ok(left)


    def logical_and_expr(self) -> Result[Expr]:
        left_res = self.equality_expr()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.curr_tok.is_keyword("and"):
            op_token = self.curr_tok
            if self._consume(TT.KEYWORD).is_err(): break 

            right_res = self.equality_expr()
            if right_res.is_err(): return right_res
            left = LogicalOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)
        return Result.ok(left)
    
    def equality_expr(self) -> Result[Expr]:
        left_res = self.comparsion_expr()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.is_kind(TT.EQ) or self.is_kind(TT.NEQ):
            op_token = self.curr_tok
            if self._consume(self.curr_tok.kind).is_err(): break

            right_res = self.comparsion_expr()
            if right_res.is_err(): return right_res
            left = ComparisonOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)
        return Result.ok(left)
    
    def comparsion_expr(self) -> Result[Expr]:
        left_res = self.term()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.is_kind(TT.GT) or self.is_kind(TT.GTE) or \
              self.is_kind(TT.LT) or self.is_kind(TT.LTE):
            op_token = self.curr_tok
            if self._consume(self.curr_tok.kind).is_err(): break

            right_res = self.term()
            if right_res.is_err(): return right_res
            left = ComparisonOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)
        return Result.ok(left)
    
    def term(self) -> Result[Expr]:
        left_res = self.factor()
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.is_kind(TT.PLUS) or self.is_kind(TT.MINUS):
            op_token = self.curr_tok
            if self._consume(self.curr_tok.kind).is_err(): break

            right_res = self.factor()
            if right_res.is_err(): return right_res
            left = BinaryOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)
        return Result.ok(left)

    def factor(self) -> Result[Expr]:
        left_res = self.unary() 
        if left_res.is_err(): return left_res
        left = left_res.ok_val

        while self.is_kind(TT.MUL) or self.is_kind(TT.DIV) or self.is_kind(TT.MOD): 
            op_token = self.curr_tok
            if self._consume(self.curr_tok.kind).is_err(): break

            right_res = self.unary() 
            if right_res.is_err(): return right_res
            left = BinaryOp(left=left, op=op_token, right=right_res.ok_val, token=op_token)
        return Result.ok(left)

    def unary(self) -> Result[Expr]: 
        if self.is_kind(TT.MINUS) or (self.is_kind(TT.KEYWORD) and self.curr_tok.value == "not"):
            op_token = self.curr_tok
            consume_res = self._consume(op_token.kind)
            if consume_res.is_err():
                return consume_res

            operand_res = self.unary()
            if operand_res.is_err():
                return operand_res
            
            return Result.ok(UnaryOp(op=op_token, operand=operand_res.ok_val, token=op_token))
        
        return self.call() 

    def call(self) -> Result[Expr]: 
        expr_res = self.primary() 
        if expr_res.is_err(): return expr_res
        
        current_expr = expr_res.ok_val


        while True:
            if self.is_kind(TT.L_PAREN):
                l_paren_token = self.curr_tok
                self._consume(TT.L_PAREN)

                arguments: list[Expr] = []
                keywords: list[KeywordArg] = []
                seen_keyword = False

                if not self.is_kind(TT.R_PAREN):
                    while True:
                        if self.is_kind(TT.IDENT) and self._peek(1) and self._peek(1).kind == TT.ASSIGN:
                            name_token = self.curr_tok
                            self._consume(TT.IDENT)
                            self._consume(TT.ASSIGN)
                            value_res = self.expr()
                            if value_res.is_err(): return value_res
                            keywords.append(KeywordArg(name=name_token, value=value_res.ok_val))
                            seen_keyword = True
                        else:
                            if seen_keyword:
                                return Result.err(ParseError("Positional argument after keyword argument is not allowed.", self.curr_tok))
                            arg_res = self.expr()
                            if arg_res.is_err(): return arg_res
                            arguments.append(arg_res.ok_val)

                        if self.is_kind(TT.R_PAREN):
                            break
                        comma_res = self._consume(TT.COMMA)
                        if comma_res.is_err():
                            return Result.err(ParseError("Expected ',' or ')' in argument list.", self.curr_tok, underlying_error=comma_res.err_val))

                r_paren_res = self._consume(TT.R_PAREN)
                if r_paren_res.is_err(): return r_paren_res

                current_expr = CallExpr(callee=current_expr, arguments=arguments, keywords=keywords, token=l_paren_token)
            
            elif self.is_kind(TT.L_SQ_BRACKET):
                l_bracket_token = self.curr_tok
                self._consume(TT.L_SQ_BRACKET)
                index_expr_res = self.expr()
                if index_expr_res.is_err(): return index_expr_res
                if self._consume(TT.R_SQ_BRACKET).is_err():
                    return Result.err(ParseError("Expected ']' after index expression.", self.curr_tok))
                current_expr = IndexExpr(collection=current_expr, index=index_expr_res.ok_val, token=l_bracket_token)
            elif self.is_kind(TT.DOT):
                dot_token = self.curr_tok
                self._consume(TT.DOT)
                attr_token = self.curr_tok
                if self._consume(TT.IDENT).is_err():
                    return Result.err(ParseError("Expected identifier after '.'", attr_token))
                current_expr = DotExpr(object=current_expr, attr=attr_token, token=dot_token)
            else:
                break
        return Result.ok(current_expr)


    def primary(self) -> Result[Expr]: 
        tok = self.curr_tok

        if tok.kind == TT.KEYWORD and tok.value == "fn":
            return self.parse_function_expr()
        elif tok.kind == TT.INT or tok.kind == TT.FLOAT:
            self._consume(tok.kind)
            value = int(tok.value) if tok.kind == TT.INT else float(tok.value)
            return Result.ok(Number(value=value, token=tok))
        elif tok.kind == TT.STRING:
            self._consume(tok.kind)
            return Result.ok(String(value=tok.value, token=tok))
        elif tok.kind == TT.KEYWORD:
            if tok.value == "true":
                self._consume(tok.kind)
                return Result.ok(Boolean(value=True, token=tok))
            elif tok.value == "false":
                self._consume(tok.kind)
                return Result.ok(Boolean(value=False, token=tok))
            # elif tok.value: # For later
            #     self._consume(tok.kind)
            #     return Result.ok(None(token=tok))

        elif tok.kind == TT.L_PAREN: # Grouped expression
            self._consume(tok.kind)
            expr_val_res = self.expr()
            if expr_val_res.is_err(): return expr_val_res
            
            r_paren_res = self._consume(TT.R_PAREN)
            if r_paren_res.is_err(): 
                return Result.err(ParseError("Expected ')' after expression in parentheses.", self.curr_tok, underlying_error=r_paren_res.err_val))
            return expr_val_res 
        elif tok.kind == TT.L_SQ_BRACKET: # Array literal
            return self.parse_array_literal()
        elif tok.kind == TT.IDENT:
            self._consume(tok.kind)
            return Result.ok(VariableExpr(name=tok, token=tok)) 
            
        prev_tok_info = "None"
        if self.curr_tok_idx > 0 and self.toks and self.curr_tok_idx -1 < len(self.toks): 
            prev_tok_info = str(self.toks[self.curr_tok_idx-1].kind.name)

        return Result.err(
            ParseError(
                f"PRIMARY_FAIL: Unexpected Token '{(tok.kind.name if tok else 'None')}' value '{(tok.value if tok else 'N/A')}'. Previous token kind: {prev_tok_info}",
                token=tok
            )
        )

    def parse_array_literal(self) -> Result[ArrayLiteral]:
        open_bracket_tok = self.curr_tok
        if (res := self._consume(TT.L_SQ_BRACKET)).is_err():
            return Result.err(ParseError("Expected '[' to start array literal.", self.curr_tok, underlying_error=res.err_val))
        
        elements: list[Expr] = []
        if self.is_kind(TT.R_SQ_BRACKET):
            close_res = self._consume(TT.R_SQ_BRACKET)
            if close_res.is_err(): return close_res 
            return Result.ok(ArrayLiteral(token=open_bracket_tok, elements=elements))
        
        while True: 
            element_res = self.expr()
            if element_res.is_err(): return element_res
            elements.append(element_res.ok_val)

            if self.is_kind(TT.R_SQ_BRACKET):
                break 
            
            comma_res = self._consume(TT.COMMA)
            if comma_res.is_err():
                return Result.err(ParseError("Expected ',' or ']' in array literal.", self.curr_tok, underlying_error=comma_res.err_val))
            
            if self.is_kind(TT.R_SQ_BRACKET):
                break


        closing_bracket_res = self._consume(TT.R_SQ_BRACKET)
        if closing_bracket_res.is_err():
            return Result.err(ParseError(f"Expected ']' to close array literal started at {open_bracket_tok.get_file_loc()}.", self.curr_tok, underlying_error=closing_bracket_res.err_val))

        return Result.ok(ArrayLiteral(token=open_bracket_tok, elements=elements))
    
    def parse_function_expr(self) -> Result[FunctionExpr]:
        fn_token = self.curr_tok
        self._consume(TT.KEYWORD) 
        if self._consume(TT.L_PAREN).is_err():
            return Result.err(ParseError("Expected '(' after 'fn' in function expression.", self.curr_tok))
        params = []
        if not self.is_kind(TT.R_PAREN):
            while True:
                param_tok = self.curr_tok
                if self._consume(TT.IDENT).is_err():
                    return Result.err(ParseError("Expected parameter name or ')'.", param_tok))
                params.append(param_tok)
                if self.is_kind(TT.R_PAREN):
                    break
                if self._consume(TT.COMMA).is_err():
                    return Result.err(ParseError("Expected ',' or ')' in parameter list.", self.curr_tok))
        if self._consume(TT.R_PAREN).is_err():
            return Result.err(ParseError("Expected ')' after parameter list.", self.curr_tok))
        body_res = self.block()
        if body_res.is_err():
            return Result.err(ParseError("Expected '{' to start function body.", self.curr_tok, underlying_error=body_res.err_val))
        return Result.ok(FunctionExpr(token=fn_token, params=params, body=body_res.ok_val))
