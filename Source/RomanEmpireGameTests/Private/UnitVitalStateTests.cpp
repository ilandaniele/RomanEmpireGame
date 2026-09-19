// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Units/UnitVitalState.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitVitalStateTest, "RomanEmpireGame.Units.VitalState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnitVitalStateTest::RunTest(const FString& Parameters)
{
	FUnitVitalState State;
	const float NaN = std::numeric_limits<float>::quiet_NaN();
	const float Infinity = std::numeric_limits<float>::infinity();
	TestEqual(TEXT("Default is explicitly uninitialized"), State.GetLifeState(), EUnitLifeState::Uninitialized);
	TestEqual(TEXT("No authored health before initialization"), State.GetHealthPercent(), 0.0f);
	TestFalse(TEXT("No damage before initialization"), State.ApplyDamage(1, 1));
	TestFalse(TEXT("No healing before initialization"), State.SetHealth(100));
	TestFalse(TEXT("Reject zero health definition"), State.Initialize(0, 100.0f, 50));
	TestFalse(TEXT("Reject zero stamina definition"), State.Initialize(100, 0.0f, 50));
	TestFalse(TEXT("Reject negative morale definition"), State.Initialize(100, 100.0f, -1));
	TestFalse(TEXT("Reject NaN definition"), State.Initialize(100, NaN, 50));
	TestFalse(TEXT("Reject infinite definition"), State.Initialize(100, Infinity, 50));
	TestEqual(TEXT("Rejected initialization is atomic"), State.GetMaxHealth(), 0);
	TestTrue(TEXT("Corrected definition can initialize once"), State.Initialize(100, 80.0f, 50));
	TestFalse(TEXT("Repeated initialization rejected"), State.Initialize(900, 900.0f, 900));
	TestEqual(TEXT("Initial health preserved"), State.GetHealth(), 100);
	TestEqual(TEXT("Initial stamina preserved"), State.GetStamina(), 80.0f);
	TestEqual(TEXT("Initial morale preserved"), State.GetMorale(), 50);
	TestFalse(TEXT("Negative health rejected"), State.SetHealth(-1));
	TestFalse(TEXT("Negative damage rejected"), State.ApplyDamage(-1, 0));
	TestFalse(TEXT("Negative morale damage rejects whole transaction"), State.ApplyDamage(10, -1));
	TestEqual(TEXT("Rejected damage preserves health"), State.GetHealth(), 100);
	TestTrue(TEXT("Damage applies atomically"), State.ApplyDamage(25, 5));
	TestEqual(TEXT("Damaged health"), State.GetHealth(), 75);
	TestEqual(TEXT("Damaged morale"), State.GetMorale(), 45);
	TestTrue(TEXT("Heal above maximum clamps"), State.SetHealth(MAX_int32));
	TestEqual(TEXT("Health bounded"), State.GetHealth(), 100);
	TestFalse(TEXT("Negative spend rejected"), State.TrySpendStamina(-1.0f));
	TestFalse(TEXT("NaN spend rejected"), State.TrySpendStamina(NaN));
	TestFalse(TEXT("Infinite spend rejected"), State.TrySpendStamina(Infinity));
	TestFalse(TEXT("Overspend rejected"), State.TrySpendStamina(81.0f));
	TestTrue(TEXT("Exact spend accepted"), State.TrySpendStamina(80.0f));
	TestEqual(TEXT("Stamina reaches zero"), State.GetStaminaPercent(), 0.0f);
	TestFalse(TEXT("NaN regeneration rejected"), State.RegenerateStamina(NaN));
	TestFalse(TEXT("Infinite regeneration rejected"), State.RegenerateStamina(Infinity));
	TestFalse(TEXT("Negative regeneration rejected"), State.RegenerateStamina(-1.0f));
	TestTrue(TEXT("Large finite regeneration saturates without overflow"), State.RegenerateStamina(MAX_flt));
	TestEqual(TEXT("Regeneration cap"), State.GetStamina(), 80.0f);
	TestTrue(TEXT("Overkill safely commits death"), State.ApplyDamage(MAX_int32, MAX_int32));
	TestEqual(TEXT("Death state"), State.GetLifeState(), EUnitLifeState::Dead);
	TestEqual(TEXT("Zero health"), State.GetHealth(), 0);
	TestEqual(TEXT("Zero morale"), State.GetMorale(), 0);
	TestFalse(TEXT("Dead cannot heal"), State.SetHealth(100));
	TestFalse(TEXT("Dead cannot initialize"), State.Initialize(100, 100.0f, 100));
	TestFalse(TEXT("Dead cannot take damage twice"), State.ApplyDamage(1, 1));
	TestFalse(TEXT("Dead cannot spend"), State.TrySpendStamina(1.0f));
	TestFalse(TEXT("Dead cannot regenerate"), State.RegenerateStamina(1.0f));
	State.EndLife();
	State.EndLife();
	TestEqual(TEXT("Teardown idempotent"), State.GetLifeState(), EUnitLifeState::Ended);
	TestFalse(TEXT("Ended cannot initialize"), State.Initialize(100, 100.0f, 100));
	FUnitVitalState Failed;
	Failed.MarkInitializationFailed();
	TestEqual(TEXT("Explicit failure state"), Failed.GetLifeState(), EUnitLifeState::InitializationFailed);
	TestFalse(TEXT("Failed cannot initialize"), Failed.Initialize(1, 1.0f, 0));
	Failed.EndLife();
	TestEqual(TEXT("Failed can tear down"), Failed.GetLifeState(), EUnitLifeState::Ended);
	FUnitVitalState Maximum;
	TestTrue(TEXT("Finite maximum values accepted"), Maximum.Initialize(MAX_int32, MAX_flt, MAX_int32));
	Maximum.MarkInitializationFailed();
	TestTrue(TEXT("Failure cannot overwrite valid initialization"), Maximum.IsAlive());
	Maximum.TrySpendStamina(1.0f);
	Maximum.RegenerateStamina(MAX_flt);
	TestTrue(TEXT("Maximum stamina remains finite"), FMath::IsFinite(Maximum.GetStamina()));
	Maximum.TrySpendStamina(MAX_flt);
	Maximum.RegenerateStamina(3.0f * FMath::Pow(2.0f, 103.0f));
	Maximum.RegenerateStamina(MAX_flt);
	TestEqual(TEXT("Double-rounding boundary cannot overflow stamina"), Maximum.GetStamina(), MAX_flt);
	TestTrue(TEXT("Zero-health setter commits death"), Maximum.SetHealth(0));
	TestFalse(TEXT("Setter death cannot resurrect"), Maximum.SetHealth(1));
	return true;
}
#endif
