from abc import ABC, abstractmethod
from dataclasses import dataclass
import typing
from .token import Token


class ExprVisitor(typing.Protocol):
    @abstractmethod
    def visitAssignExpr(self, expr: "Assign") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitBinaryExpr(self, expr: "Binary") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitCallExpr(self, expr: "Call") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitGetExpr(self, expr: "Get") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitGroupingExpr(self, expr: "Grouping") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitLambdaExpr(self, expr: "Lambda") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitListExpr(self, expr: "List") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitLiteralExpr(self, expr: "Literal") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitLogicalExpr(self, expr: "Logical") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitPostfixExpr(self, expr: "Postfix") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitPrefixExpr(self, expr: "Prefix") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitSetExpr(self, expr: "Set") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitSuperExpr(self, expr: "Super") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitTernaryExpr(self, expr: "Ternary") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitThisExpr(self, expr: "This") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitUnaryExpr(self, expr: "Unary") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitVariableExpr(self, expr: "Variable") -> typing.Any:
        raise NotImplementedError


class Expr(ABC):
    @abstractmethod
    def accept(self, visitor: ExprVisitor) -> typing.Any:
        raise NotImplementedError


@dataclass(frozen=True)
class Assign(Expr):
    name: Token
    operator: Token
    value: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitAssignExpr(self)


@dataclass(frozen=True)
class Binary(Expr):
    left: Expr
    operator: Token
    right: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitBinaryExpr(self)


@dataclass(frozen=True)
class Call(Expr):
    callee: Expr
    paren: Token
    arguments: typing.Tuple[Expr, ...]

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitCallExpr(self)


@dataclass(frozen=True)
class Get(Expr):
    object: Expr
    name: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitGetExpr(self)


@dataclass(frozen=True)
class Grouping(Expr):
    expression: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitGroupingExpr(self)


@dataclass(frozen=True)
class Lambda(Expr):
    params: typing.Tuple[Token, ...]
    body: "Stmt"

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitLambdaExpr(self)


@dataclass(frozen=True)
class List(Expr):
    expressions: typing.Tuple[Expr, ...]

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitListExpr(self)


@dataclass(frozen=True)
class Literal(Expr):
    value: typing.Any

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitLiteralExpr(self)


@dataclass(frozen=True)
class Logical(Expr):
    left: Expr
    operator: Token
    right: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitLogicalExpr(self)


@dataclass(frozen=True)
class Postfix(Expr):
    operator: Token
    name: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitPostfixExpr(self)


@dataclass(frozen=True)
class Prefix(Expr):
    operator: Token
    name: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitPrefixExpr(self)


@dataclass(frozen=True)
class Set(Expr):
    object: Expr
    name: Token
    value: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitSetExpr(self)


@dataclass(frozen=True)
class Super(Expr):
    keyword: Token
    method: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitSuperExpr(self)


@dataclass(frozen=True)
class Ternary(Expr):
    one: Expr
    op1: Token
    two: Expr
    op2: Token
    three: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitTernaryExpr(self)


@dataclass(frozen=True)
class This(Expr):
    keyword: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitThisExpr(self)


@dataclass(frozen=True)
class Unary(Expr):
    operator: Token
    expression: Expr

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitUnaryExpr(self)


@dataclass(frozen=True)
class Variable(Expr):
    name: Token

    def accept(self, visitor: ExprVisitor) -> typing.Any:
        return visitor.visitVariableExpr(self)


class StmtVisitor(typing.Protocol):
    @abstractmethod
    def visitBlockStmt(self, stmt: "Block") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitClassStmt(self, stmt: "Class") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitExpressionStmt(self, stmt: "Expression") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitFuncStmt(self, stmt: "Func") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitIfStmt(self, stmt: "If") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitLoopInteruptStmt(self, stmt: "LoopInterupt") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitPrintStmt(self, stmt: "Print") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitReturnStmt(self, stmt: "Return") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitVarStmt(self, stmt: "Var") -> typing.Any:
        raise NotImplementedError

    @abstractmethod
    def visitWhileStmt(self, stmt: "While") -> typing.Any:
        raise NotImplementedError


class Stmt(ABC):
    @abstractmethod
    def accept(self, visitor: StmtVisitor) -> typing.Any:
        raise NotImplementedError


@dataclass(frozen=True)
class Block(Stmt):
    statements: typing.Tuple[Stmt, ...]

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitBlockStmt(self)


@dataclass(frozen=True)
class Class(Stmt):
    name: Token
    methods: typing.Tuple["Func", ...]
    superclass: typing.Optional[Variable]

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitClassStmt(self)


@dataclass(frozen=True)
class Expression(Stmt):
    value: Expr

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitExpressionStmt(self)


@dataclass(frozen=True)
class Func(Stmt):
    name: Token
    params: typing.Tuple[Token, ...]
    body: typing.Tuple[typing.Optional[Stmt], ...]

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitFuncStmt(self)


@dataclass(frozen=True)
class If(Stmt):
    condition: Expr
    thenBranch: Stmt
    elseBranch: Stmt | None

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitIfStmt(self)


@dataclass(frozen=True)
class LoopInterupt(Stmt):
    keyword: Token
    value: int = 1

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitLoopInteruptStmt(self)


@dataclass(frozen=True)
class Print(Stmt):
    value: Expr

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitPrintStmt(self)


@dataclass(frozen=True)
class Return(Stmt):
    keyword: Token
    value: typing.Optional[Expr]

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitReturnStmt(self)


@dataclass(frozen=True)
class Var(Stmt):
    name: Token
    value: typing.Optional[Expr]

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitVarStmt(self)


@dataclass(frozen=True)
class While(Stmt):
    condition: Expr
    body: Stmt
    for_transformed: bool = False

    def accept(self, visitor: StmtVisitor) -> typing.Any:
        return visitor.visitWhileStmt(self)


