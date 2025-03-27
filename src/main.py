from src import error
from src.scanner import Scanner
from src.parser import Parser
from src.interpreter import Interpreter


interpreter = Interpreter()

def run(source: str):
    scanner = Scanner(source)
    tokens = scanner.scanTokens()
    parser = Parser(tokens)
    statements = parser.parse()

    if error.hadError:
        return

    interpreter.interpret(statements)


def runPrompt():
    while True:
        try:
            code = input(">>> ")
            run(code)
            error.hadError = False
            error.hadRuntimeError = False
        except EOFError:
            break


def runFile(path: str):
    with open(path) as f:
        run(f.read())

    if error.hadError or error.hadRuntimeError:
        exit(1)
