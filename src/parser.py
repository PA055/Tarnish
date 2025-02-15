from typing import Tuple
from .token import Token, TokenType
from .expr import Expr, Binary, Unary, Literal, Grouping, Ternary, Postfix
from .error import error, ParseError


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.current: int = 0

    def match(self, *types: Tuple[TokenType, ...]) -> bool:
        for tokenType in types:
            if (self.check(tokenType)):
                self.advance()
                return True
        return False

    def check(self, tokenType: TokenType) -> bool:
        if (self.isAtEnd()):
            return False
        return self.peek().tokenType == tokenType

    def peek(self) -> Token:
        return self.tokens[self.current]

    def advance(self) -> Token:
        if not self.isAtEnd():
            self.current += 1
        return self.previous()

    def consume(self, tokenType: TokenType, message: str) -> Token:
        if self.check(tokenType):
            return self.advance()
        raise self.error(self.peek(), message)

    def error(self, token: Token, message: str) -> ParseError:
        error(token, message)
        return ParseError()

    def previous(self) -> Token:
        return self.tokens[self.current - 1]

    def isAtEnd(self) -> bool:
        return self.peek().tokenType == TokenType.EOF

    def synchronize(self) -> None:
        self.advance()

        while not self.isAtEnd():
            if self.previous().tokenType == TokenType.SEMICOLON:
                return

            next: Token = self.peek().tokenType

            if next == TokenType.CLASS:
                return
            elif next == TokenType.FUNC:
                return
            elif next == TokenType.VAR:
                return
            elif next == TokenType.FOR:
                return
            elif next == TokenType.IF:
                return
            elif next == TokenType.WHILE:
                return
            elif next == TokenType.PRINT:
                return
            elif next == TokenType.RETURN:
                return

    def expression(self) -> Expr:
        return self.comma()

    def comma(self) -> Expr:
        expr: Expr = self.ternary()
        while self.match(TokenType.COMMA):
            operator: Token = self.previous()
            right: Expr = self.ternary()
            expr = Binary(expr, operator, right)
        return expr

    def ternary(self) -> Expr:
        expr: Expr = self.lor()
        if self.match(TokenType.QUESTION):
            op1: Token = self.previous()
            two: Expr = self.ternary()
            op2: Token = self.consume(TokenType.COLON, "expected ':' in ternary")
            three: Expr = self.ternary()
            return Ternary(expr, op1, two, op2, three)
        # TODO: the python style a > b > c
        return expr

    def lor(self) -> Expr:
        expr: Expr = self.land()
        while self.match(TokenType.BAR_BAR):
            operator: Token = self.previous()
            right: Expr = self.land()
            expr = Binary(expr, operator, right)
        return expr

    def land(self) -> Expr:
        expr: Expr = self.equality()
        while self.match(TokenType.AMPERSAND_AMPERSAND):
            operator: Token = self.previous()
            right: Expr = self.equality()
            expr = Binary(expr, operator, right)
        return expr

    def equality(self) -> Expr:
        expr: Expr = self.comparison()
        while self.match(TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL):
            operator: Token = self.previous()
            right: Expr = self.comparison()
            expr = Binary(expr, operator, right)
        return expr

    def comparison(self) -> Expr:
        expr: Expr = self.bor()
        while self.match(TokenType.GREATER, TokenType.GREATER_EQUAL,
                         TokenType.LESS, TokenType.LESS_EQUAL):
            operator: Token = self.previous()
            right: Expr = self.bor()
            expr = Binary(expr, operator, right)
        return expr

    def bor(self) -> Expr:
        expr: Expr = self.bxor()
        while self.match(TokenType.BAR):
            operator: Token = self.previous()
            right: Expr = self.bxor()
            expr = Binary(expr, operator, right)
        return expr

    def bxor(self) -> Expr:
        expr: Expr = self.band()
        while self.match(TokenType.CARET):
            operator: Token = self.previous()
            right: Expr = self.band()
            expr = Binary(expr, operator, right)
        return expr

    def band(self) -> Expr:
        expr: Expr = self.bshift()
        while self.match(TokenType.AMPERSAND):
            operator: Token = self.previous()
            right: Expr = self.bshift()
            expr = Binary(expr, operator, right)
        return expr

    def bshift(self) -> Expr:
        expr: Expr = self.term()
        while self.match(TokenType.GREATER_GREATER, TokenType.LESS_LESS):
            operator: Token = self.previous()
            right: Expr = self.term()
            expr = Binary(expr, operator, right)
        return expr

    def term(self) -> Expr:
        expr: Expr = self.factor()
        while self.match(TokenType.PLUS, TokenType.MINUS):
            operator: Token = self.previous()
            right: Expr = self.factor()
            expr = Binary(expr, operator, right)
        return expr

    def factor(self) -> Expr:
        expr: Expr = self.unary()
        while self.match(TokenType.SLASH, TokenType.STAR, TokenType.PERCENT):
            operator: Token = self.previous()
            right: Expr = self.unary()
            expr = Binary(expr, operator, right)
        return expr

    def unary(self) -> Expr:
        if self.match(TokenType.MINUS, TokenType.BANG,
                      TokenType.PLUS, TokenType.TILDE,
                      TokenType.PLUS_PLUS, TokenType.MINUS_MINUS):
            operator: Token = self.previous()
            right: Expr = self.unary()
            return Unary(operator, right)
        return self.exponent()

    def exponent(self) -> Expr:
        expr: Expr = self.postfix()
        if self.match(TokenType.STAR_STAR):
            operator: Token = self.previous()
            right: Expr = self.exponent()
            return Binary(expr, operator, right)
        return expr

    def postfix(self) -> Expr:
        expr: Expr = self.primary()
        if self.match(TokenType.PLUS_PLUS, TokenType.MINUS_MINUS):
            operator: Token = self.previous()
            return Postfix(operator, expr)
        return expr

    def primary(self) -> Expr:
        if self.match(TokenType.FALSE):
            return Literal(False)
        if self.match(TokenType.TRUE):
            return Literal(True)
        if self.match(TokenType.NONE):
            return Literal(None)

        if self.match(TokenType.NUMBER, TokenType.STRING):
            return Literal(self.previous().literal)

        if self.match(TokenType.LEFT_PAREN):
            expr: Expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN, "Expected ')' after expression.")
            return Grouping(expr)

        raise self.error(self.peek(), "Expect expression.")

    def parse(self) -> Expr:
        try:
            return self.expression()
        except ParseError:
            return None
