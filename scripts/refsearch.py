#!/usr/bin/env python3
"""Compatibility shim. Prefer the standalone repoindex workspace CLI."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/repoindex"))
from workspace_cli import main as cli_main


def main():
    args = sys.argv[1:]
    if not args:
        return cli_main(args)
    defaults = ["--workspace", str(ROOT), "--limit", "40"]
    if args[0] not in {"text", "scopes"}:
        defaults.append("--json")
    return cli_main([args[0]] + defaults + args[1:])


if __name__ == "__main__":
    sys.exit(main())
