from .pyle_errors import LexerError, Result
from .pyle_types import *
import sys

from typing import Tuple


BINARY_OPERATORS = {
    "+": TokenType.PLUS,
    "-": TokenType.MINUS,
    "*": TokenType.MUL,
    "/": TokenType.DIV,
    "%": TokenType.MOD,
}


class Lexer:
    def __init__(self, src_name: str, source: str):
        self.source = source
        self.src_name = InternedStr(sys.intern(src_name))
        self.curr_idx = 0
        self.curr_char: str | None = self.source[self.curr_idx] if self.source else None
        self.line = 1
        self.col = 1
        self.tokens = []

    def _advance(self):
        if self.curr_char == "\n":
            self.line += 1
            self.col = 0

        self.curr_idx += 1
        if self.curr_idx < len(self.source):
            self.curr_char = self.source[self.curr_idx]
            self.col += 1
        else:
            self.curr_char = None

    def has_char(self):
        """Checks if there is a valid current character"""
        return self.curr_char is not None

    def add_tok(self, tok_type: TokenType, value: str, loc: Loc):
        self.tokens.append(Token(tok_type, value, loc, source_name=self.src_name))

    def get_tok(self, tok_type: TokenType, value: str, loc: Loc) -> Token:
        """Helper for creating Token"""
        return Token(tok_type, value, loc, source_name=self.src_name)
    
    def get_loc(self, col_start: int | None = None) -> Loc:
        """Helper for creating Location"""
        actual_col_start = col_start if col_start is not None else self.col
        actual_col_end = self.col if col_start is not None else self.col
        
        if col_start is not None:
             actual_col_end = self.col -1

        return Loc(self.line, actual_col_start, actual_col_end if col_start is not None else None)

    def create_error(self, msg: str, loc: Loc):
        """Create and return LexerError"""
        error_token_loc = loc if isinstance(loc, Loc) else self.get_loc()
        tok = self.get_tok(TokenType.ERROR, msg, error_token_loc)
        return Result.err(LexerError(msg, tok))
    
    def tokenize(self) -> Result[list[Token]]:
        while self.has_char():
            if self.curr_char.isspace():
                self._advance()
                continue
            elif self.curr_char == "/":
                peeked_char = self.peek()
                if peeked_char == "/":
                    self._advance()
                    self._advance()
                    while self.has_char() and self.curr_char != "\n":
                        self._advance()
                    continue
                elif peeked_char == "*":
                    comment_start_line = self.line
                    comment_start_col = self.col
                    self._advance()
                    self._advance()
                    
                    found_comment_end = False
                    while self.has_char():
                        if self.curr_char == "*" and self.peek() == "/":
                            self._advance()
                            self._advance()
                            found_comment_end = True
                            break
                        self._advance()
                    
                    if not found_comment_end:
                        err_loc = Loc(comment_start_line, comment_start_col, None)
                        return self.create_error("Unterminated multi-line comment", err_loc)
                    continue
            elif self.curr_char in ("+", "-", "/", "*", "%"):
                op = BINARY_OPERATORS[self.curr_char]
                self.tokens.append(self.get_tok(op, self.curr_char, self.get_loc()))
                self._advance()
            elif self.curr_char == "%":
                self.tokens.append(self.get_tok(TokenType.MOD, self.curr_char, self.get_loc()))
                self._advance()
            elif self.curr_char == ";":
                self.tokens.append(self.get_tok(TokenType.SEMICOLON, self.curr_char, self.get_loc()))
                self._advance()
            elif self.curr_char == ".":
                self.add_tok(TokenType.DOT, ".", self.get_loc())
                self._advance()
            elif self.curr_char == ":":
                 self.add_tok(TokenType.COLON, ":", self.get_loc())
                 self._advance()
            elif self.curr_char in ("(", ")", "[", "]", "{", "}", "[", "]", ","):
                paren_tt = {
                    "(": TokenType.L_PAREN, ")": TokenType.R_PAREN,
                    "[": TokenType.L_BRACKET, "]": TokenType.R_BRACKET,
                    "{": TokenType.L_CURLY_BRACE, "}": TokenType.R_CURLY_BRACE,
                    "[": TokenType.L_SQ_BRACKET, "]": TokenType.R_SQ_BRACKET,
                    ",": TokenType.COMMA
                }
                self.add_tok(paren_tt[self.curr_char], self.curr_char, self.get_loc())
                self._advance()
            elif self.curr_char == ">":
                start_loc = self.get_loc()
                peeked = self.peek()
                self._advance()
                if peeked and peeked == "=":
                    start_col = self.col
                    self._advance()
                    self.add_tok(TokenType.GTE, ">=", start_loc.copy_with(col_start=start_col))
                    continue
                self.add_tok(TokenType.GT, ">", start_loc)
            elif self.curr_char == "<":
                start_loc = self.get_loc()
                peeked = self.peek()
                self._advance()
                if peeked and peeked == "=":
                    start_col = self.col
                    self._advance()
                    self.add_tok(TokenType.LTE, "<=", start_loc.copy_with(col_start=start_col))
                    continue
                self.add_tok(TokenType.LT, "<", start_loc)
            elif self.curr_char in ("="):
                start_loc = self.get_loc()
                peeked = self.peek()
                self._advance()
                if peeked and peeked == "=":
                    start_col = self.col
                    self._advance()
                    self.add_tok(TokenType.EQ, "==", start_loc.copy_with(col_start=start_col))
                    continue
                self.add_tok(TokenType.ASSIGN, "=", start_loc)
            elif self.curr_char == "!":
                start_loc = self.get_loc()
                peeked = self.peek()
                self._advance()
                if peeked and peeked == "=":
                    start_col = self.col
                    self._advance()
                    self.add_tok(TokenType.NEQ, "!=", start_loc.copy_with(col_start=start_col))
                    continue
                self.add_tok(TokenType.BANG, "!", start_loc)
            elif self.curr_char.isdigit():
                res = self._parse_numbers()
                if res.is_err(): return res
                self.tokens.append(res.ok_val)
            elif self.is_any_of_chars(("\"", "'")):
                res = self._parse_string()
                if res.is_err(): return res
                self.tokens.append(res.ok_val)
            elif self.curr_char.isalpha():
                res = self._parse_ident()
                if res.is_err(): return res
                self.tokens.append(res.ok_val)

        eof_tok = Token(TokenType.EOF, "", Loc(self.line, 0, self.col))
        self.tokens.append(eof_tok)
        return Result.ok(self.tokens)

    def peek(self, offset: int = 1) -> str | None:
        peek_idx = self.curr_idx + offset
        if peek_idx < len(self.source):
            return self.source[peek_idx]
        return None

    def is_char_eq(self, char_to_check: str) -> bool:
        """Checks if the current character is equal to the given character."""
        return self.has_char() and self.curr_char == char_to_check

    def is_any_of_chars(self, chars_tuple: Tuple[str, ...]) -> bool:
        """Checks if the current character is one of the characters in the given tuple."""
        if not self.has_char():
            return False
        return self.curr_char in chars_tuple
    
    def _parse_numbers(self) -> Result[Token]:
        nums = []
        start_col = self.col
        
        dot_count = 0
        while (
            self.has_char() and 
            (self.curr_char.isdigit() or self.is_char_eq(".") or self.is_char_eq("_"))
        ):
            
            is_underline = self.curr_char == "_"
            is_dot = self.curr_char == "."

            if is_underline: 
                if not nums or not nums[-1].isdigit():
                     return self.create_error(
                        f"Invalid number format: '_' must be between digits at line {self.line}, column {self.col}",
                        Loc(self.line, self.col, self.col)
                    )
                self._advance()
                continue
            if is_dot: 
                if dot_count > 0:
                    return self.create_error(
                        f"Invalid number: multiple decimal points at line {self.line}, columns {start_col}-{self.col}",
                        Loc(self.line, start_col, self.col)
                    )
                dot_count += 1
            nums.append(self.curr_char)
            self._advance()
        
        loc = Loc(self.line, start_col, self.col -1 if nums else start_col)
        num_str = "".join(nums)

        if not num_str:
            return self.create_error("Expected a number.", loc)
        if num_str == ".":
             return self.create_error("Invalid number: isolated decimal point.", loc)
        if num_str.endswith("_"):
            return self.create_error("Invalid number: cannot end with '_'.", loc)

        ttype = TokenType.INT
        if dot_count == 1: 
            ttype = TokenType.FLOAT
            if num_str.startswith("."):
                num_str = "0" + num_str
            if num_str.endswith("."):
                num_str = num_str + "0"

        tok = self.get_tok(ttype, num_str, loc)
        return Result.ok(tok)

    def _parse_string(self) -> Result[Token]:
        strings = []
        start_col = self.col
        start_line = self.line
        start_quote = self.curr_char
        self._advance()

        while self.has_char() and not self.is_char_eq(start_quote):
            strings.append(self.curr_char)
            self._advance()

        if not self.has_char() or not self.is_char_eq(start_quote):
            return self.create_error(
                f"Unterminated string starting at line {start_line}:{start_col}, missing closing '{start_quote}'", 
                Loc(start_line, start_col, self.col -1 if self.has_char() else self.col)
            )
        
        self._advance()
        
        loc = Loc(start_line, start_col, self.col -1)
        tok = self.get_tok(
            TokenType.STRING, "".join(strings), 
            loc
        )
        return Result.ok(tok)
    
    def _parse_ident(self) -> Result[Token]:
        ident_chars = []
        start_col = self.col

        ident_chars.append(self.curr_char)
        self._advance()

        while self.has_char() and (self.curr_char.isalnum() or self.is_char_eq("_")):
            ident_chars.append(self.curr_char)
            self._advance()

        ident_str = "".join(ident_chars)
        loc = Loc(self.line, start_col, self.col -1)

        tok_kind = TokenType.IDENT
        if ident_str in KEYWORD_CONSTANTS:
            tok_kind = TokenType.KEYWORD
        
        tok = self.get_tok(tok_kind, ident_str, loc)
        return Result.ok(tok)