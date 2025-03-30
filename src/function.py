from .interupts import ReturnInterupt
from .environment import Environment
from .callable import TarnishCallable
from .grammar import Func, Lambda
from typing import Any, TYPE_CHECKING
if TYPE_CHECKING:
    from .interpreter import Interpreter
    from .classes import TarnishInstance


class TarnishFunction(TarnishCallable):
    def __init__(self, declaration: Func | Lambda, closure: Environment, isInitializer: bool = False) -> None:
        self.declaration = declaration
        self.closure = closure
        self.isInitializer = isInitializer

    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> Any:
        enviorment = Environment(self.closure)
        for i, param in enumerate(self.declaration.params):
            enviorment.define(param, arguments[i])

        try:
            if isinstance(self.declaration, Lambda):
                interpreter.executeBlock([self.declaration.body], enviorment)
            else:
                interpreter.executeBlock([i for i in self.declaration.body if i is not None], enviorment)
        except ReturnInterupt as r:
            if self.isInitializer:
                return self.closure.getAt(0, 'this')
            return r.value

        if self.isInitializer:
            return self.closure.getAt(0, 'this')
        return None

    def bind(self, instance: "TarnishInstance"):
        environment = Environment(self.closure)
        environment.define("this", instance)
        return TarnishFunction(self.declaration, environment, self.isInitializer)

    def arity(self) -> int:
        return len(self.declaration.params)

    def __str__(self):
        return f"function '{self.declaration.name.lexme}'"
