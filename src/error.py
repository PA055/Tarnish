from .token import Token, TokenType
global hadError, hadRuntimeError

hadError = False
hadRuntimeError = False


class TarnishParseError(RuntimeError):
    pass


class TarnishRuntimeError(RuntimeError):
    def __init__(self, token: Token, message: str):
        super().__init__(message)
        self.token = token


def report(line: int, where: str, message: str) -> None:
    global hadError
    hadError = True
    print(f"[line {line}] - Error{where}: {message}")


def error(line: int | Token, message: str) -> None:
    if isinstance(line, Token):
        if line.tokenType == TokenType.EOF:
            report(f"{line.line} at end", "", message)
        else:
            report(f"{line.line} at '{line.lexme}'", "", message)
    else:
        report(line, "", message)


def runtimeError(e: TarnishRuntimeError) -> None:
    global hadRuntimeError
    print(f"[line {e.token.line}] - {e}")
    hadRuntimeError = True
