# C017C — retire the unused generated HUD path

2026-10-06, M6.3 closed. Production UI remains the verified responsive native controller-owned `URomanEmpireMainWidget`; no alternative layout or Widget Blueprint dependency.

- Deleted only tracked, unused `Content/Blueprints/UI/WBP_MainHUD.design.json` (13,837 bytes). Recoverable from Git history. The exact UI folder contained no binary `.uasset`; none was deleted. Targeted Source/Config/script reference checks found only the two legacy factories and inventory entry, not a runtime requirement.
- Removed `create_widget_blueprint` and its calls from CreateBlueprints/MasterSetup; they can no longer emit the empty Widget Blueprint or advertise it as a required setup asset. Other generator logic is preserved and remains explicitly unvalidated; this is not approval to run a full setup.
- Updated asset provenance/status inventory. No import, asset save, engine connection, external install, dependency/config/runtime change, staging/commit or Windows security change.
- Added read-only AST/absence guard `Tools/Tests/test_native_ui_retirement.py`: standard-library unittest parses both scripts without importing Unreal or executing setup, rejects factory/function/call/path restoration, and verifies the unused design is absent. Three tests pass (`Artifacts/C017C-NativeUIRetirement.log`).
- C017B full103 clean/rendered6/strictIWYU94/Shipping verify the unchanged native runtime. C017C repository0M1warnings/LFS/diff checks pass. No redundant native rebuild needed for removing an unused JSON and Python factory.

Next C017D/M6.4: actual material instances and readable visual states, then remaining assets/audio/procedural/animation/accessibility/editor generation and M7–M8. Whole M6/goal/trusted-player gates remain open.
