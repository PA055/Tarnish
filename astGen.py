import sys
import os
from typing import Tuple


def defineType(f, baseName, className, fields):
    f.write(f"@dataclass(frozen=True)\nclass {className}({baseName}):\n")
    for field in fields:
        f.write(f"    {field}\n")
    f.write(f"""
    def accept(self, visitor: {baseName}Visitor) -> typing.Any:
        return visitor.visit{className}{baseName}(self)


""")


def defineAst(outputDir: str, *asts: Tuple[str, dict]):
    path = os.path.join(outputDir, 'grammar.py')
    f = open(path, 'w')
    f.write("""from abc import ABC, abstractmethod
from dataclasses import dataclass
import typing
from .token import Token


""")

    for baseName, types in asts:
        f.write(f"class {baseName}Visitor(typing.Protocol):\n")
        for className in types.keys():
            f.write(f"""    @abstractmethod
    def visit{className}{baseName}(self, {baseName.lower()}: "{className}") -> typing.Any:
        raise NotImplementedError

""")
        f.write(f"""
class {baseName}(ABC):
    @abstractmethod
    def accept(self, visitor: {baseName}Visitor) -> typing.Any:
        raise NotImplementedError


""")

        for className, fields in types.items():
            defineType(f, baseName, className, fields)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python astGen.py <output_directory>")
        exit(64)

    outputDir = sys.argv[1]
    defineAst(
        outputDir,  ("Expr", {
            "Assign":         ["name: Token", "value: Expr"],
            "Binary":         ["left: Expr", "operator: Token", "right: Expr"],
            "Call":           ["callee: Expr", "paren: Token", "arguments: list[Expr]"],
            "Grouping":       ["expression: Expr"],
            "Lambda":         ["params: list[Token]", "body: \"Stmt\""],
            "List":           ["expressions: list[Expr]"],
            "Literal":        ["value: typing.Any"],
            "Logical":        ["left: Expr", "operator: Token", "right: Expr"],
            "Postfix":        ["operator: Token", "name: Token"],
            "Prefix":         ["operator: Token", "name: Token"],
            "Ternary":        ["one: Expr", "op1: Token", "two: Expr", "op2: Token", "three: Expr"],
            "Unary":          ["operator: Token", "expression: Expr"],
            "Variable":       ["name: Token"],
        }),         ("Stmt", {
            "Block":          ["statements: list[Stmt]"],
            "LoopInterupt":   ["keyword: Token", "value: int = 1"],
            "Expression":     ["value: Expr"],
            "Func":           ["name: Token", "params: list[Token]", "body: list[typing.Optional[Stmt]]"],
            "If":             ["condition: Expr", "thenBranch: Stmt", "elseBranch: Stmt | None"],
            "Print":          ["value: Expr"],
            "Return":         ["keyword: Token", "value: typing.Optional[Expr]"],
            "Var":            ["name: Token", "value: typing.Optional[Expr]"],
            "While":          ["condition: Expr", "body: Stmt", "for_transformed: bool = False"],
        }))
