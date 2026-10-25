#!/usr/bin/env python3
"""Compatibility shim for explicit index refresh; never called on query startup."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/repoindex"))
from workspace_cli import main as cli_main, write_guide


def guide():
    return write_guide(ROOT)


def main(argv=None):
    return cli_main(["index", "--workspace", str(ROOT)] + (sys.argv[1:] if argv is None else argv))


if __name__ == "__main__":
    sys.exit(main())
