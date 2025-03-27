from typing import Optional
from .token import Token, TokenType
from .error import error, TarnishParseError
from .grammar import (
    Expr,
    Binary,
    Unary,
    Literal,
    Grouping,
    Ternary,
    Postfix,
    Variable,
    Assign,
    Logical,
    Call,
    Func,
    Return,
    Lambda,
    Stmt,
    Print,
    Expression,
    Block,
    If,
    While,
    Var
)


class Parser:
    def __init__(self, tokens: list[Token]):
        self.tokens = tokens
        self.current: int = 0

    def parse(self) -> list[Optional[Stmt]]:
        statements: list[Optional[Stmt]] = []
        while not self.isAtEnd():
            statements.append(self.declaration())
        return statements

    def synchronize(self) -> None:
        self.advance()

        while not self.isAtEnd():
            if self.previous().tokenType == TokenType.SEMICOLON:
                return

            if self.peek().tokenType in (
                TokenType.CLASS,
                TokenType.ENUM,
                TokenType.FUNC,
                TokenType.FOR,
                TokenType.IF,
                TokenType.INTERFACE,
                TokenType.WHILE,
                TokenType.PRINT,
                TokenType.RETURN,
                TokenType.VAR,
            ):
                return

            self.advance()

    def declaration(self) -> Optional[Stmt]:
        try:
            if self.match(TokenType.FUNC):
                return self.funcDeclaration("function")
            if self.match(TokenType.VAR):
                return self.varDeclaration()
            return self.statement()
        except TarnishParseError:
            self.synchronize()
            return None

    def funcDeclaration(self, type: str) -> Stmt:
        name = self.consume(TokenType.IDENTIFIER, f"Expect {type} name.")
        self.consume(TokenType.LEFT_PAREN, f"Expect '(' after {type} name.")

        parameters: list[Token] = []
        if not self.check(TokenType.RIGHT_PAREN):
            parameters.append(self.consume(TokenType.IDENTIFIER, "Expect parameter name."))
            while self.match(TokenType.COMMA):
                if len(parameters) >= 255:
                    self.error(self.peek(), "Can't have more than 255 arguments.")
                parameters.append(self.consume(TokenType.IDENTIFIER, "Expect parameter name."))
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after arguments.")

        self.consume(TokenType.LEFT_BRACE, f"Expect '{{' before {type} body.")
        body: list[Optional[Stmt]] = self.block()
        return Func(name, parameters, body)

    def varDeclaration(self) -> Stmt:
        name = self.consume(TokenType.IDENTIFIER, "Expected variable name.")

        initializer = None
        if self.match(TokenType.EQUAL):
            initializer = self.expression()

        if not self.isAtEnd():
            self.consume(TokenType.SEMICOLON, "Expect ';' after expression.")

        return Var(name, initializer)

    def statement(self) -> Stmt:
        if self.match(TokenType.IF):
            return self.ifStatement()

        if self.match(TokenType.VAR):
            return self.varDeclaration()

        if self.match(TokenType.WHILE):
            return self.whileStatement()

        if self.match(TokenType.FOR):
            return self.forStatement()

        if self.match(TokenType.PRINT):
            return self.printStatement()

        if self.match(TokenType.RETURN):
            return self.returnStatement()

        if self.match(TokenType.LEFT_BRACE):
            return Block(self.block())

        return self.expressionStatement()

    def block(self) -> list[Optional[Stmt]]:
        statements: list[Optional[Stmt]] = []
        while not self.check(TokenType.RIGHT_BRACE) and not self.isAtEnd():
            statements.append(self.declaration())

        self.consume(TokenType.RIGHT_BRACE, "Expect '}' after block.")
        return statements

    def ifStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'if'.")
        condition: Expr = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after if condition.")

        thenBranch: Stmt = self.statement()
        elseBranch: Stmt | None = None
        if self.match(TokenType.ELSE):
            elseBranch = self.statement()

        return If(condition, thenBranch, elseBranch)

    def forStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'for'.")

        if self.match(TokenType.SEMICOLON):
            initializer = None
        elif self.match(TokenType.VAR):
            initializer = self.varDeclaration()
        else:
            initializer = self.expressionStatement()

        condition = None
        if not self.check(TokenType.SEMICOLON):
            condition = self.expression()
        self.consume(TokenType.SEMICOLON, "Expect ';' after loop condition.")

        increment = None
        if not self.check(TokenType.RIGHT_PAREN):
            increment = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after for clauses.")

        body = self.statement()
        if increment is not None:
            body = Block([body, Expression(increment)])
        if condition is None:
            condition = Literal(True)
        body = While(condition, body)
        if initializer is not None:
            body = Block([initializer, body])

        return body

    def whileStatement(self) -> Stmt:
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after 'while'.")
        condition: Expr = self.expression()
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after condition.")
        body: Stmt = self.statement()
        return While(condition, body)

    def printStatement(self) -> Stmt:
        value = self.expression()
        if not self.isAtEnd():
            self.consume(TokenType.SEMICOLON, "Expect ';' after expression.")
        return Print(value)

    def returnStatement(self) -> Stmt:
        keyword = self.previous()

        value = None
        if not self.check(TokenType.SEMICOLON):
            value = self.expression()

        if not self.isAtEnd():
            self.consume(TokenType.SEMICOLON, "Expect ';' after expression.")
        return Return(keyword, value)

    def expressionStatement(self) -> Stmt:
        value = self.expression()
        if not self.isAtEnd():
            self.consume(TokenType.SEMICOLON, "Expect ';' after expression.")
        return Expression(value)

    def expression(self) -> Expr:
        return self.assignment()

    def comma(self) -> Expr:  # TODO: Make more like python
        expr: Expr = self.assignment()
        while self.match(TokenType.COMMA):
            operator: Token = self.previous()
            right: Expr = self.ternary()
            expr = Binary(expr, operator, right)
        return expr

    def assignment(self) -> Expr:
        expr = self.ternary()

        if self.match(TokenType.EQUAL):
            equals = self.previous()
            value = self.assignment()

            if isinstance(expr, Variable):
                return Assign(expr.name, value)
            self.error(equals, "Invalid assignment target.")

        return expr

    def ternary(self) -> Expr:
        expr: Expr = self.lor()
        if self.match(TokenType.QUESTION):
            op1: Token = self.previous()
            two: Expr = self.ternary()
            op2: Token = self.consume(
                TokenType.COLON, "expected ':' in ternary")
            three: Expr = self.ternary()
            return Ternary(expr, op1, two, op2, three)
        # TODO: the python style a > b > c
        return expr

    def lor(self) -> Expr:
        expr: Expr = self.land()
        while self.match(TokenType.BAR_BAR):
            operator: Token = self.previous()
            right: Expr = self.land()
            expr = Logical(expr, operator, right)
        return expr

    def land(self) -> Expr:
        expr: Expr = self.equality()
        while self.match(TokenType.AMPERSAND_AMPERSAND):
            operator: Token = self.previous()
            right: Expr = self.equality()
            expr = Logical(expr, operator, right)
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
        expr: Expr = self.call()
        if self.match(TokenType.PLUS_PLUS, TokenType.MINUS_MINUS):
            operator: Token = self.previous()
            return Postfix(operator, expr)
        return expr

    def call(self) -> Expr:
        expr = self.lambdaFn()
        while True:
            if self.match(TokenType.LEFT_PAREN):
                arguments: list[Expr] = []
                if not self.check(TokenType.RIGHT_PAREN):
                    arguments.append(self.expression())
                    while self.match(TokenType.COMMA):
                        if len(arguments) >= 255:
                            self.error(self.peek(), "Can't have more than 255 arguments.")
                        arguments.append(self.expression())
                paren = self.consume(TokenType.RIGHT_PAREN, "Expect ')' after arguments.")
                expr = Call(expr, paren, arguments)
            else:
                break
        return expr

    def lambdaFn(self) -> Expr:
        if not self.match(TokenType.LAMBDA):
            return self.primary()
        self.consume(TokenType.LEFT_PAREN, "Expect '(' after lambda")
        params: list[Token] = []
        if not self.check(TokenType.RIGHT_PAREN):
            params.append(self.consume(TokenType.IDENTIFIER, "Expect parameter name."))
            while self.match(TokenType.COMMA):
                if len(params) >= 255:
                    self.error(self.peek(), "Can't have more than 255 arguments.")
                params.append(self.consume(TokenType.IDENTIFIER, "Expect parameter name."))
        self.consume(TokenType.RIGHT_PAREN, "Expect ')' after arguments.")
        body = self.statement()
        return Lambda(params, body)

    def primary(self) -> Expr:
        if self.match(TokenType.FALSE):
            return Literal(False)
        if self.match(TokenType.TRUE):
            return Literal(True)
        if self.match(TokenType.NONE):
            return Literal(None)

        if self.match(TokenType.NUMBER, TokenType.STRING):
            return Literal(self.previous().literal)

        if self.match(TokenType.IDENTIFIER):
            return Variable(self.previous())

        if self.match(TokenType.LEFT_PAREN):
            expr: Expr = self.expression()
            self.consume(TokenType.RIGHT_PAREN,
                         "Expected ')' after expression.")
            return Grouping(expr)

        raise self.error(self.peek(), "Expect expression.")

    def match(self, *types: TokenType) -> bool:
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

    def error(self, token: Token, message: str) -> TarnishParseError:
        error(token, message)
        return TarnishParseError()

    def previous(self) -> Token:
        return self.tokens[self.current - 1]

    def isAtEnd(self) -> bool:
        return self.peek().tokenType == TokenType.EOF
