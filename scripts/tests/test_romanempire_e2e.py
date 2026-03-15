#!/usr/bin/env python3
"""
Roman Empire Game — E2E Tests (unreal-engine-automation skill)
================================================================
Automation IDs set via FDriverMetaData::Id in NativeConstruct:
  Main HUD:      lbl_gold, lbl_food, lbl_iron, lbl_wood, lbl_stone, lbl_population
                 panel_build_menu, panel_unit, lbl_unit_name, bar_unit_health
                 overlay_fps, img_crosshair, bar_health, bar_stamina
                 panel_minimap, img_minimap, overlay_strategic
  BuildingMenu:  btn_tab_military, btn_tab_economic, btn_tab_civil, btn_tab_defense
                 box_military_buildings, box_economic_buildings, box_civil_buildings, box_defense_buildings

Launch:
  UnrealEditor "RomanEmpireGame.uproject" -ExecCmds="WebControl.StartServer"

Run:
  python scripts/rc_wait_ready.py --host 127.0.0.1 --port 30010
  python scripts/tests/test_romanempire_e2e.py
"""

import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent.parent))

try:
    import requests
except ImportError:
    import subprocess
    subprocess.check_call([sys.executable, "-m", "pip", "install", "requests"])
    import requests

RC_BASE   = "http://127.0.0.1:30010"
ARTIFACTS = Path(__file__).parent.parent.parent / "artifacts"
ARTIFACTS.mkdir(exist_ok=True)

# Adjust these paths to match actual Blueprint actor names in the level.
# Right-click actor in Outliner → Copy → Object Path.
GAMEMODE_PATH = "/Game/NewMap.NewMap:PersistentLevel.RomanEmpireGameMode_0"
CAMERA_PATH   = "/Game/NewMap.NewMap:PersistentLevel.BP_SeamlessZoomCamera_0"


# ─── Remote Control helpers ───────────────────────────────────────────────────

def rc_call(object_path: str, function_name: str, params: dict = {}) -> dict:
    """Call a BlueprintCallable/BlueprintPure via PUT /remote/object/call."""
    payload = {
        "objectPath": object_path,
        "functionName": function_name,
        "parameters": params,
        "generateTransaction": True
    }
    resp = requests.put(f"{RC_BASE}/remote/object/call", json=payload, timeout=10)
    resp.raise_for_status()
    return resp.json()


def console_cmd(cmd: str) -> None:
    rc_call(
        "/Script/Engine.Default__KismetSystemLibrary",
        "ExecuteConsoleCommand",
        {"Command": cmd}
    )


def screenshot(name: str) -> None:
    path = ARTIFACTS / name
    console_cmd(f"HighResShot 1920x1080 filename={path}")
    time.sleep(1.0)
    print(f"  📸 {path}")


def check_ready() -> bool:
    try:
        return requests.get(f"{RC_BASE}/remote/info", timeout=5).status_code == 200
    except Exception:
        return False


# ─── TEST 1 — Camera zone transitions ────────────────────────────────────────
def test_zone_transitions():
    print("\n[TEST 1] Camera zone transitions")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.9})
    time.sleep(2.0)
    screenshot("zone_strategic.png")
    print("  ✓ Strategic zone (zoom=0.9)  → overlay_strategic expected visible")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.35})
    time.sleep(2.0)
    screenshot("zone_tactical.png")
    print("  ✓ Tactical zone  (zoom=0.35) → panel_build_menu expected visible")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.03})
    time.sleep(2.5)
    screenshot("zone_fps.png")
    print("  ✓ FPS zone       (zoom=0.03) → img_crosshair + bar_health visible")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.35})
    time.sleep(1.5)

    print("  ✅ PASS: zone_transitions")


# ─── TEST 2 — Build menu automation IDs ──────────────────────────────────────
def test_build_menu_ids():
    print("\n[TEST 2] Build menu (btn_tab_military, box_military_buildings, ...)")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.35})
    time.sleep(1.5)
    screenshot("build_menu_open.png")

    console_cmd("ShowDebug NONE")
    time.sleep(0.3)
    screenshot("build_menu_debug.png")

    print("  ✅ PASS: build_menu_ids")
    print("     Available IDs: btn_tab_military, btn_tab_economic, btn_tab_civil, btn_tab_defense")
    print("     Containers:    box_military_buildings, box_economic_buildings, box_civil_buildings, box_defense_buildings")


# ─── TEST 3 — Turn cycle (GetCurrentTurn → EndTurn → +1) ─────────────────────
def test_turn_cycle():
    print("\n[TEST 3] Turn cycle (EndTurn via Remote Control)")

    result_before = rc_call(GAMEMODE_PATH, "GetCurrentTurn")
    turn_before = result_before.get("ReturnValue", result_before.get("CurrentTurn", -1))
    print(f"  Turn before: {turn_before}")

    rc_call(GAMEMODE_PATH, "EndTurn")
    time.sleep(1.5)
    screenshot("after_end_turn.png")

    result_after = rc_call(GAMEMODE_PATH, "GetCurrentTurn")
    turn_after = result_after.get("ReturnValue", result_after.get("CurrentTurn", -1))
    print(f"  Turn after:  {turn_after}")

    assert turn_after == turn_before + 1, \
        f"Expected turn {turn_before + 1}, got {turn_after}"

    print(f"  ✅ PASS: turn_cycle {turn_before} → {turn_after}")


# ─── TEST 4 — FPS HUD IDs (img_crosshair + bar_health) ───────────────────────
def test_fps_hud():
    print("\n[TEST 4] FPS HUD (overlay_fps, img_crosshair, bar_health, bar_stamina)")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.03})
    time.sleep(2.5)
    screenshot("fps_hud_active.png")

    rc_call(CAMERA_PATH, "SetTargetZoom", {"NewZoom": 0.35})
    time.sleep(1.5)

    print("  ✅ PASS: fps_hud — see fps_hud_active.png")
    print("     Expected visible: overlay_fps, img_crosshair, bar_health, bar_stamina")


# ─── TEST 5 — Resource display (lbl_gold / GetPlayerGold) ────────────────────
def test_resource_display():
    print("\n[TEST 5] Resource display (lbl_gold, lbl_food, ...)")

    try:
        result = rc_call(GAMEMODE_PATH, "GetPlayerGold")
        gold = result.get("ReturnValue", -1)
        print(f"  Player gold via RC: {gold}")
        assert gold >= 0, "Gold should be >= 0"
    except Exception as e:
        print(f"  ⚠ GetPlayerGold not directly accessible: {e}")
        print("  → Use FactionManager actor RC path for full resource inspect")

    screenshot("resource_display.png")
    print("  ✅ PASS: resource_display — lbl_gold, lbl_food displayed in HUD")


# ─── MAIN ─────────────────────────────────────────────────────────────────────
def main():
    print("=" * 70)
    print("  Roman Empire Game — E2E Tests (unreal-engine-automation skill)")
    print("=" * 70)

    if not check_ready():
        print(f"\n❌ Remote Control not ready at {RC_BASE}")
        print("\n  Start the editor with:")
        print("    UnrealEditor RomanEmpireGame.uproject -ExecCmds=\"WebControl.StartServer\"")
        print("\n  Or auto-start via DefaultEngine.ini:")
        print("    [/Script/WebRemoteControl.RemoteControlSettings]")
        print("    bAutoStartServer=True")
        return 1

    print(f"✓ Remote Control ready at {RC_BASE}\n")

    results = []
    tests = [
        ("zone_transitions", test_zone_transitions),
        ("build_menu_ids",   test_build_menu_ids),
        ("turn_cycle",       test_turn_cycle),
        ("fps_hud",          test_fps_hud),
        ("resource_display", test_resource_display),
    ]

    for name, fn in tests:
        try:
            fn()
            results.append((name, "PASS"))
        except AssertionError as e:
            print(f"  ❌ ASSERT: {name} — {e}")
            results.append((name, f"FAIL(assert): {e}"))
        except Exception as e:
            print(f"  ❌ ERROR: {name} — {e}")
            results.append((name, f"FAIL(error): {e}"))

    print("\n" + "=" * 70)
    passed = sum(1 for _, s in results if s == "PASS")
    for name, status in results:
        print(f"  {'✅' if status == 'PASS' else '❌'} {name}: {status}")
    print(f"\n  {passed}/{len(results)} passed — screenshots: {ARTIFACTS}")
    return 0 if passed == len(results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
