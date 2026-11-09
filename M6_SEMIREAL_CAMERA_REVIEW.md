# M6 — Private real-camera candidate review

2026-10-19, GPT, C-018H_SemirealCameraPreview. Current technical candidate is
Runtime05/DA_RuntimePreview with IntegrationV3, ONLY in GamePreview. No main art
adoption or owner visual acceptance. Source GLB/LOD/skin remains semireal_v2.

## Real rendering / discoveries

Private fixture Tools/ArtProbe/Native/SemirealCameraPreview.cpp uses the native
game controller/pawn and actual game viewport pixels, D3D12/SM6. No SceneCapture
proxy, fake screenshot, image generation or changed FOV. Production default
tactical zoom=.55; real first-person possession/hide-head/exit restoration.
Images are 1280x720 game viewport renders, NOT Slate/UMG or packaged-player proof.
Paired screenshots hide skeletal meshes only; actor/camera/state remain fixed.

RTS01/FPS01 passed native1/1 each, one known MotionVectorSimulation engine warning
each, zero errors, exit0. Real handedness right+50.189cm/left-50.189cm. Visibility
witness changed1920RTS/38704FPS pixels; images inspected. RTS units are small at
the production tactical zoom. Original receipt camera position was sampled from
the SETUP cache before frames, not the actual capture; newer fixture records it
at image capture and asserts viewtarget=pawn. Old receipts/images preserved.

FPS inspection revealed inherited slotCrest actually represents BRASS fittings
(rivets/hilt/trim) in this model. Tinting it made gold metal purple. New candidate
uses Body+Shield ONLY; default legacy slot policy unchanged. Runtime04 author and
directed material-policy test passed; source materials03-author backup retained.

The stronger FPS02 test then caught real clipping: attack-hand screenX1765 was
outside1280 and a close arm/sleeve obstructed the right edge. Both image and error
remain. Positive mesh pixels alone were insufficient. No projection threshold or
camera/FOV/collision was relaxed. Extra compile02 failure was a nonliteral
FString::Printf format; fixed with a checked literal, failed log retained.

## Same-body local-player pose correction

RomanSoldierAnimInstance adds a presentation-only local-player snapshot and
component-space hand/elbow targets. It derives control eligibility from the
actual local player controller, not merely a possession flag. Legacy graphs do
not consume these fields. No new gameplay timer, hit/damage callback, collision,
camera offset, FOV, fake viewmodel or mesh scale. Targets are calculated using
actor/mesh transforms (including existing reflected axes); optional visual reach
pulse reads the existing montage's normalized position, not another clock.

IntegrationV3 duplicates the original verified graph/new-rig clips, inserting
local→component→two-boneIK→local after montage and before death. One hand_r solver,
no stretching, component-space targets, alpha bound to local-first-person only.
Tactical/non-player/death authority remains unchanged. New Runtime05 uses V3 and
keeps brass untinted. V2/Runtime03/04 and old sources remain evidence.

Strict private IKbuild01/02/03 pass; AuthoringV3Applied01 native1/1 clean with actual
schema links/compiler; Runtime05 author/readback10assets passes. CharactersIK01
4/4 clean passes with new negative check: an AI possession flag does NOT produce
local-player IK/targets. Source hashes and final build/suite status in HANDOFF.md.

CameraFPS03 native1/1,0errors/1known enginewarning,exit0. Both Idle and Attack images
inspected: actual hand/grip/sword visible, brass gold, no own head occlusion.
Hand apex=(872.727,546.182), within unchanged1280x720. Same actual camera
(440000,440000,168.15), rotation0; initial visibility changes68217pixels and hiding
ONLY the player's attack mesh changes41690pixels. The NPC remains visible in that
witness. This is actual same-body rendering, not only a playing montage flag.
Hand/face/cloth/animation finesse still require owner art review; not final art.

## Evidence / remaining verification

GamePreview/Artifacts/Camera{RTS01,FPS01,FPS02,FPS03} contain original PNGs and text
receipts. Failed FPS02 is intentionally kept. Reports/Camera*/index.json and
camera-*.log preserve assertions/warnings. Early camera source backed up as
camera-capture01-source.cpp; earlier authors as authoring-v2-source.cpp and
runtime-materials04-author.py. Do not overwrite or relaunch completed captures.

Final RTS02 and FPS04 each native1/1,0errors/1known MotionVectorSimulation warning,
exit0. All current Visible/Attack images inspected. RTS witness1922pixels, actual
camera438273.968/440000/2587.110 at pitch-53.333332 and default zoom.55. FPS camera
unchanged at440000/440000/168.15; hand872.727/546.182, initialmesh68359pixels and
own-attack41443pixels. Exit restores RTS pawn/head and explicitly clears localIK/
targets after a real animation update. Source requires actual viewtarget=pawn;
new readonly authoring checks saved IK node/pins/no-stretch. No material/shader
compile error signatures in current renderer logs. Bootstrap log also contains
generic bootstrap automation “Condition failed” lines before the chosen
test; do not claim the entire raw log is error-free. Chosen reports have0errors.

CameraFull01 final native regression:141success (140clean/1with intentional
missing.png warning),0failed/0pending,exit0. Includes V3 readonly IK node/pin/
no-stretch contract and the strengthened AI/local-player distinction. No WASAPI
or connectivity-probe warning in THIS cohort; prior warnings remain historical.
Final Shipping private camera01 PASS5actions/23.77s; main Editor camera01
PASS11actions/33.99s after confirming no live Editor. No owner process closed or
reopened. Runtime05 fresh readonly10asset receipt PASS/exit0. Source snapshots
main/private byte-exact; repository0M1warnings/diff/Python5guards/PS AST PASS.
No main Content/Config/baseline adoption; generated binaries updated. All owned
processes terminal. Camera-cohort01.json in GamePreview stores exact source/tool/
PNG/receipt SHA256s and current candidate lineage. The retained MotionVectorSimulation warning is not
silently suppressed. Current images: GamePreview/Artifacts/CameraRTS02 and
CameraFPS04; old captures remain historical evidence.

OPEN: owner visual acceptance, animated real-input/player QA, camera pitch/motion
review beyond fixed views, installed LOD/new-model100-unit GPU performance, main
art adoption, VQ02 camp sample and all remaining M7/M8 release/security/CI gates.
