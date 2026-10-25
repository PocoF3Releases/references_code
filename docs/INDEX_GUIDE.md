<!-- repoindex:begin -->
# Index snapshot

Stored provenance only; not a live freshness check. No source scan is needed to query.

| Tree | Files | Header symbols | Implementations | SQLite MiB | Bundle MiB | Chunks |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| LA.UM.8.12.r1-11900-sm8250.0 | 44561 | 225021 | 2490 | 132.8 | 20.1 | 5 |
| android-t-qcom | 212048 | 2977498 | 72079 | 1995.1 | 380.8 | 63 |

All configured trees are selected by default; narrow with `--tree` or `--scope`.
Queries restore verified caches once; they never rebuild source indexes.

Use `python3 tools/repoindex workspace files QUERY` to locate paths, `impl QUERY` for bodies,
and `source PATH --line N --limit 20` for small indexed ranges.
Source code is a snapshot; header extraction is heuristic and body search is literal.
Raw chunks are <=32 MiB, compressed chunks <=50 MiB. Refresh only after source changes.
<!-- repoindex:end -->
