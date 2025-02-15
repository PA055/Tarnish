from .token import Token, TokenType
global hadError

hadError = False


def report(line: int, where: str, message: str):
    global hadError
    hadError = True
    print(f"[line {line}] - Error{where}: {message}")


def error(line: int | Token, message: str):
    if isinstance(line, Token):
        if line.tokenType == TokenType.EOF:
            report(f"{line.line} at end", "", message)
        else:
            report(f"{line.line} at '{line.lexme}'", "", message)
    else:
        report(line, "", message)


class ParseError(RuntimeError):
    pass
