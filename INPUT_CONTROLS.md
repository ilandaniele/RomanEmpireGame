# Input controls

Keyboard/mouse defaults for the Windows single-player prototype (M2.6 plus M3 additive selection).

The controller loads `/Game/Input/DA_RomanInputConfig`. Exactly one gameplay context and `IMC_Shared` are registered for the local player. Strategic/tactical transitions follow the canonical camera zoom band; possession is explicit.

| Control | Strategic | Tactical | Possessed soldier |
|---|---|---|---|
| W / A / S / D | Pan camera | Pan camera | Move soldier |
| Mouse wheel | Zoom | Zoom | Unmapped |
| Mouse motion | Edge scroll when viewport focused | Edge scroll when viewport focused | Look |
| Left mouse | Unmapped | Click/drag selection; confirm valid placement | Attack |
| Left / right Shift | Unmapped | Hold before click to toggle units; before drag to add units | Unmapped |
| Right mouse | Unmapped | Command selected player units; cancel placement | Hold shield; release to stop blocking |
| F | Unmapped | Possess selected living player unit | Return to camera |
| B | Unmapped | Toggle building menu | Unmapped |
| 1–6 | Unmapped | Choose authored building quick slot, only with menu open | Unmapped |
| T / END TURN button | Advance one campaign turn | Advance one campaign turn | Unavailable |
| Escape | Clear selection | Cancel placement, otherwise close menu, otherwise clear selection | Return to camera |

Tactical commands, construction, training, and turns require initialized gameplay and an active campaign. Training uses the selected eligible player-owned barracks through the TRAIN button; the keyboard does not search for an arbitrary producer. Numbered slots use building-definition IDs, not hard-coded building classes. There are no active jump, sprint, or gamepad bindings in this milestone.

Right-clicking terrain with several units selected assigns separate formation destinations around the click. Only selected units move, including when they belong to a larger squad. If the formation cannot fit on the NavMesh, the command is rejected with feedback and previous orders are preserved. A single selected unit uses the exact clicked destination. Routes use collision-aware navigation; an unreachable route leaves that unit holding position.

## Lifecycle and mapping ownership

- Actions live under `/Game/Input/Actions/IA_*`; mode contexts live under `/Game/Input/Contexts/IMC_*`. The config holds 20 distinct actions and four distinct contexts; Tactical has 18 mappings.
- One-shot actions bind Started. Movement/look/zoom bind Triggered. Selection and blocking have completion/cancellation cleanup.
- Mode changes cancel partial selection, placement, open menus, blocking, and pending pawn movement. Releasing F is required before its new enter/exit action can fire. Held digital movement keys must also be released before movement resumes in a new context; this supplements UE 5.8's Boolean-only rebuild suppression. Analog input is not latched.
- Losing viewport focus flushes keys and cancels transient interactions. Unpossession and death clear blocking independently of input dispatch.
- The controller removes only its four owned contexts, never unrelated local-player mappings. Shared cancellation has priority 100; the active gameplay context has priority 0.

Selection samples Shift when the mouse gesture starts. Plain click/drag replaces; Shift-click toggles each eligible unit once and Shift-drag adds without duplicates. Empty additive gestures preserve selection, while plain empty gestures clear it. Buildings remain exclusive selections, including with Shift. Invalid unit/building clicks preserve existing selection and show a three-second, non-interactive warning; a successful selection clears it. Menu, placement, mode changes and focus loss cancel partial gestures. Modifier queries use the evaluated action plus live resolved key mappings, so focus flushing rejects a cached held action immediately without hard-coded Shift keys in gameplay code.

## Editing defaults and future remapping

Change physical keys and modifiers in the IMC assets, not the gameplay controller. Keep action identities, value types, and context membership intact. CameraPan and SoldierMove are separate cumulative Axis2D actions: W swizzles X to Y, S additionally negates Y, A negates X, and D is positive X. Opposite keys cancel.

`URomanInputConfig::ValidateConfiguration` validates references, distinct identities, value types, all six slots, valid keys, and exact logical action membership in defaults and authored profile overrides. Asset Manager scans `/Game/Input` as `RomanInputConfig` with AlwaysCook. These small always-needed assets stay loaded with the controller.

An end-user remapping screen and saved key profiles are not implemented yet. Future work can add Enhanced Input user settings/player-mappable metadata, conflict handling, persistence and dynamic control labels without changing gameplay commands. The current help text documents defaults; update it and the default-mapping assertions when changing those defaults.

## Verification

`BuildProject.ps1 -Action Test` runs input configuration/negative-contract tests, exact authored defaults and modifier evaluation, and `RomanEmpireGame.Input.PhysicalKeysAndModeLifecyclePIE`. The PIE test sends simulated physical-key events into the actual PlayerController and Enhanced PlayerInput stack; it does not broadcast action delegates or move the OS cursor. It covers menu gating, mode isolation, unrelated-context preservation, turns, F/W held across possession, both Shift keys independently and together, RMB release, synchronous focus-flush cleanup, external mode changes and death. `AdditiveSelectionPIE`, `LiveRegistryPIE` and `Buildings.PlayerSelectionPIE` cover selection commands, real rectangle projection, invalid-target UMG feedback and teardown. This is in-process integration coverage, not a complete desktop focus/gamepad acceptance test.
