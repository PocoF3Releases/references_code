# Android reference workspace

Historical Android/Qualcomm sources with ready file, header and complete .cpp/.cc
body indexes. Source/tool folders are ordinary Git files; no submodules or LFS.
Queries restore caches automatically and never rebuild source indexes.

```bash
git clone --single-branch --branch main https://github.com/PocoF3Releases/references_code.git
cd references_code
python3 tools/repoindex workspace impl 'AudioFlinger::checkAttributionSourcePackage' --scope frameworks --limit 10
python3 tools/repoindex workspace source android-t-qcom/frameworks/av/services/audioflinger/AudioFlinger.cpp --line 283 --limit 20
```

Both trees are selected by default. `--tree` selects one; `--scope` selects a topic.
Output is human-readable and bounded; add `--json` for JSON Lines. No initial scan,
model service or Python package installation is needed. Linux/WSL and SQLite with
FTS5/trigram support are required; ripgrep is optional for live text searches.

- [Search recipes and index-only checkout](docs/SEARCH.md)
- [Source provenance](docs/SOURCE_MAP.md)
- [Current sizes/counts](docs/INDEX_GUIDE.md)
- [Validation and evidence](docs/VALIDATION.md)
- [Workspace maintenance](docs/MAINTENANCE.md)
- [Standalone repoindex documentation](tools/repoindex/README.md)

`repoindex.json` owns tree names, scopes and benchmark cases. `index/` owns ready
bundles; `evidence/` owns current source/audit/measurement records. Generic code,
documentation and tests live together in `tools/repoindex/`, copied from the
[canonical tool](https://github.com/johnmart19/repoindex). Existing `scripts/`
commands remain small compatibility shims.

Source contents/notices are preserved. These partial historical references do not
establish Android 17 buildability or device behavior. Active project decisions:
[workhub](https://github.com/PocoF3Releases/Agents.md).
