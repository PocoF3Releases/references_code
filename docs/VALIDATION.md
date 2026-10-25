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

The standalone suite contains 61 fixtures, including all six migrated consumer
contracts, source-free queries, output/JSON budgets, guide ownership and explicit
maintenance completion. It requires no consumer checkout or real source scan.
Current [measurements](../evidence/benchmarks.json) and the generated
[index guide](INDEX_GUIDE.md) provide observed speed and sizes; measurements are
not universal guarantees.

A source-free package acceptance test restores both real bundles, queries bodies,
reads small indexed ranges, checks compatibility shims and confirms default output
budgets. Saved audits do not guarantee future freshness. No compiler/ROM/device
acceptance is implied by file/header/body search coverage.
