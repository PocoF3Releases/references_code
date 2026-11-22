# Storage and guarantees

Each configured tree has `index/<tree>/manifest.json` plus immutable
`part-<sha256>.gz` files. Sources remain ordinary files. Bundles contain a complete
SQLite database sliced into raw pieces of at most 32 MiB; each compressed file is
capped at 50 MiB. This stays below GitHub's 100 MiB Git-file limit without LFS.
Restore currently supports databases up to 16 GiB.

The manifest records format/schema identity, source inventory and coverage flags,
database hash/size, and each chunk's compressed hash/size and raw size. Paths are
relative, traversal is rejected, decompression is bounded and the assembled DB is
hash/integrity checked before installation. Checksums detect corruption relative
to a trusted Git revision; they do not authenticate malicious manifest changes.

`.cache/repoindex/<tree>/<database-hash>.db` is immutable query storage. A receipt
records verified size/mtime for fast reuse. Changed receipts/files cause restore;
full verification is available explicitly. Readers use SQLite read-only mode.
No complete source-content reads occur during normal indexed queries.

## SQLite ownership

| Data | Tables / views |
| --- | --- |
| File inventory | `dirs`, `files`; `vfiles` reconstructs relative paths |
| Header symbols | `symbols`; `vsymbols` exposes names/kinds/declarations/paths |
| Header includes | `inc_targets`, `includes`; `vincludes` exposes targets |
| Path acceleration | `search_files`, optional external-content `search_paths_fts` |
| Complete bodies | `implementation_bodies` (one row per distinct body), `implementation_paths` (path → body), external-content `implementation_fts`, large-body `implementation_segments` + contentless `implementation_segments_fts`; view `implementation_files` keeps one row per path |
| Provenance/coverage | `meta` |

Header extraction is comment-aware and heuristic: classes, structs, enums, unions,
macros, selected globals, enumerators, typedef names, one-line function prototypes
and function-pointer fields. Names produced by macro expansion (X-macro lists) are
not visible to it; search their text with `impl`. It is not a full C++ grammar or
ABI index. Implementation text covers every selected regular file in full, including
bodies/comments/strings. Source and output completeness are different: display
limits never sample stored text. Directory links are not traversed; nonregular or
unreadable implementation inputs fail publication rather than being silently skipped.

Body text is UTF-8 decoded, CRLF/CR normalized to LF and NUL/invalid bytes shown as
replacement characters. Affected-file counts are recorded. Original sources are
untouched. Raw-source and normalized-body SHA-256 values permit precise auditing.
Mtimes remain integer-second inventory metadata, not content identities.

## Query semantics and acceleration

File searches preserve literal substring or glob rules. SQLite's default lower/
LIKE behavior supplies ASCII case-insensitivity; globs are case-sensitive. Ordered
materialized paths avoid repeated joins and permit early LIMIT stops. Optional
FTS5 trigram candidates accelerate longer path patterns; exact filters remain.

Body search uses whole-file documents so multiline matches cannot disappear at
chunk/block boundaries. Implementation schema 2 stores each distinct normalized
body once (by SHA-256) and indexes it once, so vendored or prebuilt copies cost one
document. With `detail=none`, queries AND every distinct quoted three-codepoint
token of the literal (up to 64, spread evenly), then verify the entire
case-sensitive literal in Python. Bodies above 1M characters also have
index-only (contentless) postings for 256K-character segments that overlap by the
4,096-character query maximum; such a body is read only when one segment holds every
query trigram, so large generated or test-data files stop costing a full read. Candidate false positives cannot become returned hits. One record is
emitted per starting line. By default identical copies collapse into one record
with `copies` and up to three `other_paths`; the first in-scope path is shown.
`symbol` collapses rows with the same kind and declaration likewise.
`--expand-copies` (API `collapse=False`) restores one record per path. Schema 1
bundles remain readable. Short queries work through ordered documents without
trigram acceleration. The result cap spans configured trees in order.

This is literal retrieval, not AST/overload resolution or proof of build/device
behavior. Indexed data is a snapshot. Inspect live source or request an explicit
refresh after source changes; never infer freshness from a restored cache alone.
