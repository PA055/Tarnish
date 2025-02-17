from typing import Dict, Any
from .token import Token
from .error import TarnishRuntimeError


class Environment:
    def __init__(self):
        self.values: Dict[str, Any] = {}

    def define(self, name: str, value: Any) -> None:
        self.values[name] = value

    def get(self, name: Token):
        if name.lexme in self.values:
            return self.values[name.lexme]

        raise TarnishRuntimeError(name, f"Undefined variable '{name.lexme}'.")
