from .error import error
from .grammar import (
    Assign,
    Binary,
    Block,
    Call,
    Class,
    Expr,
    Expression,
    ExprVisitor,
    Func,
    Get,
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
    Set,
    Stmt,
    StmtVisitor,
    Super,
    Ternary,
    This,
    Unary,
    Var,
    Variable,
    While,
)
from enum import Enum, auto
from typing import Any, Dict, Union, Sequence, TYPE_CHECKING
if TYPE_CHECKING:
    from .token import Token
    from .interpreter import Interpreter


class FunctionType(Enum):
    NONE = auto()
    FUNCTION = auto()
    INITIALIZER = auto()
    METHOD = auto()
    LAMBDA = auto()


class ClassType(Enum):
    NONE = auto()
    CLASS = auto()
    SUBCLASS = auto()


class Resolver(ExprVisitor, StmtVisitor):
    def __init__(self, interpreter: "Interpreter"):
        self.interpreter = interpreter
        self.scopes: list[Dict[str, bool]] = []
        self.loopDepth = 0
        self.currentFunction = FunctionType.NONE
        self.currentClass = ClassType.NONE

    def visitVariableExpr(self, expr: Variable):
        if len(self.scopes) > 0 and self.scopes[-1].get(expr.name.lexme, None) is False:
            error(expr.name, "Can't read local variable in its own initializer.")
        self.resolveLocal(expr, expr.name)

    def visitAssignExpr(self, expr: Assign):
        self.resolve(expr.value)
        self.resolveLocal(expr, expr.name)

    def visitBinaryExpr(self, expr: Binary):
        self.resolve(expr.left)
        self.resolve(expr.right)

    def visitCallExpr(self, expr: Call):
        self.resolve(expr.callee)
        for arg in expr.arguments:
            self.resolve(arg)

    def visitGetExpr(self, expr: Get):
        self.resolve(expr.object)

    def visitGroupingExpr(self, expr: Grouping):
        self.resolve(expr.expression)

    def visitLiteralExpr(self, expr: Literal):
        pass

    def visitListExpr(self, expr: List):
        pass

    def visitLogicalExpr(self, expr: Logical):
        self.resolve(expr.left)
        self.resolve(expr.right)

    def visitSetExpr(self, expr: Set):
        self.resolve(expr.value)
        self.resolve(expr.object)

    def visitThisExpr(self, expr: This):
        if self.currentClass == ClassType.NONE:
            error(expr.keyword, "Can't use 'this' outside of a class.")
            return None
        self.resolveLocal(expr, expr.keyword)

    def visitPostfixExpr(self, expr: Postfix):
        self.resolveLocal(expr, expr.name)

    def visitPrefixExpr(self, expr: Prefix):
        self.resolveLocal(expr, expr.name)

    def visitSuperExpr(self, expr: Super):
        if self.currentClass == ClassType.NONE:
            error(expr.keyword, "Can't use 'super' outside of a class.")
        elif self.currentClass != ClassType.SUBCLASS:
            error(expr.keyword, "Can't use 'super' in a class with no superclass.")

        self.resolveLocal(expr, expr.keyword)

    def visitTernaryExpr(self, expr: Ternary):
        self.resolve(expr.one)
        self.resolve(expr.two)
        self.resolve(expr.three)

    def visitUnaryExpr(self, expr: Unary):
        self.resolve(expr.expression)

    def visitBlockStmt(self, stmt: Block):
        self.beginScope()
        self.resolve(stmt.statements)
        self.endScope()

    def visitVarStmt(self, stmt: Var):
        self.declare(stmt.name)
        if stmt.value is not None:
            self.resolve(stmt.value)
        self.define(stmt.name)

    def visitFuncStmt(self, stmt: Func):
        self.declare(stmt.name)
        self.define(stmt.name)
        self.resolveFunction(stmt, FunctionType.FUNCTION)

    def visitClassStmt(self, stmt: Class):
        enclosingClass = self.currentClass
        self.currentClass = ClassType.CLASS

        self.declare(stmt.name)
        self.define(stmt.name)

        if stmt.superclass is not None:
            if stmt.name.lexme == stmt.superclass.name.lexme:
                error(stmt.superclass.name, "A class can't inherit from itself.")

            self.currentClass = ClassType.SUBCLASS
            self.resolve(stmt.superclass)

            self.beginScope()
            self.scopes[-1]['super'] = True

        self.beginScope()
        self.scopes[-1]['this'] = True

        for method in stmt.methods:
            declaration = FunctionType.METHOD
            if method.name.lexme == '__init__':
                declaration = FunctionType.INITIALIZER
            self.resolveFunction(method, declaration)

        self.endScope()
        if stmt.superclass is not None:
            self.endScope()
        self.currentClass = enclosingClass

    def visitExpressionStmt(self, stmt: Expression):
        self.resolve(stmt.value)

    def visitIfStmt(self, stmt: If):
        self.resolve(stmt.condition)
        self.resolve(stmt.thenBranch)
        if stmt.elseBranch is not None:
            self.resolve(stmt.elseBranch)

    def visitLambdaExpr(self, expr: Lambda):
        self.resolveFunction(expr, FunctionType.LAMBDA)

    def visitPrintStmt(self, stmt: Print):
        self.resolve(stmt.value)

    def visitReturnStmt(self, stmt: Return):
        if self.currentFunction == FunctionType.NONE:
            error(stmt.keyword, "Can't return from top-level code.")
        if stmt.value is not None:
            if self.currentFunction == FunctionType.INITIALIZER:
                error(stmt.keyword, "Can't return values from an initializer.")
            self.resolve(stmt.value)

    def visitLoopInteruptStmt(self, stmt: LoopInterupt):
        if self.loopDepth == 0:
            error(stmt.keyword, "Can't exit from a loop in top-level code.")

    def visitWhileStmt(self, stmt: While):
        self.resolve(stmt.condition)
        self.loopDepth += 1
        self.resolve(stmt.body)
        self.loopDepth -= 1

    def declare(self, name: "Token"):
        if len(self.scopes) == 0:
            return
        scope = self.scopes[-1]
        if scope.get(name.lexme, None) is not None:
            error(name, "Already a variable with this name in this scope.")
        scope[name.lexme] = False

    def define(self, name: "Token"):
        if len(self.scopes) == 0:
            return
        self.scopes[-1][name.lexme] = True

    def beginScope(self):
        self.scopes.append({})

    def endScope(self):
        self.scopes.pop()

    def resolve(self, sections: Union[Stmt, Expr, Sequence[Union[Stmt, Expr]]]):
        if not isinstance(sections, Sequence):
            sections = [sections]

        for section in sections:
            section.accept(self)

    def resolveLocal(self, expr: Expr, name: "Token"):
        for i, scope in enumerate(self.scopes[::-1]):
            if name.lexme in scope.keys():
                self.interpreter.resolve(expr, i)

    def resolveFunction(self, stmt: Func | Lambda, type: FunctionType): 
        enclosingFunction = self.currentFunction
        enclosingLoopDepth = self.loopDepth
        self.currentFunction = type
        self.beginScope()

        for param in stmt.params:
            self.declare(param)
            self.define(param)

        if isinstance(stmt, Lambda):
            self.resolve(stmt.body)
        else:
            self.resolve([i for i in stmt.body if i is not None])

        self.endScope()
        self.currentFunction = enclosingFunction
        self.loopDepth = enclosingLoopDepth
