import sys
import os


def defineType(f, baseName, className, fields):
    f.write(f"@dataclass(frozen=True)\nclass {className}({baseName}):\n")
    for field in fields:
        f.write(f"    {field}\n")
    f.write(f"""
    def accept(self, visitor: "Visitor") -> any:
        return visitor.visit{className}{baseName}(self)


""")


def defineVisitor(path: str, baseName: str, types: dict):
    f = open(path, 'w')
    f.write(f"""from abc import abstractmethod
from typing import TYPE_CHECKING, Protocol

if TYPE_CHECKING:
    from .expr import {", ".join(types.keys())}


class Visitor(Protocol):
""")
    for className in types.keys():
        f.write(f"""    @abstractmethod
    def visit{className}{baseName}(self, expr: "{className}") -> any:
        raise NotImplementedError

""")


def defineAst(outputDir: str, baseName: str, types: dict):
    path = os.path.join(outputDir, baseName.lower() + '.py')
    f = open(path, 'w')
    f.write(f"""from abc import ABC, abstractmethod
from dataclasses import dataclass
import typing
from .{baseName.lower()}_visitor import Visitor

if typing.TYPE_CHECKING:
    from .token import Token


class {baseName}(ABC):
    @abstractmethod
    def accept(self, visitor: "Visitor") -> any:
        raise NotImplementedError


""")

    for className, fields in types.items():
        defineType(f, baseName, className, fields)

    path = os.path.join(outputDir, f'{baseName.lower()}_visitor.py')
    defineVisitor(path, baseName, types)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python astGen.py <output_directory>")
        exit(64)

    outputDir = sys.argv[1]
    defineAst(outputDir, "Expr", {
        "Unary": ["operator: \"Token\"", "expression: Expr"],
        "Binary": ["left: Expr", "operator: \"Token\"", "right: Expr"],
        "Ternary": ["one: Expr", "op1: \"Token\"", "two: Expr", "op2: \"Token\"", "three: Expr"],
        "Postfix": ["operator: \"Token\"", "expression: Expr"],
        "Grouping": ["expression: Expr"],
        "Literal": ["value: any"],
    })
