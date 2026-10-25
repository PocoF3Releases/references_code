# Android reference searches

Use the pinned tool in this checkout (Linux/WSL). Ready indexes cover both
`android-t-qcom` and `LA.UM.8.12.r1-11900-sm8250.0` by default. Never rebuild,
verify, benchmark or dump inventories at chat startup. Cold caches restore from
committed chunks automatically, without scanning sources or calling a service.

From the checkout root, find a hit, then inspect a small indexed range:

```bash
python3 tools/repoindex workspace files '*AudioFlinger*' --limit 5
python3 tools/repoindex workspace impl 'AudioFlinger::checkAttributionSourcePackage' --limit 3
python3 tools/repoindex workspace source android-t-qcom/frameworks/av/services/audioflinger/AudioFlinger.cpp --line 283 --limit 12
```

`scopes` lists topics. Narrow with `--scope camera` or `--tree android-t-qcom`;
`symbol NAME` finds heuristic C/C++ header declarations. Defaults: 20 results,
400-character excerpts, 12 KB stdout; use `--json` for machines. Indexed modes work
without source folders. Use scoped live `text`/rg for edits, regex and other
languages. Status/audits are saved evidence, not a future freshness guarantee.

Read [search recipes](docs/SEARCH.md) only as needed; it routes all modes and
index-only checkout. [Maintenance](docs/MAINTENANCE.md) owns explicit checks/refresh and the one-command
fresh-clone acceptance check; none is a startup prerequisite.
Tool code/docs/tests belong in `tools/repoindex`; config/evidence belong here.
Develop upstream in independent `~/tools/repoindex`, retain this standalone copy.
Preserve source bytes/licenses and legacy branches. These historical references
are separate from active Android 17 build/device acceptance.
Shared workhub: https://github.com/PocoF3Releases/Agents.md .
Use Context7 only for a specific documentation gap; reuse resolved IDs/versions.
