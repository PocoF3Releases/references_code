# Android reference searches

Use the pinned tool here (Linux/WSL). Ready indexes cover `android-t-qcom` and
`LA.UM.8.12.r1-11900-sm8250.0`. Never rebuild, verify or benchmark at startup;
a cold cache restores from committed chunks once, without scanning sources.

```bash
python3 tools/repoindex workspace files '*AudioFlinger*' --limit 5
python3 tools/repoindex workspace symbol audio_stream_out_t --limit 3
python3 tools/repoindex workspace impl 'checkAttributionSourcePackage' --limit 5
python3 tools/repoindex workspace source android-t-qcom/hardware/libhardware/include/hardware/audio.h --line 280 --limit 15
```

| Need | Mode |
| --- | --- |
| Paths of any file type | `files` substring/glob |
| C/C++ declaration: type, macro, enumerator, typedef, prototype, fn-pointer field | `symbol NAME` |
| Literal in C/C++ incl. headers, Java/Kotlin, AIDL/HAL, XML, mk/bp/rc/te, py/sh, proto/json/txt, dts | `impl` |
| Bounded lines of an indexed file | `source PATH --line N` |
| Regex, camera tuning XML | scoped `text` (live files) |

Narrow with `--scope` (`scopes`: 24 topics) or `--tree`. Identical prebuilt/VNDK
copies print once as `(+N identical copies)`. Defaults: 20 results, 12 KB output;
`--json` for machines. Indexed modes beat grepping whole trees and work without
source folders. [Search recipes](docs/SEARCH.md) only as needed;
[maintenance](docs/MAINTENANCE.md) owns refresh/acceptance checks.
Tool code/docs/tests belong in `tools/repoindex` (develop in `~/tools/repoindex`);
config/evidence belong here. Preserve source bytes, licenses and legacy branches.
These references are separate from active Android 17 build/device acceptance.
Workhub: https://github.com/PocoF3Releases/Agents.md .
Use Context7 only for a specific documentation gap; reuse resolved IDs/versions.
