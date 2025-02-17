from abc import ABC, abstractmethod
from dataclasses import dataclass
import typing
from .token import Token


class ExprVisitor(typing.Protocol):
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

    @abstractmethod
    def visitVariableExpr(self, expr: "Variable") -> any:
        raise NotImplementedError


class Expr(ABC):
    @abstractmethod
    def accept(self, visitor: ExprVisitor) -> any:
        raise NotImplementedError


@dataclass(frozen=True)
class Unary(Expr):
    operator: Token
    expression: Expr

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitUnaryExpr(self)


@dataclass(frozen=True)
class Binary(Expr):
    left: Expr
    operator: Token
    right: Expr

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitBinaryExpr(self)


@dataclass(frozen=True)
class Ternary(Expr):
    one: Expr
    op1: Token
    two: Expr
    op2: Token
    three: Expr

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitTernaryExpr(self)


@dataclass(frozen=True)
class Postfix(Expr):
    operator: Token
    expression: Expr

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitPostfixExpr(self)


@dataclass(frozen=True)
class Grouping(Expr):
    expression: Expr

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitGroupingExpr(self)


@dataclass(frozen=True)
class Literal(Expr):
    value: any

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitLiteralExpr(self)


@dataclass(frozen=True)
class Variable(Expr):
    name: Token

    def accept(self, visitor: ExprVisitor) -> any:
        return visitor.visitVariableExpr(self)


class StmtVisitor(typing.Protocol):
    @abstractmethod
    def visitPrintStmt(self, stmt: "Print") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitExpressionStmt(self, stmt: "Expression") -> any:
        raise NotImplementedError

    @abstractmethod
    def visitVarStmt(self, stmt: "Var") -> any:
        raise NotImplementedError


class Stmt(ABC):
    @abstractmethod
    def accept(self, visitor: StmtVisitor) -> any:
        raise NotImplementedError


@dataclass(frozen=True)
class Print(Stmt):
    value: Expr

    def accept(self, visitor: StmtVisitor) -> any:
        return visitor.visitPrintStmt(self)


@dataclass(frozen=True)
class Expression(Stmt):
    value: Expr

    def accept(self, visitor: StmtVisitor) -> any:
        return visitor.visitExpressionStmt(self)


@dataclass(frozen=True)
class Var(Stmt):
    name: Token
    value: Expr

    def accept(self, visitor: StmtVisitor) -> any:
        return visitor.visitVarStmt(self)


