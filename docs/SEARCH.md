# Search

Run from this checkout in WSL/Linux, using its pinned tool. Both ready trees are
selected by default: `android-t-qcom` and `LA.UM.8.12.r1-11900-sm8250.0`.
No query invokes ingestion or an implicit rebuild; do not run `index`, `implement`,
`optimize`, `verify` or benchmarks merely to start a chat. A small compressed bundle
or cold cache is not evidence of missing coverage.

Find a candidate, inspect a body hit, then read only the useful line range:

```bash
python3 tools/repoindex workspace scopes
python3 tools/repoindex workspace files '*AudioFlinger*' --limit 10
python3 tools/repoindex workspace symbol BPSNode --scope camera
python3 tools/repoindex workspace impl 'BPSNode::ExecuteProcessRequest' --scope camera --limit 5
python3 tools/repoindex workspace source LA.UM.8.12.r1-11900-sm8250.0/camx/src/hwl/bps/camxbpsnode.cpp --line 1351 --limit 20
```

| Need | Mode / coverage |
| --- | --- |
| Paths in any language, build files or prebuilts | `files` substring/glob |
| Exact C/C++ header type/macro/global | `symbol NAME` (heuristic) |
| Method name, function-body fragment, comment or string in .cpp/.cc | `impl 'literal'` |
| Small indexed implementation range after a hit | `source PATH --line N --limit 20` |
| Live edits, Java/Kotlin/C/other suffixes, or regex | scoped `text`/`rg`; needs live files |

To select only one tree:

```bash
python3 tools/repoindex workspace files '*AudioFlinger*' --tree android-t-qcom --limit 5
python3 tools/repoindex workspace impl 'BPSNode::ExecuteProcessRequest' --tree LA.UM.8.12.r1-11900-sm8250.0 --limit 3
```

Use a named `--scope`, or a direct `--path-prefix tree/component`. Either intersects
with repeatable `--tree` selections. Topics include camera, audio, display, radio,
performance, frameworks, hardware, vendor, apps, system, external and prebuilts;
`scopes` shows exact paths. An empty intersection returns no hits. Defaults: 20 results, 400-character excerpts,
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

Root configuration and `AGENTS.md` are included by cone-mode sparse checkout.
Run queries immediately: no pip install, virtual environment, setup script,
submodules, LFS or rebuild step is needed. The first ordinary indexed query
restores selected caches automatically from local chunks; no `restore` command
is required. Both current caches total about 2.1 GiB after decompression from
about 401 MiB of bundles. This one-time work is restoration, not indexing.
Indexed queries then need no network.

Add a needed source
component with `git sparse-checkout add android-t-qcom/frameworks/av` before live
search/editing. Source-free queries and indexed ranges were acceptance-tested in a real fresh
GitHub clone with no installed packages or existing caches; see the optional
[record](../evidence/fresh-clone.json).

The existing `scripts/refsearch.py` command forwards to the same frontend. It keeps
JSON for indexed results and plain live/scopes output, with its historical 40-result
default. `scripts/reindex.py` and `scripts/benchmark_search.py` are also compatibility
shims; the generic implementation is maintained only in repoindex.

For tool development or optional isolated installation, use the
[package guide](../tools/repoindex/docs/DEVELOPMENT.md). Search commands above
use the pinned workspace copy, not a potentially different global installation.

## When something is missing

`status` reports saved coverage; it is not a prerequisite for each query. A missing
cache restores from ready chunks. If a query fails, check spelling, case, tree and
scope, then `--help`; do not interpret zero matches as permission to rebuild.
`impl` is case-sensitive and limited to indexed suffixes; use `files` first or live
text for another language. `source` past EOF returns no rows. Indexed line numbers
refer to the saved snapshot, so check live code before editing or backporting.

For an incomplete clone, obtain the missing manifest/chunks from this repository
instead of scanning source. For changed source, follow explicit maintainer refresh
instructions in [maintenance](MAINTENANCE.md); old audits do not prove freshness.
Development/maintenance checks are opt-in: `python3 scripts/check.py` checks this
workspace's documentation/configuration/part sizes without restoring or scanning.

For an explicit end-to-end release check, run
`python3 scripts/check_fresh_clone.py`; it clones and tests both ready indexes
without installing packages or indexing source. See
[maintenance](MAINTENANCE.md#fresh-index-only-acceptance) for costs/report options.
Ordinary searches do not require this test.
