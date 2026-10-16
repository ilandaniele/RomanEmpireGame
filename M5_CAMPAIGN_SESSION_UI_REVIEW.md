# C-015D3 — Campaign session controls and UI journeys

Date: 2026-10-05. Implements native entry, army inspection/movement, selected-map battle, terminal Continue and quarantined-transfer retry. C-015 and the full plan remain open.

## Commands and ownership

`RomanCampaignCommands.cpp` exposes typed PlayerController commands and read-only eligibility queries. A current, begun, local controller and its exact GameInstance world are required. Strategic actions additionally require an active, ready Strategic session, no pending transfer/encounter, current ownership and actual adjacency. Battle checks the selected territory's authored map and current Rome/Carthage war. Commands call the existing authorities and revalidate at execution; no UI wallet, roster or campaign owner is introduced.

Campaign-session strategic zoom cannot enable sandbox construction/training/orders. Barracks' direct reservation boundary rejects Strategic sessions too. `BeginCampaignEncounter` keeps its pristine-arena compatibility outside sessions but rejects live sessions, which must use actual selected-map travel. Mapped tactical paid reinforcement remains supported.

Continue requires the actual resolved mapped encounter, NOT an active global campaign: a global winner can still apply/consume its result and return without reviving turns. Sandbox Restart remains available only outside sessions. Retry is queried independently of public readiness and is valid only in the actual quarantined destination with the retained capsule.

## Presentation and lifecycle

`RomanCampaignWidget.cpp` extends the existing native root, with `RomanArmyButton` carrying only a stable army GUID. Detached public GameState queries provide all army stacks/counts/positions. Selection is UI-local; actions use current selected army and territory. Army/territory/mode/diplomacy/session notifications refresh presentation without polling or actor enumeration. Public diplomacy relays re-query current authority values when publishing.

The GameInstance session listener binds uniquely even when the incoming GameState is not ready, allowing the recovery panel to remain operable in quarantine. Removal from the viewport removes both world and session subscriptions; reattachment catches up from actual state. Hidden/detached/foreign-world callbacks cannot execute commands. Pending transfer synchronously disables commands. HUD buttons retain mouse-only/no-keyboard-focus behavior. The global outcome overlay allows hit testing its Continue child.

## Evidence

- Directed final UI coverage: 5/5 successful, zero test warnings/errors, exit0; `Artifacts/Automation/C015D3_Targeted_Verified`, `Artifacts/C015D3-Targeted-Verified.log`.
- Rendered D3 journeys: 3/3 successful (one clean, two with warnings), exit0; `Artifacts/Automation/C015D3_Visual`, `Artifacts/C015D3-Visual.log`. Six actual Slate-root PNGs in `Artifacts/C015D3_Visual/`; strategic, tactical victory/defeat and global-win Continue images were inspected. Warnings are the known engine `r.MotionVectorSimulation` render-thread warning and the deliberately attempted inactive-campaign turn; final logical tests explicitly expect the latter. No global log suppression.
- Full regression: 67/67 successful, zero test warnings/errors, exit0 (`Artifacts/Automation/C015D3_Full`, `Artifacts/C015D3-Full.log`). Final Editor49, strict IWYU81 (including final test assertions) and Shipping12 succeeded (`Artifacts/C015D3-FinalTestsBuild.log`, `Artifacts/C015D3-IWYU-Final.log`, `Artifacts/C015D3-Shipping.log`). Repository validation reports zero open M1 warnings; LFS fsck and diff check pass. All processes are terminal at this block's close.

Journeys route pointer events through the actual Slate widget path for entry, stable army selection, owned-adjacent movement out/back, selected Africa battle and Continue. Actual OpenLevel creates replacement worlds. Real deaths or occupation/timer capture resolve outcomes. Tests preserve canonical IDs, city, wallet, turn, casualties/retirement, consumed encounter and complete prepaid-order refunds. Each new world's first readiness sees restored authorities. Diplomatic neutral/war events disable/re-enable attack immediately. Real remove/reattach cycles reject detached entry/Continue and keep one session listener. Global victory returns while remaining inactive.

Two recovery journeys deliberately cause incoming content or map/nonce rejection, preserve the capsule and keep the world not ready. They prove visible recovery controls, detached retry rejection, reattachment, synchronous retry disabling and pointer-routed actual-map retry. Additional final tests prove strategic-session zoom cannot issue sandbox commands or debit training, and direct arena materialization creates no encounter.

Initial build/test mistakes were corrected: inherited `Slot` shadowing; attempting protected `NativeDestruct` instead of actual viewport removal; and hardcoded region UObject names that changed suffixes after rebuild. Tests now find regions by stable public territory ID. None remains an open defect.

## Remaining gates

This is an Editor/PIE and in-process Slate proof, not OS physical input or a trusted packaged-player journey. No new cook, disk save, corruption handling, responsive multi-resolution UI, strategic recruitment/assignment, Carthaginian campaign AI, producer-destruction refund or full milestone closure is claimed. Existing unsigned-player Code Integrity restriction remains; do not weaken security or repeat the identical blocked launch. Continue with canonical recruitment/assignment, AI and M5.4 persistence, then remaining M3/M4/M6/M7 gates.
