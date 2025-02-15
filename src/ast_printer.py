from .expr_visitor import Visitor
from .expr import Expr, Binary, Grouping, Literal, Unary, Ternary, Postfix
from .token import Token, TokenType


class AstPrinter(Visitor):
    def print(self, expr: Expr) -> str:
        return expr.accept(self)

    def visitBinaryExpr(self, expr: Binary) -> str:
        return f"({expr.left.accept(self)} {expr.operator.lexme} {expr.right.accept(self)})"

    def visitGroupingExpr(self, expr: Grouping) -> str:
        return f"({expr.expression.accept(self)})"

    def visitUnaryExpr(self, expr: Unary) -> str:
        return f"({expr.operator.lexme} {expr.expression.accept(self)})"

    def visitLiteralExpr(self, expr: Literal) -> str:
        return f"{expr.value}"

    def visitTernaryExpr(self, expr: Ternary) -> str:
        return f"({expr.one.accept(self)} {expr.op1.lexme} {expr.two.accept(self)} {expr.op2.lexme} {expr.three.accept(self)})"

    def visitPostfixExpr(self, expr: Postfix):
        return f"({expr.expression.accept(self)} {expr.operator.lexme})"

def main():
    expr = Binary(
        Unary(
            Token(TokenType.MINUS, '-', None, 1),
            Literal(123)
        ),
        Token(TokenType.STAR, '*', None, 1),
        Grouping(
            Literal(45.67)
        )
    )

    print(AstPrinter().print(expr))
