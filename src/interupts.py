from typing import Any


class TarnishInterupt(RuntimeError):
    pass


class ReturnInterupt(TarnishInterupt):
    def __init__(self, value: Any):
        self.value = value


class BreakInterupt(TarnishInterupt):
    def __init__(self, number: int):
        self.number = number


class ContinueInterupt(TarnishInterupt):
    pass
