from abc import ABC, abstractmethod
from dataclasses import dataclass
import typing
from .expr_visitor import Visitor

if typing.TYPE_CHECKING:
    from .token import Token


class Expr(ABC):
    @abstractmethod
    def accept(self, visitor: "Visitor") -> any:
        raise NotImplementedError


@dataclass(frozen=True)
class Unary(Expr):
    operator: "Token"
    expression: Expr

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitUnaryExpr(self)


@dataclass(frozen=True)
class Binary(Expr):
    left: Expr
    operator: "Token"
    right: Expr

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitBinaryExpr(self)


@dataclass(frozen=True)
class Ternary(Expr):
    one: Expr
    op1: "Token"
    two: Expr
    op2: "Token"
    three: Expr

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitTernaryExpr(self)


@dataclass(frozen=True)
class Postfix(Expr):
    operator: "Token"
    expression: Expr

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitPostfixExpr(self)


@dataclass(frozen=True)
class Grouping(Expr):
    expression: Expr

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitGroupingExpr(self)


@dataclass(frozen=True)
class Literal(Expr):
    value: any

    def accept(self, visitor: "Visitor") -> any:
        return visitor.visitLiteralExpr(self)


