from .token import Token
from .error import TarnishRuntimeError
from typing import Dict, Any, Optional, TYPE_CHECKING


class Environment:
    def __init__(self, enclosing: Optional["Environment"] = None):
        self.values: Dict[str, Any] = {}
        self.enclosing: Optional["Environment"] = enclosing

    def define(self, name: Token | str, value: Any) -> None:
        if isinstance(name, Token):
            name = name.lexme

        self.values[name] = value

    def assign(self, name: Token, value: Any) -> None:
        if name.lexme in self.values:
            self.values[name.lexme] = value
            return

        if self.enclosing is not None:
            self.enclosing.assign(name, value)
            return

        raise TarnishRuntimeError(name, f"Undefined variable '{name.lexme}'.")

    def get(self, name: Token):
        if name.lexme in self.values:
            return self.values[name.lexme]

        if self.enclosing is not None:
            return self.enclosing.get(name)

        raise TarnishRuntimeError(name, f"Undefined variable '{name.lexme}'.")

    def assign_at(self, distance: int, name: Token, value: Any) -> None:
        self.ancestor(distance).values[name.lexme] = value

    def get_at(self, distance: int, name: str | Token) -> Any:
        if isinstance(name, str):
            return self.ancestor(distance).values[name]
        return self.ancestor(distance).values[name.lexme]

    def ancestor(self, distance: int) -> "Environment":
        environment: "Environment" = self
        for _ in range(distance):
            assert environment.enclosing is not None
            environment = environment.enclosing
        return environment
