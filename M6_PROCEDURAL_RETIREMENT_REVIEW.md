# C017F — retire unused procedural mesh helper

2026-10-06. C017F/M6.6 CLOSED. Retired unused helper; current static visuals and cooked assets retained.

Decision: no production procedural helper retained. Exact native caller search finds only Assets/ProceduralMeshGenerators.h/.cpp and the Build.cs dependency; scripts/tests have no references. A readonly content binary scan checks both ASCII and UTF16 symbols ProceduralMeshGenerators/ProceduralMeshComponent and finds zero references. Current world terrain/buildings/unit figures use actual engine StaticMesh primitives, not this helper. Do not remove or redesign those working visuals.

Removed the two unused tracked sources, recoverable through Git; no binary deleted. Removed the runtime module dependency and explicitly disabled the unused ProceduralMeshComponent plugin in the project. Other plugins/config/gameplay/definitions remain unchanged. The helper's GenerateBarracksMesh built a box then overwrote its section with empty arrays; repairing an uncalled prototype would not benefit the retained production path. No new asset/source-media/license/dependency/tool installation.

Readonly guard: `python -B -m unittest discover -s Tools/tests -p test_procedural_retirement.py`3/3 verifies retirement, absent dependency and explicit plugin disablement. Entire Python suite13/13 pass (`C017F-Python.log`). Source guards are not behavioral evidence; actual PIE/native/build/cook proof follows. Full108 clean from C017E is the immediately preceding regression baseline, not a claim of another full run after this removal.

Final gates:

- `C017F-Editor.log`51 actions, `C017F-IWYU.log`95 and `C017F-Shipping.log`50 succeeded, exit0.
- `C017F_Targeted`13/13 clean, exit0, zero warnings/failures/pending/notRun. Actual content discovery/required-map/definition propagation/atomic placement, material actor states, imported PCM, world playback/GC, canonical victory/defeat and physical objective/terminal restart pass with the plugin disabled. `C017F-Targeted.log` and Artifacts/Automation/C017F_Targeted/index.json.
- `C017F-Cook.log` exit0:579 cooked/0 incrementally skipped/7 platform skipped/586 total, no Error/Warning. Actual cooked AssetRegistry.bin parsed by engine (`C017F-CookedRegistry.log`, exit0/no errors or warnings); Artifacts/C017F_CookedRegistry/Page_00000.txt retains all7audio+12material paths. Zen output, not source-file existence.
- All19 source hashes unchanged against previous recorded manifests; repository0openM1warnings (`C017F-Repository-Final.log`), LFS fsck and diff checks pass. No active operation at closure; HEAD0726de2 unchanged, no staging/commit/push. Older dirty/untracked gameplay/asset work preserved.

Remaining M6.7 characters/animation, M6.8 settings/accessibility, M6.9 validated generators and M7–M8 remain open; static prototype figures are not skeletal or production-ready characters. Standalone trusted-player gate remains blocked by security, no repeated unsigned launch/bypass.
