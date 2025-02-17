from typing import Tuple
from .grammar import Expr, Ternary, Binary, Unary, Postfix, Literal, Grouping, Variable, \
                     Stmt, Print, Expression, Var, \
                     ExprVisitor, StmtVisitor
from .token import TokenType, Token
from .error import TarnishRuntimeError, runtimeError
from .environment import Environment


class Interpreter(ExprVisitor, StmtVisitor):
    def __init__(self):
        self.environment = Environment()

    def assertInteger(self, operator: Token, *values: Tuple[any, ...]) -> None:
        if not all(isinstance(v, int) for v in values):
            raise TarnishRuntimeError(operator, "Operand must be an integer.")

    def assertNumeric(self, operator: Token, *values: Tuple[any, ...]) -> None:
        if not all(isinstance(v, (int, float)) for v in values):
            raise TarnishRuntimeError(operator, "Operand must be a number.")

    def interpret(self, statements: list[Stmt]):
        try:
            for statement in statements:
                self.execute(statement)
        except TarnishRuntimeError as e:
            runtimeError(e)

    def evaluate(self, expr: Expr) -> any:
        return expr.accept(self)

    def execute(self, stmt: Stmt) -> any:
        return stmt.accept(self)

    def visitLiteralExpr(self, expr: Literal) -> any:
        return expr.value

    def visitVariableExpr(self, expr: Variable) -> any:
        return self.environment.get(expr.name)

    def visitGroupingExpr(self, expr: Grouping) -> any:
        return self.evaluate(expr.expression)

    def visitUnaryExpr(self, expr: Unary) -> any:
        right = self.evaluate(expr.expression)

        if expr.operator.tokenType == TokenType.MINUS:
            return -float(right)
        elif expr.operator.tokenType == TokenType.PLUS:
            return float(right)

        elif expr.operator.tokenType == TokenType.TILDE:
            return ~int(right)
        elif expr.operator.tokenType == TokenType.BANG:
            return not bool(right)

        elif expr.operator.tokenType == TokenType.PLUS_PLUS:
            return int(right) + 1
        elif expr.operator.tokenType == TokenType.MINUS_MINUS:
            return int(right) - 1

        return None

    def visitPostfixExpr(self, expr: Postfix) -> any:
        return self.evaluate(expr.expression)

    def visitBinaryExpr(self, expr: Binary) -> any:
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

        elif expr.operator.tokenType == TokenType.PERCENT:
            self.assertNumeric(left, right)
            return left % right
        elif expr.operator.tokenType == TokenType.SLASH:
            self.assertNumeric(left, right)
            if right == 0:
                raise TarnishRuntimeError(expr.operator, "Cannot divide by zero.")
            return left / right
        elif expr.operator.tokenType == TokenType.STAR:
            self.assertNumeric(left, right)
            return left * right
        elif expr.operator.tokenType == TokenType.STAR_STAR:
            self.assertNumeric(left, right)
            return left ** right
        elif expr.operator.tokenType == TokenType.MINUS:
            self.assertNumeric(left, right)
            return left - right
        elif expr.operator.tokenType == TokenType.PLUS:
            if isinstance(left, (float, int)) and isinstance(right, (float, int)):
                return left + right
            elif isinstance(left, str) or isinstance(right, str):
                return str(left) + str(right)
            else:
                raise TarnishRuntimeError(expr.operator, "Operands must be two numbers or include a string")

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

    def visitTernaryExpr(self, expr: Ternary) -> any:
        one = self.evaluate(expr.one)

        if (expr.op1.tokenType == TokenType.QUESTION and
                expr.op2.tokenType == TokenType.COLON):
            return self.evaluate(expr.two) if bool(one) else self.evaluate(expr.three)

        return None

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

    def visitExpressionStmt(self, stmt: Expression):
        self.evaluate(stmt.value)

    def visitVarStmt(self, stmt: Var):
        value = None
        if stmt.value is not None:
            value = self.evaluate(stmt.value)

        self.environment.define(stmt.name.lexme, value)
