from dataclasses import dataclass
from src.callable import TarnishCallable
from typing import Any, TYPE_CHECKING
if TYPE_CHECKING:
    from src.interpreter import Interpreter


@dataclass
class String(TarnishCallable):
    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> str:
        return str(arguments[0]).lower() if arguments[0] in (True, False, None) else str(arguments[0])

    def arity(self) -> int:
        return 1
