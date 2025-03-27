import time
from dataclasses import dataclass
from src.callable import TarnishCallable
from typing import Any, TYPE_CHECKING
if TYPE_CHECKING:
    from src.interpreter import Interpreter


@dataclass
class Time(TarnishCallable):
    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> float:
        return time.time()

    def arity(self) -> int:
        return 0
