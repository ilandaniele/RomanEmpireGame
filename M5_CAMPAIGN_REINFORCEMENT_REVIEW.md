# C-015C — Paid campaign reinforcement enrollment

Verified 2026-10-05. C-015C is complete; C-015 and the full implementation plan remain open. No commits, assets, configuration or dependencies changed.

## Contract

Barracks captures a fresh receipt GUID plus encounter/army destination at paid acceptance. Queued promotion preserves that exact receipt and all six original costs. During an active mapped encounter, acceptance requires current Tactical mode, canonical participants at war, content and complete deployment; ambiguous multiple defending armies have no implicit destination policy. Sandbox receipts carry no campaign destination.

CampaignManager privately accepts only the actual producer's active receipt during its guarded production transaction. Unit identity/faction/class, world/lifecycle, canonical encounter and destination are revalidated. TacticalBattleSubsystem prepares a value candidate from the actual live singleton squad; the pure coordinator atomically extends the strategic inventory, deployment snapshot, explicit content binding and consumed-receipt ledger. Both candidates commit and attach casualty tracking before any tactical/army/encounter event. Nested result application, training or cancellation cannot interleave this publication. Arbitrary direct tactical reinforcement registration is rejected in campaign deployments; sandbox behavior remains available.

An enrollment failure destroys the new actor and retires its singleton squad. Existing failed-order processing retains the original receipt if refund credit is blocked, retries credit only and never spawns again. An owner mismatch cannot create a unit for the new owner and refunds the original payer. Terminal outstanding-order settlement, producer destruction and travel preservation remain later work; this is not strategic recruitment UI or a saved session.

## Evidence

- Editor52+6+9+4 plus final factory-guard incremental builds succeeded.
- Final directed10/10 clean, exit0: `Artifacts/Automation/C015C_Targeted_Final`, `Artifacts/C015C-Targeted-Final.log`. Earlier directed9 and corrected reinforcementPIE1 were also clean.
- Pure reinforcement test covers malformed identities, receipt replay, duplicate squad, unplanned army, quantity/content guards, new content, stale inventory, overflow, complete casualty application and terminal rejection.
- Real reinforcement victory/defeat PIE covers two prepaid receipts, captured destination, single credit/promotion, complete observer reads, reentry, physical capture or army retirement, partial staged failure, blocked credit, no respawn, all-six original-cost refund after catalog edit, owner-change original-payer refund and ordinary result replay/reset/lifecycle.
- Full54/54 clean, exit0: `Artifacts/Automation/C015C_Full_Verified`, `Artifacts/C015C-Full-Verified.log`.
- Strict IWYU74 and Shipping14 plus final incremental3 succeeded (`Artifacts/C015C-IWYU.log`, `Artifacts/C015C-Shipping.log`, `Artifacts/C015C-Shipping-Verified.log`). All build/test handles closed.
- Repository validation0 M1 warnings, LFS fsck and diff check pass. No new render/cook/trusted standalone player evidence.

Initial directed failure exposed an incorrect actor Owner comparison: engine `APawn::PossessedBy` assigns Owner to the AI controller. The enrollment gate now verifies the private production transaction and exact active receipt, not Pawn Owner. The first full suite53pass/1fail exposed only a missing expected unavailable-factory diagnostic: the new owner guard ran earlier. Content and owner are now both validated before spawn with the prior factory diagnostic preserved; final directed UI/refund regression passes. Failed initial reports are retained, not presented as approved gates.

Gameplay/data/testing skills guided stable receipt IDs, one authority per domain, all-or-nothing value commits and real public-contract PIE verification. Context updates remain limited to the three user-authorized sections.

## Next — C-015D

Preserve the canonical campaign across real selected tactical-map travel and return. Use a validated immutable transfer snapshot only while travelling, not a second mutable live campaign owner. Restore factions/resources/diplomacy, territories/settlements, armies, turn/outcome and RNG before GameState readiness. Explicitly settle or preserve all prepaid producer receipts before teardown, including terminal orders; no lost reservations. Keep local squad handles transient, rematerialize against the captured plan and apply the canonical result exactly once before return/Continue. Existing prototype defaults and reset tests must remain intentional until a real campaign entry flow is authored. Disk save/load, strategic actions/UI, deterministic Carthage AI and remaining M6/M7 gates follow.
