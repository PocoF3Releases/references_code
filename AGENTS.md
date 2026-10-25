# Android reference workspace

Use ready indexes; never rebuild, deep-verify or benchmark at chat startup.
Historical references are separate from the active Android 17 build/device.

```bash
python3 tools/repoindex workspace impl 'NAME_OR_CODE' --scope TOPIC --limit 10
python3 tools/repoindex workspace source TREE/path.cpp --line N --limit 20
```

`files` locates paths; `symbol` finds header declarations; `impl` searches complete
.cpp/.cc bodies. All configured trees are selected by default. `scopes` lists
named paths. `status` shows saved coverage/cache readiness, not live freshness.
Cold caches restore automatically from committed chunks without source scans.

Output defaults to compact text, 20 results and 12 KB; use `--json` for machines.
Narrow selection before increasing limits. Read small ranges, never whole bodies
or inventories. Use live `text`/rg for regex, recent edits and other languages.

Read [search](docs/SEARCH.md) or [sources](docs/SOURCE_MAP.md) only as needed.
Generic tool behavior/tests belong in `tools/repoindex`; workspace data belongs in
`repoindex.json` and `evidence/`. Preserve source bytes/licenses and legacy branches.
No ROM build/flashing is implied. Shared context: https://github.com/PocoF3Releases/Agents.md .
Use Context7 only for a specific library gap; reuse resolved IDs/versions.
