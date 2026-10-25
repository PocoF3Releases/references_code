"""Internal stdlib-only worker for check_fresh_clone.py; never scans source trees."""
from contextlib import ExitStack, redirect_stdout, redirect_stderr
import importlib.metadata
import importlib.util
import io
import json
from pathlib import Path
import socket
import sqlite3
import subprocess
import sys
import time
from unittest.mock import patch

# Consumer-owned smoke samples, deliberately outside the generic repoindex package.
SAMPLES = {
    "android-t-qcom": ("*AudioFlinger*", "AudioFlinger", "AudioFlinger::checkAttributionSourcePackage", "frameworks"),
    "LA.UM.8.12.r1-11900-sm8250.0": ("*camxbpsnode*", "BPSNode", "BPSNode::ExecuteProcessRequest", "camera"),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def probe(root):
    root = Path(root).resolve()
    require(sys.flags.isolated and sys.flags.no_site, "Probe requires Python -I -S")
    require(not list(importlib.metadata.distributions()), "Site packages leaked into isolation")
    require(importlib.util.find_spec("repoindex") is None, "Installed repoindex leaked into isolation")
    require(importlib.util.find_spec("pip") is None, "pip leaked into isolation")
    require(not (root / ".cache").exists(), "Fresh clone unexpectedly contains a cache")
    sys.path.insert(0, str(root / "tools/repoindex/src"))
    from repoindex import workspace, scanner, implementations, workspace_cli, VERSION
    require(Path(workspace.__file__).resolve().is_file() and
            root in Path(workspace.__file__).resolve().parents, "Wrong repoindex imported")
    names = workspace.config(root)["trees"]
    require(set(names) == set(SAMPLES), "Update smoke samples for configured trees")
    require(all(not (root / name).exists() for name in names), "Clone contains source trees")
    checks = []

    def query(label, arguments, expected=0, machine=True):
        output, errors = io.StringIO(), io.StringIO()
        started = time.monotonic()
        with redirect_stdout(output), redirect_stderr(errors):
            code = workspace_cli.main(arguments + ["--workspace", str(root)] + (["--json"] if machine else []))
        require(code == expected, label + ": " + errors.getvalue()[-1500:])
        text = output.getvalue()
        require(len(text.encode()) <= 12000, label + ": output budget exceeded")
        rows = [json.loads(line) for line in text.splitlines()] if machine else []
        checks.append({"check": label, "exit": code, "expected_exit": expected,
                       "seconds": round(time.monotonic() - started, 3), "stdout_bytes": len(text.encode())})
        return rows, errors.getvalue()

    with ExitStack() as guards:
        for module, name in [(socket, "socket"), (socket, "getaddrinfo"), (subprocess, "Popen"),
                             (workspace, "build"), (workspace, "optimize"), (workspace, "verify"),
                             (scanner, "build_index"), (implementations, "build")]:
            guards.enter_context(patch.object(module, name, side_effect=AssertionError("Forbidden network/process/ingestion: " + name)))
        rows, _ = query("cold saved status", ["status"])
        require(len(rows) == 2 and all(not row["cache_ready"] for row in rows), "Status restored or skipped an index")
        query("configured scopes", ["scopes"])
        require(not (root / ".cache").exists(), "Navigation created a cache")
        rows, _ = query("first ordinary query restores both", ["files", "*AudioFlinger*"])
        require(rows and all(row["tree"] == "android-t-qcom" for row in rows), "Android file lookup failed")
        rows, _ = query("warm saved status", ["status"])
        require({row["tree"] for row in rows} == set(names) and all(row["cache_ready"] and
                row["scan_errors"] == 0 and row["implementation_files"] > 0 for row in rows), "Both indexes are not ready")
        database_stats = {p.relative_to(root).as_posix(): [p.stat().st_size, p.stat().st_mtime_ns]
                          for p in (root / ".cache").rglob("*.db")}
        require(len(database_stats) == 2, "Expected exactly two restored databases")
        headers = []
        for tree, (files, symbol, literal, scope) in SAMPLES.items():
            for label, arguments in [("files", ["files", files]), ("symbol", ["symbol", symbol]),
                                     ("body", ["impl", literal])]:
                rows, _ = query(tree + " " + label, arguments + ["--tree", tree, "--limit", "5"])
                require(rows and all(row["tree"] == tree for row in rows), "Wrong/missing selected-tree results")
                if label == "symbol":
                    headers.append(rows[0]["path"])
            rows, _ = query(tree + " default body", ["impl", literal, "--limit", "5"])
            require(rows and all(row["tree"] == tree for row in rows), "Default selection missed " + tree)
            hit = rows[0]
            rows, _ = query(tree + " scoped body", ["impl", literal, "--scope", scope, "--limit", "5"])
            require(rows and all(row["tree"] == tree for row in rows), "Scoped body lookup failed")
            rows, _ = query(tree + " source range", ["source", hit["path"], "--line", str(hit["line"]), "--limit", "4"])
            require(len(rows) == 1 and literal in rows[0]["text"], "Indexed range did not contain body hit")
        trees = {row["tree"] for row in workspace.query(root, None, "files", "*", prefixes=headers, limit=10)}
        require(trees == set(names), "Default Python API skipped a tree")
        checks.append({"check": "both-tree Python API with guards", "exit": 0})
        rows, _ = query("empty tree/scope intersection", ["impl", "BPSNode", "--tree", "android-t-qcom", "--scope", "camera"])
        require(not rows, "Empty intersection broadened selection")
        rows, _ = query("range past EOF", ["source", hit["path"], "--line", "100000000"])
        require(not rows, "Range past EOF produced rows")
        _, errors = query("missing live source is explicit", ["text", "token", "--scope", "frameworks"], expected=2)
        require("Live source unavailable" in errors, "Missing-source error is unclear")
        query("unknown tree fails", ["files", "token", "--tree", "not-a-tree"], expected=2)
        for machine in (False, True):
            _, errors = query("bounded " + ("JSON" if machine else "text"), ["files", "*.cpp", "--limit", "1000"], machine=machine)
            require("Output budget reached" in errors, "Output-budget boundary not exercised")
        rows, _ = query("global result limit", ["files", "*.cpp", "--limit", "2"])
        require(len(rows) == 2, "Global result limit failed")
    return {"completed": True, "trees": names, "tool_version": VERSION,
            "python": sys.version.split()[0], "sqlite": sqlite3.sqlite_version,
            "site_packages_enabled": False, "third_party_distributions": 0,
            "pip_installed": False, "repoindex_installed": False, "source_directories_present": False,
            "probe_network_process_ingestion_guards": True, "database_stats": database_stats, "checks": checks}


if __name__ == "__main__":
    try:
        print(json.dumps(probe(Path(sys.argv[1])), indent=2))
    except (OSError, ValueError, AssertionError, StopIteration) as error:
        print("Query probe failed: " + str(error), file=sys.stderr)
        raise SystemExit(1)
