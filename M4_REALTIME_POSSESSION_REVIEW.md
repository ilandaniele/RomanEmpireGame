# C-009 — Real-time possession

Verified 2026-10-05, UE 5.8.2, Windows Editor. Implements the user-approved M4.8 policy: no slow motion or pause during possession. Runtime was already correct and remains unchanged.

The existing real-PIE PlayerOwnership fixture now reads actual WorldSettings: global, cinematic, replay and effective dilation stay exactly 1; the world is not paused. Custom dilation stays 1 for the player controller, tactical camera, controlled soldier, other soldiers/enemy and retained AI controller where available.

Checks run before/during possession, throughout all three enter/exit cycles, after ownership/player-faction loss and camera-activation interruptions (including lethal damage/destruction), and before/after controller EndPlay. Existing same-AI, selection, transform, pitch and HUD assertions remain.

Evidence:

- Editor build 38/38 actions, exit 0: `Artifacts/C009-Editor.log`.
- Directed `RomanEmpireGame.Units.PlayerOwnershipPIE`: 1/1, zero warnings/errors/pending, exit 0. Report `Artifacts/Automation/C009_Targeted/index.json`.
- Strict IWYU 69/69 actions, exit 0: `Artifacts/C009-IWYU.log`.

No runtime/module/config/asset change, Shipping rebuild, new full-suite run, rendered or packaged-player claim. C-008 full 38/38 is historical runtime regression evidence, not execution of these strengthened assertions. M4 animation/collision, physical input, battle resolution and trusted-player gates remain open.

Skills: `ue-testing-debugging`; `ue-project-context` records only authorized framework/verification updates.
