from src.error import error
from src.token import TokenType, Token


class Scanner:
    def __init__(self, source: str):
        self.source: str = source
        self.tokens: list[Token] = []

        self.start: int = 0
        self.current: int = 0
        self.line: int = 1

    def isDigit(self, char: str) -> bool:
        return all("0" <= c <= "9" for c in char)

    def isAlpha(self, char: str) -> bool:
        return all("a" <= c <= "z" or "A" <= c <= "Z" or c == '_' for c in char)

    def isAlphaNumeric(self, char: str) -> bool:
        return self.isDigit(char) or self.isAlpha(char)

    def isAtEnd(self) -> bool:
        return self.current >= len(self.source)

    def advance(self, n: int = 1) -> str:
        self.current += n
        return self.source[self.current - n: self.current]

    def peek(self, n: int = 1) -> str:
        if (self.current + n > len(self.source)):
            return '\0'
        return self.source[self.current: self.current + n]

    def match(self, expected: str) -> bool:
        if self.isAtEnd():
            return False
        if not self.source[self.current:].startswith(expected):
            return False
        self.current += len(expected)
        return True

    def addToken(self, tokenType: TokenType, value: any = None) -> None:
        self.tokens.append(Token(tokenType, self.source[self.start:self.current], value, self.line))

    def blockComment(self):
        nesting = 1
        while nesting > 0:
            if self.match("/*"):
                nesting += 1

            if self.match("*/"):
                nesting -= 1
                if nesting == 0:
                    break

            self.advance()

    def identifier(self) -> None:
        while self.isAlphaNumeric(self.peek()):
            self.advance()

        keywords = {
            "class": TokenType.CLASS,
            "else": TokenType.ELSE,
            "enum": TokenType.ENUM,
            "false": TokenType.FALSE,
            "for": TokenType.FOR,
            "func": TokenType.FUNC,
            "if": TokenType.IF,
            "interface": TokenType.INTERFACE,
            "none": TokenType.NONE,
            "print": TokenType.PRINT,
            "private": TokenType.PRIVATE,
            "protected": TokenType.PROTECTED,
            "public": TokenType.PUBLIC,
            "return": TokenType.RETURN,
            "super": TokenType.SUPER,
            "this": TokenType.THIS,
            "true": TokenType.TRUE,
            "var": TokenType.VAR,
            "while": TokenType.WHILE
        }

        lexme = self.source[self.start:self.current]
        self.addToken(keywords.get(lexme, TokenType.IDENTIFIER))

    def number(self) -> None:
        while self.isDigit(self.peek()):
            self.advance()

        if self.peek() == '.' and (self.isDigit(self.peek(2)[1])):
            self.advance()
            while self.isDigit(self.peek()):
                self.advance()

        self.addToken(TokenType.NUMBER, float(self.source[self.start:self.current]))

    def string(self) -> None:
        if self.match("\"\""):
            while self.peek(3) != "\"\"\"" and not self.isAtEnd():
                if self.peek() == "\n":
                    self.line += 1
                self.advance()

            if self.isAtEnd():
                error(self.line, "Unterminated string.")
                return

            self.advance(3)
            self.addToken(TokenType.STRING, self.source[self.start + 3:self.current - 3])
            return

        while self.peek() != "\"" and not (self.isAtEnd() or self.peek() == '\n'):
            self.advance()

        if self.isAtEnd() or self.peek() == '\n':
            error(self.line, "Unterminated string.")
            return

        self.advance()
        self.addToken(TokenType.STRING, self.source[self.start + 1:self.current - 1])


    def scanToken(self) -> None:
        c: str = self.advance()
        if c == "(":   self.addToken(TokenType.LEFT_PAREN)
        elif c == ")": self.addToken(TokenType.RIGHT_PAREN)
        elif c == "{": self.addToken(TokenType.LEFT_BRACE)
        elif c == "}": self.addToken(TokenType.RIGHT_BRACE)
        elif c == "[": self.addToken(TokenType.LEFT_BRACKET)
        elif c == "]": self.addToken(TokenType.RIGHT_BRACKET)
        elif c == ',': self.addToken(TokenType.COMMA)
        elif c == '@': self.addToken(TokenType.AT_SIGN)
        elif c == ':': self.addToken(TokenType.COLON)
        elif c == ';': self.addToken(TokenType.SEMICOLON)
        elif c == '?': self.addToken(TokenType.QUESTION)

        elif c == "=":
            self.addToken(TokenType.EQUAL_EQUAL if self.match("=") else TokenType.EQUAL)
        elif c == "!":
            self.addToken(TokenType.BANG_EQUAL if self.match("=") else TokenType.BANG)
        elif c == "~":
            self.addToken(TokenType.TILDE_TILDE if self.match("~") else TokenType.TILDE)
        elif c == "^":
            self.addToken(TokenType.CARET_EQUAL if self.match("=") else TokenType.CARET)
        elif c == "%":
            self.addToken(TokenType.PERCENT_EQUAL if self.match("=") else TokenType.PERCENT)

        elif c == "/":
            if self.match("/"):
                while self.peek() != '\n' and not self.isAtEnd():
                    self.advance()
            elif self.match("*"):
                self.blockComment()
            elif self.match("="):
                self.addToken(TokenType.SLASH_EQUAL)
            else:
                self.addToken(TokenType.SLASH)

        elif c == "*":
            if self.match("*"):
                self.addToken(TokenType.STAR_STAR)
            elif self.match("="):
                self.addToken(TokenType.STAR_EQUAL)
            else:
                self.addToken(TokenType.STAR)

        elif c == "+":
            if self.match("+"):
                self.addToken(TokenType.PLUS_PLUS)
            elif self.match("="):
                self.addToken(TokenType.PLUS_EQUAL)
            else:
                self.addToken(TokenType.PLUS)

        elif c == "|":
            if self.match("|"):
                self.addToken(TokenType.BAR_BAR)
            elif self.match("="):
                self.addToken(TokenType.BAR_EQUAL)
            else:
                self.addToken(TokenType.BAR)

        elif c == "&":
            if self.match("&"):
                self.addToken(TokenType.AMPERSAND_AMPERSAND)
            elif self.match("="):
                self.addToken(TokenType.AMPERSAND_EQUAL)
            else:
                self.addToken(TokenType.AMPERSAND)

        elif c == "-":
            if self.match("-"):
                self.addToken(TokenType.MINUS_MINUS)
            elif self.match(">"):
                self.addToken(TokenType.ARROW)
            elif self.match("="):
                self.addToken(TokenType.MINUS_EQUAL)
            else:
                self.addToken(TokenType.MINUS)

        elif c == "<":
            if self.match("<="):
                self.addToken(TokenType.LESS_LESS_EQUAL)
            elif self.match("<"):
                self.addToken(TokenType.LESS_LESS)
            elif self.match("="):
                self.addToken(TokenType.LESS_EQUAL)
            else:
                self.addToken(TokenType.LESS)

        elif c == ">":
            if self.match(">="):
                self.addToken(TokenType.GREATER_GREATER_EQUAL)
            elif self.match(">"):
                self.addToken(TokenType.GREATER_GREATER)
            elif self.match("="):
                self.addToken(TokenType.GREATER_EQUAL)
            else:
                self.addToken(TokenType.GREATER)

        elif self.isAlpha(c):
            self.identifier()

        elif c == "\"":
            self.string()

        elif c == '.':
            if self.isDigit(self.peek()):
                self.number()
            else:
                self.addToken(TokenType.DOT)

        elif self.isDigit(c):
            self.number()

        elif c == '\n':
            self.line += 1

        elif c.isspace():
            pass

        else:
            error(self.line, f"Unexpected character: {c}.")

    def scanTokens(self) -> list[Token]:
        while not self.isAtEnd():
            self.start = self.current
            self.scanToken()

        self.tokens.append(Token(TokenType.EOF, "", None, self.line))
        return self.tokens
