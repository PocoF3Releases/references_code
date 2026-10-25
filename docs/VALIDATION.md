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

The reusable [fresh-clone check](MAINTENANCE.md#fresh-index-only-acceptance)
passed 29 acceptance checks against a real GitHub sparse clone with no source
folders or initial caches. The [report](../evidence/fresh-clone.json) records zero
third-party distributions, no pip/installed repoindex, automatic restoration of
both indexes, guarded API/CLI queries, bounded output, expected errors and
unchanged bundles/databases/worktree. Its temporary clone was removed afterward.
The command never invokes source ingestion or synthetic fixture builds.

Separately, 81 canonical fixtures and 3 consumer-harness regressions pass. The
consumer tests cover the guarded probe, missing chunks under Python optimization,
and cleanup/reporting after clone failure. Their inputs are tiny synthetic trees;
they do not rebuild either real source index. Python/SQLite remain runtime
prerequisites; this is not a certification of every platform/version.
