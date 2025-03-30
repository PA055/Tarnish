from .error import TarnishRuntimeError
from .callable import TarnishCallable
from .function import TarnishFunction
from .grammar import Class, Func
from .token import Token
from typing import TYPE_CHECKING, Any, Dict
if TYPE_CHECKING:
    from .interpreter import Interpreter


class TarnishInstance():
    def __init__(self, instanceClass: "TarnishClass"):
        self.instanceClass = instanceClass
        self.fields: Dict[str, Any] = {}

    def __str__(self):
        return f"{self.instanceClass.name} instance"

    def get(self, name: Token) -> Any:
        if name.lexme in self.fields:
            return self.fields[name.lexme]

        method = self.instanceClass.findMethod(name.lexme)
        if method is not None:
            return method.bind(self)


        raise TarnishRuntimeError(name, f"Undefined property '{name.lexme}'.")

    def set(self, name: Token, value: Any):
        self.fields[name.lexme] = value


class TarnishClass(TarnishCallable, TarnishInstance):
    def __init__(self, name: str, methods: Dict[str, TarnishFunction], superclass: "TarnishClass"):
        self.name = name
        self.methods = methods
        self.superclass = superclass
        self.instanceClass = self

    def __str__(self) -> str:
        return f"class <{self.name}>"

    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> TarnishInstance:
        instance = TarnishInstance(self)
        initializer = self.findMethod('__init__')
        if initializer is not None:
            initializer.bind(instance).call(interpreter, arguments)
        return instance

    def arity(self) -> int:
        initializer = self.findMethod('__init__')
        if initializer is None:
            return 0
        return initializer.arity()

    def findMethod(self, name: str):
        if name in self.methods:
            return self.methods[name]

        if self.superclass is not None:
            return self.superclass.findMethod(name)

        return None
