# Validation

The ready data still covers 256,609 files and every selected implementation:

| Tree | All files | Complete .cpp/.cc bodies | Header symbols | Include edges |
| --- | ---: | ---: | ---: | ---: |
| `android-t-qcom` | 212,048 | 72,079 | 2,977,498 | 458,588 |
| `LA.UM.8.12.r1-11900-sm8250.0` | 44,561 | 2,490 | 225,021 | 20,546 |

The 2026-10-25 [data audit](../evidence/verification.json) checked every chunk/DB
hash, SQLite/FTS consistency, logical coverage, stored text hashes and every raw/
normalized implementation hash. Other live files were checked by path/size/mtime.
No data rebuild was needed for the organization cleanup: source-tree and database
hashes are unchanged. Text normalization is disclosed in the tool's
[format reference](../tools/repoindex/docs/FORMAT.md).

The standalone suite contains 81 fixtures without resource warnings, including all six migrated consumer
contracts, source-free queries, output/JSON budgets, guide ownership and explicit
maintenance completion. It requires no consumer checkout or real source scan.
Current [measurements](../evidence/benchmarks.json) and the generated
[index guide](INDEX_GUIDE.md) provide observed speed and sizes; measurements are
not universal guarantees.

A source-free package acceptance test restores both real bundles, queries bodies,
reads small indexed ranges, checks compatibility shims and confirms default output
budgets. Saved audits do not guarantee future freshness. No compiler/ROM/device
acceptance is implied by file/header/body search coverage.

Repoindex 2.1.1 packaging also passed an isolated sdist-to-wheel acceptance check,
covering installed console/module/API entry points, every legacy command and
synthetic source-free restoration. Parser, normalization and matching function
ASTs match the previous release. The tested host uses Python 3.14; this is not
a runtime certification of every supported Python/SQLite version.

The final cleanup adds regression coverage for empty selections, query-only scope
flags on maintenance, EOF ranges, cross-selection benchmark comparisons, malformed
input and atomic/restore failures. Review agents checked correctness/security/
performance and documentation/architecture; no further required findings remained
in their reviewed scope. Workspace checks validate docs/config/part sizes without
restoring databases or scanning source trees.

A real [fresh GitHub sparse clone](../evidence/fresh-clone.json), with no source
folders or existing caches, passed 29 acceptance checks and all 81 synthetic tool
tests. Its isolated Python environment contained zero third-party distributions,
no installed repoindex and no pip. The first ordinary query restored both caches;
file/header/body/range searches worked for each tree and both-tree API queries
passed with network, subprocess and ingestion guards. JSON/text budgets, scope
intersections, compatibility scripts, nested/outside-workspace discovery and
expected missing-live-source errors passed. Bundles, restored DB mtimes and the
Git worktree stayed unchanged. Only runtime Python/SQLite and the checkout are
needed; the isolated environment was test isolation, not a setup requirement.
