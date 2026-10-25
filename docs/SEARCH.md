# Search

Run from this checkout in WSL/Linux. Both source trees are selected by default.
No query invokes source ingestion or an implicit rebuild.

```bash
python3 tools/repoindex workspace scopes
python3 tools/repoindex workspace files '*AudioFlinger*' --limit 10
python3 tools/repoindex workspace symbol BPSNode --scope camera
python3 tools/repoindex workspace impl 'BPSNode::ExecuteProcessRequest' --scope camera --limit 5
python3 tools/repoindex workspace source LA.UM.8.12.r1-11900-sm8250.0/camx/src/hwl/bps/camxbpsnode.cpp --line 1351 --limit 20
```

Use a named `--scope`, or a direct `--path-prefix tree/component`. Either intersects
with repeatable `--tree` selections. Defaults: 20 results, 400-character excerpts,
12,000 stdout bytes. `source` returns a bounded line range. Raise limits only after
narrowing; `--json` produces valid JSON Lines. `impl --context 1` is optional and
bounded. Body text is complete in storage; output clipping never removes indexed
content. Header symbols are heuristic and body matching is literal, not compiler
AST/overload analysis.

`status` reads stored counts/cache readiness only. A cold cache is normal; first
indexed use restores verified chunks into `.cache/repoindex/<tree>/`. Later queries
reuse read-only databases, including during rebuilds. Verify a hit against live
source if freshness matters. For changed files, regex or other suffixes use:

```bash
python3 tools/repoindex workspace text 'exact log message' --scope audio --limit 10
rg -n -F -- 'exact message' android-t-qcom/frameworks/av
```

Live `text` requires source folders. Indexed `files`, `symbol`, `impl` and `source`
work without them. Exact matching, API use, flags and output-limit behavior are
owned by the standalone [usage guide](../tools/repoindex/docs/USAGE.md).

## Index-only checkout

A sparse clone can fetch the query tools/bundles without materializing source blobs:

```bash
git clone --filter=blob:none --sparse --single-branch --branch main https://github.com/PocoF3Releases/references_code.git
cd references_code
git sparse-checkout set tools/repoindex index docs evidence scripts
python3 tools/repoindex workspace impl 'AudioFlinger::checkAttributionSourcePackage' --scope frameworks
```

Root configuration is included by cone-mode sparse checkout. Add a needed source
component with `git sparse-checkout add android-t-qcom/frameworks/av` before live
search/editing. Source-free queries and indexed ranges were acceptance-tested.

The existing `scripts/refsearch.py` command forwards to the same frontend. It keeps
JSON for indexed results and plain live/scopes output, with its historical 40-result
default. `scripts/reindex.py` and `scripts/benchmark_search.py` are also compatibility
shims; the generic implementation is maintained only in repoindex.
