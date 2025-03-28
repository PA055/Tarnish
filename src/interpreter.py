from typing import Dict, Tuple, Any

from .token import TokenType, Token
from .error import TarnishRuntimeError, runtimeError
from .environment import Environment
from .callable import TarnishCallable
from .function import TarnishFunction
from .interupts import BreakInterupt, ContinueInterupt, ReturnInterupt
from .builtins.time import Time
from .builtins.string import String
from .grammar import (
    Assign,
    Binary,
    Block,
    Call,
    Expr,
    Expression,
    ExprVisitor,
    Func,
    Grouping,
    If,
    Lambda,
    List,
    Literal,
    Logical,
    LoopInterupt,
    Postfix,
    Prefix,
    Print,
    Return,
    Stmt,
    StmtVisitor,
    Ternary,
    Unary,
    Var,
    Variable,
    While,
)


class Interpreter(ExprVisitor, StmtVisitor):
    globals = Environment()
    globals.define("time", Time())
    globals.define("str", String())

    def __init__(self):
        self.environment = Environment(Interpreter.globals)
        self.locals: Dict[Expr, int] = {}

    def assertInteger(self, operator: Token, *values: Tuple[Any, ...]) -> None:
        if not all(isinstance(v, int) for v in values):
            raise TarnishRuntimeError(operator, "Operand must be an integer.")

    def assertNumeric(self, operator: Token, *values: Tuple[Any, ...]) -> None:
        if not all(isinstance(v, (int, float)) for v in values):
            raise TarnishRuntimeError(operator, "Operand must be a number.")

    def interpret(self, statements: list[Stmt]):
        try:
            for statement in statements:
                self.execute(statement)
        except TarnishRuntimeError as e:
            runtimeError(e)

    def lookUpVariable(self, name: Token, expr: Expr) -> Any:
        distance = self.locals.get(expr, None)
        if distance is not None:
            return self.environment.getAt(distance, name)
        else:
            return self.environment.get(name)

    def evaluate(self, expr: Expr) -> Any:
        return expr.accept(self)

    def execute(self, stmt: Stmt) -> Any:
        return stmt.accept(self)

    def resolve(self, expr: Expr, depth: int):
        self.locals[expr] = depth

    def executeBlock(self, statements: list[Stmt], environment: Environment):
        previous: Environment = self.environment
        try:
            self.environment = environment
            for statement in statements:
                self.execute(statement)
        finally:
            self.environment = previous

    def visitLiteralExpr(self, expr: Literal) -> Any:
        return expr.value

    def visitVariableExpr(self, expr: Variable) -> Any:
        return self.lookUpVariable(expr.name, expr)

    def visitAssignExpr(self, expr: Assign) -> Any:
        value = self.evaluate(expr.value)

        distance = self.locals.get(expr, None)
        if distance is not None:
            self.environment.assignAt(distance, expr.name, value)
        else:
            self.environment.assign(expr.name, value)

        return value

    def visitGroupingExpr(self, expr: Grouping) -> Any:
        return self.evaluate(expr.expression)

    def visitCallExpr(self, expr: Call) -> Any:
        callee = self.evaluate(expr.callee)

        arguments: list[Any] = []
        for arg in expr.arguments:
            arguments.append(self.evaluate(arg))

        if not isinstance(callee, TarnishCallable):
            raise TarnishRuntimeError(expr.paren, "Can only call functions and classes.")

        if len(arguments) != callee.arity():
            raise TarnishRuntimeError(expr.paren, f"Expected {callee.arity()} arguments but got {len(arguments)}.")

        return callee.call(self, arguments)

    def visitLambdaExpr(self, expr: Lambda):
        function = TarnishFunction(expr, self.environment)
        return function

    def visitUnaryExpr(self, expr: Unary) -> Any:
        right = self.evaluate(expr.expression)

        if expr.operator.tokenType == TokenType.MINUS:
            return -float(right)
        elif expr.operator.tokenType == TokenType.PLUS:
            return float(right)

        elif expr.operator.tokenType == TokenType.TILDE:
            return ~int(right)
        elif expr.operator.tokenType == TokenType.BANG:
            return not bool(right)

        # TODO: seperate this, and add += and -=
        elif expr.operator.tokenType == TokenType.PLUS_PLUS:
            return int(right)
        elif expr.operator.tokenType == TokenType.MINUS_MINUS:
            return int(right)

        return None

    def visitPrefixExpr(self, expr: Prefix) -> Any:
        value = self.lookUpVariable(expr.name, expr) + (1 if expr.operator.tokenType == TokenType.PLUS_PLUS else -1)

        distance = self.locals.get(expr, None)
        if distance is not None:
            self.environment.assignAt(distance, expr.name, value)
        else:
            self.environment.assign(expr.name, value)

        return value

    def visitPostfixExpr(self, expr: Postfix) -> Any:
        mod = 1 if expr.operator.tokenType == TokenType.PLUS_PLUS else -1
        value = self.lookUpVariable(expr.name, expr) + mod

        distance = self.locals.get(expr, None)
        if distance is not None:
            self.environment.assignAt(distance, expr.name, value)
        else:
            self.environment.assign(expr.name, value)

        return value - mod

    def visitLogicalExpr(self, expr: Logical) -> Any:
        left = self.evaluate(expr.left)
        if expr.operator.tokenType == TokenType.BAR_BAR:
            if left:
                return left
        else:
            if not left:
                return left
        return self.evaluate(expr.right)

    def visitBinaryExpr(self, expr: Binary) -> Any:
        left = self.evaluate(expr.left)
        right = self.evaluate(expr.right)

        if expr.operator.tokenType == TokenType.COMMA:
            return right

        elif expr.operator.tokenType == TokenType.EQUAL_EQUAL:
            return left == right
        elif expr.operator.tokenType == TokenType.BANG_EQUAL:
            return left != right

        elif expr.operator.tokenType == TokenType.CARET:
            self.assertInteger(left, right)
            return left ^ right
        elif expr.operator.tokenType == TokenType.BAR:
            self.assertInteger(left, right)
            return left | right
        elif expr.operator.tokenType == TokenType.AMPERSAND:
            self.assertInteger(left, right)
            return left & right
        elif expr.operator.tokenType == TokenType.GREATER_GREATER:
            self.assertInteger(left, right)
            return left >> right
        elif expr.operator.tokenType == TokenType.LESS_LESS:
            self.assertInteger(left, right)
            return left << right

        elif expr.operator.tokenType == TokenType.PERCENT:
            self.assertNumeric(left, right)
            return left % right
        elif expr.operator.tokenType == TokenType.SLASH:
            self.assertNumeric(left, right)
            if right == 0:
                raise TarnishRuntimeError(
                    expr.operator, "Cannot divide by zero.")
            return left / right
        elif expr.operator.tokenType == TokenType.STAR:
            self.assertNumeric(left, right)
            return left * right
        elif expr.operator.tokenType == TokenType.STAR_STAR:
            self.assertNumeric(left, right)
            return left**right
        elif expr.operator.tokenType == TokenType.MINUS:
            self.assertNumeric(left, right)
            return left - right
        elif expr.operator.tokenType == TokenType.PLUS:
            if isinstance(left, (float, int)) and isinstance(right, (float, int)):
                return left + right
            elif isinstance(left, str) or isinstance(right, str):
                return str(left) + str(right)
            else:
                raise TarnishRuntimeError(
                    expr.operator, "Operands must be two numbers or include a string"
                )

        elif expr.operator.tokenType == TokenType.LESS:
            self.assertNumeric(left, right)
            return left < right
        elif expr.operator.tokenType == TokenType.LESS_EQUAL:
            self.assertNumeric(left, right)
            return left <= right
        elif expr.operator.tokenType == TokenType.GREATER:
            self.assertNumeric(left, right)
            return left > right
        elif expr.operator.tokenType == TokenType.GREATER_EQUAL:
            self.assertNumeric(left, right)
            return left >= right

        return None

    def visitTernaryExpr(self, expr: Ternary) -> Any:
        one = self.evaluate(expr.one)

        if expr.op1.tokenType == TokenType.QUESTION and expr.op2.tokenType == TokenType.COLON:
            return self.evaluate(expr.two) if bool(one) else self.evaluate(expr.three)

        return None

    def visitListExpr(self, expr: List) -> Any:
        pass

    def visitReturnStmt(self, stmt: Return) -> Any:
        value = None
        if stmt.value is not None:
            value = self.evaluate(stmt.value)
        raise ReturnInterupt(value)

    def visitLoopInteruptStmt(self, stmt: LoopInterupt):
        if stmt.keyword.tokenType == TokenType.CONTINUE:
            raise ContinueInterupt()
        elif stmt.keyword.tokenType == TokenType.BREAK:
            raise BreakInterupt(stmt.value)

    def visitPrintStmt(self, stmt: Print):
        value = self.evaluate(stmt.value)
        if value is True:
            print("true")
        elif value is False:
            print("false")
        elif value is None:
            print("none")
        else:
            print(value)

    def visitVarStmt(self, stmt: Var) -> Any:
        value = None
        if stmt.value is not None:
            value = self.evaluate(stmt.value)
        self.environment.define(stmt.name, value)

    def visitExpressionStmt(self, stmt: Expression):
        self.evaluate(stmt.value)

    def visitBlockStmt(self, stmt: Block):
        self.executeBlock(stmt.statements, Environment(self.environment))

    def visitIfStmt(self, stmt: If):
        if self.evaluate(stmt.condition):
            self.execute(stmt.thenBranch)
        elif stmt.elseBranch is not None:
            self.execute(stmt.elseBranch)

    def visitWhileStmt(self, stmt: While):
        while self.evaluate(stmt.condition):
            try:
                self.execute(stmt.body)
            except BreakInterupt as e:
                if e.number > 1:
                    raise BreakInterupt(e.number - 1)
                break
            except ContinueInterupt:
                if stmt.for_transformed and isinstance(stmt.body, Block):
                    self.executeBlock([stmt.body.statements[-1]], Environment(self.environment))
                    continue
                continue

    def visitFuncStmt(self, stmt: Func):
        function = TarnishFunction(stmt, self.environment)
        self.environment.define(stmt.name.lexme, function)
