from enum import StrEnum
from typing import Any


class TokenType(StrEnum):
    # Single character tokens
    LEFT_PAREN = "LEFT_PAREN"
    RIGHT_PAREN = "RIGHT_PAREN"
    LEFT_BRACE = "LEFT_BRACE"
    RIGHT_BRACE = "RIGHT_BRACE"
    LEFT_BRACKET = "LEFT_BRACKET"
    RIGHT_BRACKET = "RIGHT_BRACKET"
    COMMA = "COMMA"
    DOT = "DOT"
    AT_SIGN = "AT_SIGN"
    COLON = "COLON"
    SEMICOLON = "SEMICOLON"
    QUESTION = "QUESTION"

    # Multi character tokens
    EQUAL = "EQUAL"
    EQUAL_EQUAL = "EQUAL_EQUAL"
    BANG = "BANG"
    BANG_EQUAL = "BANG_EQUAL"
    TILDE = "TILDE"
    TILDE_TILDE = "TILDE_TILDE"
    CARET = "CARET"
    CARET_EQUAL = "CARET_EQUAL"
    PERCENT = "PERCENT"
    PERCENT_EQUAL = "PERCENT_EQUAL"
    SLASH = "SLASH"
    SLASH_EQUAL = "SLASH_EQUAL"

    STAR = "STAR"
    STAR_STAR = "STAR_STAR"
    STAR_EQUAL = "STAR_EQUAL"

    PLUS = "PLUS"
    PLUS_PLUS = "PLUS_PLUS"
    PLUS_EQUAL = "PLUS_EQUAL"

    BAR = "BAR"
    BAR_BAR = "BAR_BAR"
    BAR_EQUAL = "BAR_EQUAL"

    AMPERSAND = "AMPERSAND"
    AMPERSAND_AMPERSAND = "AMPERSAND_AMPERSAND"
    AMPERSAND_EQUAL = "AMPERSAND_EQUAL"

    MINUS = "MINUS"
    ARROW = "ARROW"
    MINUS_MINUS = "MINUS_MINUS"
    MINUS_EQUAL = "MINUS_EQUAL"

    LESS = "LESS"
    LESS_EQUAL = "LESS_EQUAL"
    LESS_LESS = "LESS_LESS"
    LESS_LESS_EQUAL = "LESS_LESS_EQUAL"

    GREATER = "GREATER"
    GREATER_EQUAL = "GREATER_EQUAL"
    GREATER_GREATER = "GREATER_GREATER"
    GREATER_GREATER_EQUAL = "GREATER_GREATER_EQUAL"

    # Literals
    IDENTIFIER = "IDENTIFIER"
    STRING = "STRING"
    NUMBER = "NUMBER"

    # Keywords
    CLASS = "CLASS"
    ELSE = "ELSE"
    ENUM = "ENUM"
    FALSE = "FALSE"
    FOR = "FOR"
    FUNC = "FUNC"
    GLOBAL = "GLOBAL"
    IF = "IF"
    INTERFACE = "INTERFACE"
    NONE = "NONE"
    PRINT = "PRINT"
    PRIVATE = "PRIVATE"
    PROTECTED = "PROTECTED"
    PUBLIC = "PUBLIC"
    RETURN = "RETURN"
    SUPER = "SUPER"
    THIS = "THIS"
    TRUE = "TRUE"
    WHILE = "WHILE"

    EOF = "EOF"


class Token:
    def __init__(self, tokenType: TokenType, lexme: str, literal: Any, line: int):
        self.tokenType = tokenType
        self.lexme = lexme
        self.literal = literal
        self.line = line

    def __str__(self) -> str:
        return f"{self.tokenType} [line {self.line}] - {self.lexme}" \
                + ("" if self.literal is None else f"({self.literal})")
