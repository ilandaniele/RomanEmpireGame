# C017E — original audio assets and native feedback

2026-10-06. C017E/M6.5 CLOSED. Seven real persisted assets and canonical native gameplay playback; full/strict/Shipping/cook gates passed. Whole M6 and trusted-player gates remain open.

`Tools/roman_audio_recipe.py` schema1 defines six original deterministic feedback tones: `S_RomanSelect`, `S_RomanOrder`, `S_RomanComplete`, `S_RomanImpact`, `S_RomanVictory`, `S_RomanDefeat`. Mono PCM16/24kHz, smooth attack/release, short decaying harmonics, bounded peak below0.8 and trailing silence. These are synthetic cues, not historical recordings, voice content, licensed samples or a background score. Project-authored proprietary source; no external asset/library/service. Source WAVs are reproducible from the recipe in `Artifacts/RomanAudioSource/schema1`, not required repository inputs.

`Tools/author_audio_assets.py` creates `SC_RomanSFX` and the six Sound Waves under `/Game/Audio/RomanFeedback`. It is readonly by default; `--apply` creates missing assets only. Existing assets must have the correct type/`RomanAudioSchema=1`; never reimported or overwritten. Existing generated source WAV bytes must match, or the operation refuses. Resolved output paths must stay inside the project artifact root. Actual format/duration, non-looping playback, volume/pitch defaults, SFX class reference and recipe PCM fingerprint are verified before each first explicit save. No live MCP is available; sequential existing Python commandlets are the actual authoring mechanism. No connection, plugin/config/dependency or Windows security changes.

Evidence:

- `C017E-RecipeTests.log`: pure standard-library2/2 pass for deterministic/distinct PCM, WAV format, positive energy, bounded peak, initial/trailing silence and unknown ID rejection. No files written by those tests.
- `C017E-AudioAuthoring1.log`: seven creations exit0, no Error/Warning lines. `C017E-AudioVerify1.log`: fresh process verifies all seven exit0, no Error/Warning lines.
- `C017E-AudioApplyRepeat.log`: all created=False, exit0, all seven saved SHA256 unchanged against `Artifacts/C017E-AudioHashes.json`.
- `C017E-AudioRejections.log`: three temporary fingerprint/ownership/looping violations rejected, finally-restored, no saves, exit0. Source hashes remain unchanged.
- Editor build `C017E-AudioTests-Build.log`69 actions succeeded. Native `C017E_ImportedPCM1`1/1 clean: `USoundWave::GetImportedSoundWaveData` decodes the actual saved PCM; exact per-wave byte length/MD5, mono/rate/duration/SFX class and non-looping state asserted. One-byte corruption changes the actual fingerprint. This is real sample content, not metadata-only/empty-cue existence or an assertion that the user heard it.
- Final current-tree strict `C017E-IWYU-AssetsFinal.log`95 succeeded; `C017E_AssetsFinal`2/2 clean (Material+Audio, zero warnings/failures/pending/notRun). Repository0openM1warnings/LFS/diff pass, all19 source material/audio assets retain recorded SHA256. No process retained.

Native PCM lengths respectively10560/13920/33600/10080/62880/55200 bytes. The asset-stage evidence above covers source/sample integrity; runtime and cook evidence follows. Previous full104/asset-only strict95 are historical, superseded by current runtime gates below.

## Native integration

`URomanFeedbackSubsystem` is automatic and Game/PIE-only, disabled on dedicated servers. Reflected class defaults reference six waves; a reflected pool owns exactly eight transient registered non-auto-destroy components. No ticks, timers, world scans or event-time synchronous loads. Validated actual local-player/current-world/begun-play/teardown guards reject inappropriate playback. Per-cue0.1-second wall-clock coalescing and bounded round-robin recycling handle bursts without changing gameplay or simulation time.

Accepted controller selections/orders, player-owned completed construction, committed combat health damage and the canonical tactical result dispatch feedback. Impact is spatial with200cm inner radius/1600cm falloff; other cues are2D. RTS listener position follows the existing camera Pawn's ground focus, avoiding inaudible effects below its high camera; possession retains the engine listener. Master1/SFX0.65 default, finite[0,1] setters project onto active/reused voices immediately. Settings UI/persistence remain M6.8. Terminal feedback deduplicates the same encounter after the short coalescing interval. Teardown unbinds, stops/unregisters/destroys voices and permits actual world GC.

Retired five unused tracked sources: `Audio/{AudioManager.h,AudioManager.cpp,SoundData.h}`, `Content/Scripts/CreateAudioAssets.py`, `Content/Audio/AudioConfig.json`. Native caller check found only the old files; exact ASCII/UTF16 content scan found zero legacy binary symbols. Both factory routes and MasterSetup's empty-cue step removed. Old null5SFX/3music rows, global actor-scan getter and unowned raw-this crossfade timer are no longer compiled. Source remains Git-recoverable; no binary deleted. Python audio recipe/retirement5/5 pass. Full legacy setup remains unvalidated, not an approved bootstrap.

Actual integration proof: `C017E_RuntimeFinal`4 audio +3 physical battle/terminal tests=7/7 clean, exit0. Tests use the actual PIE audio device, imported sounds, active components, canonical selection/order/combat/construction, real defeat, ten real defender casualties followed by engine-overlap occupation/capture and real victory. Killing defenders alone correctly leaves InProgress and no victory sound. Initial red fixture expected victory without occupation; fixed the test, not battle policy, and removed temporary probes. Repeated terminal event after0.12seconds does not start another voice. Invalid cue/location/NaN levels, live mute/unmute, eight-voice bound, GC ownership and actual PIE shutdown/collection pass. Report is clean; startup engine self-test/layout messages are not project test failures or suppressed checks. Previous GC crash after a new PIE exposed a real raw static mesh cache in GameMode: it now uses validated weak references and reloads only during bootstrap.

## Final gates

- `C017E-Full.log`:108/108 clean, exit0, Artifacts/Automation/20261006_203619_208/index.json; warnings/failures/pending/notRun0. Actual audio device starts imported cues; this is not a subjective listening approval.
- `C017E-IWYU-RuntimeFinal.log`:96 actions succeeded; `C017E-Shipping.log`:14 succeeded. No dependency/plugin/config changes required by audio.
- `C017E-Cook.log`: exit0,579 cooked/0 incremental skips/7 platform skips/586 total, no Error/Warning lines. Actual cooked AssetRegistry.bin parsed by engine `DumpAssetRegistry`, `C017E-CookedRegistry.log` exit0/no errors or warnings. `Artifacts/C017E_CookedRegistry/Page_00000.txt` contains all seven exact `/Game/Audio/RomanFeedback/<name>.<name>` paths, including SFX class. This checks the actual Zen cooked registry, not loose/source existence.
- Python recipe/retirement5 pass; repository0openM1warnings/LFS/diff pass;12materials+7audio retain original recorded SHA256. No active build/test/cook process at closure. Prior failed diagnostics retained; no suppressed checks or battle policy changes.

M6.5 is closed as functional original feedback, not a promise of music/ambience/voice, subtitles, historical recordings or subjective listening quality. Settings UI/persistence remain M6.8; standalone/trusted-player/performance/distribution and whole M6–M8 gates remain open. Next C017F/M6.6 retires the separate unused procedural helper, preserving native StaticMesh placeholders.

Authoring command (omit `--apply` for readonly; substitute `--test-rejection` for readonly negative probes; do not combine):

```powershell
$audioScriptArg = '-script="' + (Join-Path $PWD 'Tools/author_audio_assets.py') + '" --apply'
& 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' "$PWD\RomanEmpireGame.uproject" -run=pythonscript $audioScriptArg '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' -DisablePlugins=Fab -unattended -nop4 -NullRHI -stdout -FullStdOutLogOutput
```

Pure guard: `python -B -m unittest discover -s Tools/tests -p test_audio_recipe.py`. Directory is lowercase; use exact casing for portable clients. Generated Python bytecode is ignored, not staged or deleted.
