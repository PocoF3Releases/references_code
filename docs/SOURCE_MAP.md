# Source map

Prepared 2026-10-25. All entries below are ordinary files within the main Git
repository. Source directory names do not establish completeness or buildability.

| Root | Provenance and role |
| --- | --- |
| `android-t-qcom/` | Existing unversioned Android T Qualcomm source snapshot; exact upstream manifest unavailable. Imported as-is in the clean snapshot, including prebuilts and its original generated header. |
| `LA.UM.8.12.r1-11900-sm8250.0/` | Exact tree from existing Qualcomm SM8250 reference commit `ddf563c1d9ebfbc92452d7306a87e2c18ee40e7c` on the preserved historical branch of this remote. |
| `tools/repoindex/` | Vendored johnmart19/repoindex 2.1.1, upstream revision `549b2eec1cdf6424933604c2a3002c63433dbffe`; improvements also pushed upstream. See `UPSTREAM.md`. |
| `index/<source-name>/` | Portable per-tree manifest and compressed SQLite slices, committed in Git. |
| `repoindex.json` | Roots, exclusions, scopes, benchmark cases and guide settings. |
| `.cache/` | Ignored restored SQLite databases, writer locks, diagnostics and original nested Git metadata backup. |

Android contains 212,048 files / 3,157,863,915 apparent bytes; Qualcomm contains
44,561 files / 4,314,522,968 apparent bytes. [source inventory](../evidence/sources.json) records component
counts. VCS internals and preparation-generated Python caches are not source files.

Camera routes to camx/chi-cdk, audio to mm-audio/calibration/Bluetooth plus framework
AV, display to Qualcomm display/framework native, and performance to android-perf
and thermal-engine. `scopes` lists exact paths. Prebuilts and external libraries
have their own scopes. These are entry points; follow cross-component references.

Legacy framework, ART, bionic and vendor branches are retained as separate
historical views. They were not automatically merged into this snapshot. Keep
exact source revisions when making comparisons with the active Android 17 tree.
