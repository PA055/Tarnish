from abc import abstractmethod
from typing import TYPE_CHECKING, Protocol

if TYPE_CHECKING:
    from .expr import Unary, Binary, Ternary, Postfix, Grouping, Literal


class Visitor(Protocol):
    @abstractmethod
    def visitUnaryExpr(self, expr: "Unary") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitBinaryExpr(self, expr: "Binary") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitTernaryExpr(self, expr: "Ternary") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitPostfixExpr(self, expr: "Postfix") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitGroupingExpr(self, expr: "Grouping") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitLiteralExpr(self, expr: "Literal") -> any:
        raise NotImplementedError

