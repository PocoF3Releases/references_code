#!/usr/bin/env python3
"""Compatibility shim for the configurable standalone benchmark command."""
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/repoindex"))
from workspace_cli import main

if __name__ == "__main__":
    sys.exit(main(["benchmark", "--workspace", str(ROOT), "--json"] + sys.argv[1:]))
