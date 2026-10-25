# Workspace maintenance

Use [repoindex maintenance](../tools/repoindex/docs/MAINTENANCE.md) for generic
refresh, verification, guide, benchmark and generation rules. This file owns only
workspace-specific policy.

- Keep historical source folders and their bytes/notices unchanged during tooling
  maintenance. Both trees include original prebuilts and one generated header.
- `repoindex.json` owns roots, exclusions, scopes, benchmark cases and guide output.
  Add source paths there, not to Python code.
- Commit only current manifest-referenced chunks. Keep active databases/receipts
  in ignored `.cache/`; never commit raw SQLite files.
- After source changes, refresh only affected trees explicitly. No startup rebuild,
  audit or benchmark. A tool/frontend documentation update alone needs no reindex.

```bash
# Cheap local docs/config/chunk-size checks; no restore or source scan:
python3 scripts/check.py
# Add synthetic tool fixture tests when changing code:
python3 scripts/check.py --tests
# Only after actual source changes:
python3 tools/repoindex workspace index --tree android-t-qcom
python3 tools/repoindex workspace verify --live --hash-implementations
python3 tools/repoindex workspace benchmark --samples 5 --budget-ms 100 --out evidence/benchmarks.json
# Optional development packaging check; uses installed build tools, no network:
python3 tools/repoindex/scripts/check_package.py
```

The configured guide updates after successful CLI maintenance; `workspace guide`
can regenerate it separately. It replaces only its owned block and preserves
curated surrounding text. Current evidence lives under `evidence/`; old prototype
reports are recoverable from the pre-cleanup history backup rather than active docs.

## Tool ownership

`tools/repoindex/UPSTREAM.md` records the canonical revision. Vendor the complete
canonical directory without `.git`; retain the local provenance file. Generic
frontends, checks, benchmarks and tests belong upstream. Search/reindex/benchmark scripts
are thin compatibility routes and must not acquire a second implementation.
`scripts/check.py` owns consumer validation using the shared checker.

Develop in independent `~/tools/repoindex`. The implementation is a package under
`src/repoindex/`; keep that entire directory when vendoring. See its
[development map](../tools/repoindex/docs/DEVELOPMENT.md) for module ownership and
installed-package checks. Refresh from a verified canonical commit; do not link
the consumer to a mutable development checkout.

## History and recovery

The requested organization cleanup consolidated `main` in this repository and in
repoindex after verified Git recovery bundles and exact expected-head leases.
All legacy reference branches were preserved. Backups and original nested Git
metadata are private local recovery material under `.cache/`; they are not needed
to run a fresh checkout. Future updates use normal commits unless a new rewrite is
explicitly authorized. Preserve edits before reconciling an existing rewritten clone.

No ROM builds, device changes, hosted index, scheduled scans or GitHub Actions are
part of this setup. Reference evidence is distinct from active Android acceptance.

Local workspace checks share the canonical Markdown validator instead of keeping
a second implementation. All checks here are explicit maintenance operations,
not startup prerequisites. The source-free acceptance record and saved data audit
are under `evidence/`; only refresh relevant evidence after a meaningful change.
