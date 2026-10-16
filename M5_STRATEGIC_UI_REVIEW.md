# M5.1 — Strategic region inspection (C-005)

Date: 2026-10-04 (America/Montevideo). HEAD remains `0726de2`; existing H-001/H-002 and C-001–C-004 work is preserved, uncommitted. This block implements read-only region UI, not army actions or the entire campaign milestone.

## Implementation / decisions

The existing controller-owned native `URomanEmpireMainWidget` now contains a Strategic-only panel with nine cards in canonical lexical-ID order and a region inspector. This is an abstract region grid, **not geographic cartography**. No image/model/Widget Blueprint or binary asset is needed.

Cards show the authored region name, current catalog owner and compact active/dormant/unowned state, with a selected marker and full tooltip. `URomanTerritoryButton` inherits the existing mouse HUD button's style and no-keyboard-focus policy. It stores only a stable territory ID and emits a typed click; it holds no owner/wallet/definition state. The inspector re-queries the C-004 GameState snapshot for name/ID, owner participation, terrain, sorted neighbor IDs, all six **base** production fields and description. It explicitly says this is not effective settlement yield. Dormant Gaul and unowned regions remain inspectable.

`SelectTerritory(ID)` is an attached/ready/Strategic-only local inspection facade, not a gameplay command. Unknown IDs clear the previous inspection. Leaving Strategic or NativeDestruct clears that local key; no campaign or tactical selection is mutated. No resource cost, production credit, army or turn change occurs. A future action must add its own canonical command/eligibility contract, not reinterpret this UI selection as campaign authority.

The widget binds once to public GameState ownership notifications, re-queries on callbacks (including nested notifications), and unbinds on NativeDestruct. Construct/readiness/mode/player refresh use the existing lifecycle. Reattachment rebuilds presentation from current public values rather than replaying missed events. Buttons are rebuilt only if the ordered IDs change; generated UObject names are not identities. No UI Tick, world actor scans, gameplay mirrors, module/dependency/config or input asset change.

## Verification

- Editor initial 34/34 and final test-lookup refinement 4/4 pass: `Artifacts/C005-Editor.log`, `Artifacts/C005-Editor-Final.log`.
- Initial directed UI/PublicGameState: **2/2, zero test warnings/errors**; `Artifacts/Automation/C005_Targeted`, `Artifacts/C005-Targeted.log`.
- Initial strict non-Unity/no-PCH build: **68/68**, `Artifacts/C005-IWYU.log`.
- Initial Shipping: **5/5**, `Artifacts/C005-Shipping.log`.
- Initial D3D12/offscreen real-PIE UI: **1/1 successful**, with one engine warning concerning render-thread `r.MotionVectorSimulation`; `Artifacts/Automation/C005_Visual`, `Artifacts/C005-Visual.log`. No test errors. Startup `Condition failed` messages are outside the test report, as in the baseline.
- Visual inspection caught card text wrapping past the grid slots. The final layout increases panel/cell size, uses compact status labels, disables card auto-wrap, ellipsizes overflow and preserves full tooltips. Final strict IWYU incremental **7/7** (`Artifacts/C005-IWYU-Final.log`) and Shipping **3/3** (`Artifacts/C005-Shipping-Final.log`) pass.
- Final D3D12/offscreen UI: **1/1 successful**, zero test errors, the same single engine `r.MotionVectorSimulation` warning. `Artifacts/Automation/C005_Visual_Final`, `Artifacts/C005-Visual-Final.log`; exit 0. Both final real Slate captures were inspected at **1024×555**: all nine card labels/owner/status and Gaul/Rome details fit without spilling between cards. Not a multi-resolution or packaged-player gate.
- Full NullRHI regression: **37/37, zero test warnings/errors**, `Artifacts/Automation/C005_Full`, `Artifacts/C005-Full.log`; exit 0. Repository validation/LFS/whitespace pass. All C-005 processes finished; no pending handles.

The strengthened `CanonicalEventDrivenPIE` checks all nine actual grid controls, stable reflected IDs, owner/status metadata, preserved keyboard focus, Slate pointer-routed selection of Gaul/Rome, six-field/neighbor/terrain inspection, no wallet/turn mutation, unknown/stale IDs and Tactical/Possessed visibility guards. An earlier public ownership observer restores Rome inside an outer Rome→Carthage notification, and UMG must remain on the final Rome state. Real detach/reattach changes Gaul's owner while the root is unsubscribed, verifies frozen detached text and rejected input, then catches up from current values and restores authored ownership. Subscription counts and ended-state rejection are also checked.

Capture opt-in is Editor-test-only: run the normal UI test with D3D12/`-RenderOffscreen -RomanCaptureStrategicUI` (without `-NullRHI`). It writes real Slate PNGs to `Artifacts/C005_Visual/{DormantGaul,ActiveRome}.png`. Default regression does not capture. Screenshots are engine-rendered evidence, not generated art or a packaged-player/physical-input claim.

Skills: `ue-ui-umg-slate` guided event-driven UMG lifetime, reflected references and reusable mouse buttons; `unreal-cpp-gameplay` guided native reflected types; `ue-testing-debugging` kept tests on public/reflected contracts and real Slate routing. Context updates are confined to authorized framework/verification sections. No MCP session, new saved asset, security change, commit, staging or push.

## Remaining / next C-006

M5.1 still needs live settlement state to leave presentation actors. Next C-006 should move the existing settlement sub-contract (presence/name/population/production bonus) into the canonical territory state/WorldMapManager, preserving current behavior through actor wrappers, then expose live settlement/effective production through GameState events/queries and UMG. Do not label `StartingSettlementName` as live state or store an army/save schema in this block. Inspect only `TerritoryRegion` settlement/resource methods, territory state/WorldMapManager initialization and campaign production.

Directed finding for that next task: `ATerritoryRegion` currently stores `bHasSettlement`, `SettlementName`, `Population`, `SettlementProductionBonus`; first founding starts population at 100 and adds Gold 100/Food 50, duplicate founding returns with a warning. `GetResourceProduction()` adds the six bonus fields to base definition production. Preserve the scenario bootstrap and wallet authority; define fail-closed/atomic arithmetic behavior and typed transactions before replacing these fields. No new player founding cost or economy rule is authorized by merely moving that authority.

Army/recruitment/movement/strategic AI, battle travel/result application, save/load and M6 presentation/accessibility remain later gates. No fresh cook/package/trusted-player or multi-resolution/localization polish claim. Four unanswered design policies remain unchanged and do not block the settlement authority work.
