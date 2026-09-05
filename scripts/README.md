# Retired Remote Control Harness

The legacy Python/pytest Remote Control harness was removed during M1 recovery because it depended on missing maps and object paths, treated screenshots and printed expectations as assertions, started a privileged editor HTTP service by default, and did not reliably own the Unreal process it attempted to terminate.

Its history remains recoverable from Git. Do not restore it as a default launch path.

Replacement test layers are:

- `Source/RomanEmpireGameTests` for native rules and state automation.
- `L_Testbed` for deterministic PIE/world validation, followed by dedicated Functional Test actors as gameplay scenarios are implemented.
- An Editor-only Automation Driver module only if UMG behavior needs it later.
- `BuildProject.ps1 -Action Test` for native automation execution.
- UAT cook/package plus an explicit packaged executable smoke check.
