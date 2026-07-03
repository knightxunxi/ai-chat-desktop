from __future__ import annotations

import sys

from .protocol import handle_line


def main() -> int:
    for raw_line in sys.stdin:
        line = raw_line.strip()
        if not line:
            continue
        print(handle_line(line), flush=True)
    return 0

