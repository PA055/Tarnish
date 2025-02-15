from src.scanner import Scanner
from src import error
from src.parser import Parser
from src.ast_printer import AstPrinter
from src.interpreter import Interpreter


def run(source: str):
    scanner = Scanner(source)
    tokens = scanner.scanTokens()
    parser = Parser(tokens)
    expression = parser.parse()

    if error.hadError:
        return

    print(AstPrinter().print(expression))
    print(Interpreter().evaluate(expression))


def runPrompt():
    while True:
        try:
            code = input(">>> ")
            run(code)
            error.hadError = False
        except EOFError:
            break


def runFile(path: str):
    with open(path) as f:
        run(f.read())

    if error.hadError:
        exit(1)
