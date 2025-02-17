import sys
import os
from typing import Tuple


def defineType(f, baseName, className, fields):
    f.write(f"@dataclass(frozen=True)\nclass {className}({baseName}):\n")
    for field in fields:
        f.write(f"    {field}\n")
    f.write(f"""
    def accept(self, visitor: {baseName}Visitor) -> any:
        return visitor.visit{className}{baseName}(self)


""")


def defineAst(outputDir: str, *asts: Tuple[Tuple[str, dict]]):
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
    def visit{className}{baseName}(self, {baseName.lower()}: "{className}") -> any:
        raise NotImplementedError

""")
        f.write(f"""
class {baseName}(ABC):
    @abstractmethod
    def accept(self, visitor: {baseName}Visitor) -> any:
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
            "Unary":       ["operator: Token", "expression: Expr"],
            "Binary":      ["left: Expr", "operator: Token", "right: Expr"],
            "Ternary":     ["one: Expr", "op1: Token", "two: Expr", "op2: Token", "three: Expr"],
            "Postfix":     ["operator: Token", "expression: Expr"],
            "Grouping":    ["expression: Expr"],
            "Literal":     ["value: any"],
            "Variable":    ["name: Token"]
        }),         ("Stmt", {
            "Print":       ["value: Expr"],
            "Expression":  ["value: Expr"],
            "Var":         ["name: Token", "value: Expr"],
        }))
