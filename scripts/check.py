#!/usr/bin/env python3
"""Check maintained docs/config/bundle sizes without restoring or scanning sources."""
import argparse
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/repoindex/src"))
from repoindex.checks import documentation_errors
from repoindex.configuration import config, scopes
from repoindex.bundles import manifest, MAX_PART


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tests", action="store_true", help="Also run standalone and consumer synthetic fixture checks")
    args = parser.parse_args()
    paths = [ROOT / "AGENTS.md", ROOT / "README.md", *(ROOT / "docs").glob("*.md"),
             ROOT / "evidence/README.md", ROOT / "index/README.md"]
    errors = documentation_errors(ROOT, {"AGENTS.md": 2000, "README.md": 3000,
                                        "docs/SEARCH.md": 7000}, paths)
    try:
        data = config(ROOT)
        scopes(ROOT, data)
        parts = 0
        for tree in data["trees"]:
            saved = manifest(ROOT, tree)
            for part in saved["parts"]:
                size = (ROOT / "index" / tree / part["file"]).stat().st_size
                if size != part["bytes"] or size > MAX_PART:
                    errors.append("Missing, mismatched or oversized bundle part: " + part["file"])
                parts += 1
    except (OSError, ValueError, TypeError, KeyError) as error:
        errors.append(str(error))
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print("Workspace docs/config and %d bundle parts checked; no restore or source scan" % parts)
    if args.tests:
        result = subprocess.call([sys.executable, str(ROOT / "tools/repoindex/scripts/check.py")], cwd=ROOT)
        if result:
            return result
        return subprocess.call([sys.executable, "-m", "unittest", "discover", "-s", str(ROOT / "scripts/tests")], cwd=ROOT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
