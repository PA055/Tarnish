from typing import Dict, Any
from .token import Token
from .error import TarnishRuntimeError


class Environment:
    def __init__(self, enclosing: "Environment" | None = None):
        self.values: Dict[str, Any] = {}
        self.enclosing: "Environment" | None = enclosing

    def define(self, name: str, value: Any) -> None:
        self.values[name] = value

    def get(self, name: Token):
        if name.lexme in self.values:
            return self.values[name.lexme]

        if self.enclosing is not None:
            return self.enclosing.get(name)

        raise TarnishRuntimeError(name, f"Undefined variable '{name.lexme}'.")

    def assign(self, name: Token, value: Any) -> None:
        if name.lexme in self.values:
            self.values[name.lexme] = value
            return

        if self.enclosing is not None:
            self.enclosing.assign(name, value)
            return

        raise TarnishRuntimeError(name, f"Undefined variable '{name.lexme}'.")
