from dataclasses import dataclass

@dataclass
class Range:
    start: int
    end: int
    step: int = 1

    def __iter__(self):
        return iter(range(self.start, self.end, self.step))