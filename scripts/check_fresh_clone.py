#!/usr/bin/env python3
"""Explicit fresh index-only acceptance: Git + stdlib Python/SQLite, no ingestion."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY = "https://github.com/PocoF3Releases/references_code.git"
SPARSE_PATHS = ["tools/repoindex", "index", "docs", "evidence", "scripts"]


def run(command, cwd, env, label):
    try:
        result = subprocess.run(command, cwd=cwd, env=env, text=True,
                                capture_output=True, timeout=600)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(label + " timed out") from error
    if result.returncode:
        raise RuntimeError(label + " failed: " + (result.stderr or result.stdout)[-2500:])
    return result.stdout


def snapshot(directory):
    return {p.relative_to(directory).as_posix(): [p.stat().st_size, p.stat().st_mtime_ns]
            for p in directory.rglob("*") if p.is_file()}


def accept(repository, ref, temporary_parent):
    if not shutil.which("git"):
        raise RuntimeError("Git is required; no packages are installed by this check")
    temporary_parent.mkdir(parents=True, exist_ok=True)
    env = {k: v for k, v in os.environ.items() if not k.startswith(("PYTHON", "PIP"))}
    env["GIT_TERMINAL_PROMPT"] = "0"
    with tempfile.TemporaryDirectory(prefix="fresh-clone-", dir=temporary_parent) as temporary:
        folder = Path(temporary)
        checkout = folder / "checkout"
        print("Cloning a fresh index-only snapshot (network required for Git)...", file=sys.stderr, flush=True)
        run(["git", "-c", "core.hooksPath=/dev/null", "clone", "--filter=blob:none", "--sparse",
             "--single-branch", "--branch=" + ref, "--", repository, str(checkout)],
            folder, env, "Git clone")
        run(["git", "sparse-checkout", "set"] + SPARSE_PATHS, checkout, env, "Sparse checkout")
        revision = run(["git", "rev-parse", "HEAD"], checkout, env, "Read revision").strip()
        before = snapshot(checkout / "index")
        home = folder / "empty-home"
        home.mkdir()
        query_env = dict(env, HOME=str(home), XDG_CACHE_HOME=str(home / "cache"),
                         GIT_NO_LAZY_FETCH="1", HTTP_PROXY="http://127.0.0.1:9",
                         HTTPS_PROXY="http://127.0.0.1:9", ALL_PROXY="http://127.0.0.1:9")
        python = [sys.executable, "-I", "-S"]
        print("Checking cold/warm queries with no site packages, sources or ingestion...",
              file=sys.stderr, flush=True)
        report = json.loads(run(python + [str(ROOT / "scripts/_fresh_clone_probe.py"), str(checkout)],
                                home, query_env, "Isolated query probe"))
        if report.get("completed") is not True:
            raise RuntimeError("Query probe did not certify completion")
        # Also exercise shipped launchers as ordinary separate processes.
        commands = [(["tools/repoindex", "workspace", "files", "*AudioFlinger*", "--json"], True),
                    (["scripts/refsearch.py", "impl", "BPSNode::ExecuteProcessRequest", "--limit", "3"], True),
                    (["scripts/reindex.py", "--help"], False),
                    (["scripts/benchmark_search.py", "--help"], False),
                    (["scripts/check.py"], False)]
        for arguments, json_lines in commands:
            output = run(python + arguments, checkout, query_env, "Launcher " + arguments[0])
            if not output.strip() or len(output.encode()) > 12000:
                raise RuntimeError("Missing or excessive launcher output: " + arguments[0])
            if json_lines:
                for line in output.splitlines():
                    json.loads(line)
            report["checks"].append({"check": "launcher " + arguments[0], "exit": 0})
        for relative, expected in report.pop("database_stats").items():
            path = checkout / relative
            if [path.stat().st_size, path.stat().st_mtime_ns] != expected:
                raise RuntimeError("A query changed its restored database")
        if snapshot(checkout / "index") != before:
            raise RuntimeError("A query changed a bundle or manifest")
        if run(["git", "status", "--porcelain"], checkout, query_env, "Check clean worktree").strip():
            raise RuntimeError("The fresh clone has unexpected tracked/untracked changes")
        for tree in report["trees"]:
            if (checkout / tree).exists():
                raise RuntimeError("A source tree was materialized")
        report.update(reference_revision=revision, data_and_worktree_unchanged=True,
                      runtime_installs_run=False, real_corpus_rebuilt=False)
        return report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", default=REPOSITORY, help="Git URL/path; default published references_code")
    parser.add_argument("--ref", default="main", help="Branch or tag to clone (default main)")
    parser.add_argument("--report", type=Path, help="Optional JSON report; no report file by default")
    parser.add_argument("--temp-dir", type=Path, default=ROOT / ".cache", help="Temporary parent; default workspace .cache")
    args = parser.parse_args(argv)
    started = time.monotonic()
    report = {"completed": False}
    code = 0
    try:
        report = accept(args.repository, args.ref, args.temp_dir.resolve())
        report["elapsed_seconds"] = round(time.monotonic() - started, 3)
    except (OSError, ValueError, RuntimeError) as error:
        report.update(completed=False, error=str(error))
        print("Fresh-clone check failed: " + str(error), file=sys.stderr)
        code = 1
    if args.report:
        try:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        except OSError as error:
            print("Cannot write requested report: " + str(error), file=sys.stderr)
            code = 1
    if code == 0:
        print("Fresh index-only clone passed: %d checks, both ready indexes, no installs or rebuilding" % len(report["checks"]))
    return code



if __name__ == "__main__":
    raise SystemExit(main())
