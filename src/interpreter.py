from .expr_visitor import Visitor
from .expr import Expr, Ternary, Binary, Unary, Postfix, Literal, Grouping
from .token import TokenType


class Interpreter(Visitor):
    def evaluate(self, expr: Expr) -> any:
        return expr.accept(self)

    def visitLiteralExpr(self, expr: Literal) -> any:
        return expr.value

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
            return int(left) ^ int(right)
        elif expr.operator.tokenType == TokenType.BAR:
            return int(left) | int(right)
        elif expr.operator.tokenType == TokenType.AMPERSAND:
            return int(left) & int(right)

        elif expr.operator.tokenType == TokenType.PERCENT:
            return float(left) % float(right)
        elif expr.operator.tokenType == TokenType.SLASH:
            return float(left) / float(right)
        elif expr.operator.tokenType == TokenType.STAR:
            return float(left) * float(right)
        elif expr.operator.tokenType == TokenType.STAR_STAR:
            return float(left) ** float(right)
        elif expr.operator.tokenType == TokenType.PLUS:
            return float(left) + float(right)
        elif expr.operator.tokenType == TokenType.MINUS:
            return float(left) - float(right)

        elif expr.operator.tokenType == TokenType.LESS:
            return float(left) < float(right)
        elif expr.operator.tokenType == TokenType.LESS_EQUAL:
            return float(left) <= float(right)
        elif expr.operator.tokenType == TokenType.GREATER:
            return float(left) > float(right)
        elif expr.operator.tokenType == TokenType.GREATER_EQUAL:
            return float(left) >= float(right)

        return None

    def visitTernaryExpr(self, expr: Ternary) -> any:
        one = self.evaluate(expr.one)

        if (expr.op1.tokenType == TokenType.QUESTION and
                expr.op2.tokenType == TokenType.COLON):
            return self.evaluate(expr.two) if bool(one) else self.evaluate(expr.three)

        return None
