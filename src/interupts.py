from typing import Any

class ReturnInterupt(RuntimeError):
    def __init__(self, value: Any):
        self.value = value
