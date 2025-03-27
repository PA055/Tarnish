from abc import abstractmethod, ABC
from dataclasses import dataclass
from typing import Any, TYPE_CHECKING
if TYPE_CHECKING:
    from .interpreter import Interpreter


@dataclass
class TarnishCallable(ABC):
    @abstractmethod
    def call(self, interpreter: "Interpreter", arguments: list[Any]) -> Any:
        raise NotImplementedError()

    @abstractmethod
    def arity(self) -> int:
        raise NotImplementedError()
