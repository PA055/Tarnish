from .interupts import ReturnInterupt
from .environment import Environment
from .callable import TarnishCallable
from .grammar import Func, Lambda
from typing import Any, TYPE_CHECKING
if TYPE_CHECKING:
    from .interpreter import Interpreter


class TarnishFunction(TarnishCallable):
    def __init__(self, declaration: Func | Lambda, closure: Environment) -> None:
        self.declaration = declaration
        self.closure = closure

    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> Any:
        enviorment = Environment(self.closure)
        for i, param in enumerate(self.declaration.params):
            enviorment.define(param, arguments[i])

        try:
            if isinstance(self.declaration, Lambda):
                interpreter.executeBlock([self.declaration.body], enviorment)
            else:
                interpreter.executeBlock(self.declaration.body, enviorment)
        except ReturnInterupt as r:
            return r.value

        return None

    def arity(self) -> int:
        return len(self.declaration.params)

    def __str__(self):
        return f"function '{self.declaration.name.lexme}'"
