from src import error
from src.scanner import Scanner
from src.parser import Parser
from src.ast_printer import AstPrinter
from src.interpreter import Interpreter


interpreter = Interpreter()

def run(source: str):
    scanner = Scanner(source)
    tokens = scanner.scanTokens()
    parser = Parser(tokens)
    expression = parser.parse()

    if error.hadError:
        return

    print(AstPrinter().print(expression))
    print(interpreter.interpret(expression))


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
