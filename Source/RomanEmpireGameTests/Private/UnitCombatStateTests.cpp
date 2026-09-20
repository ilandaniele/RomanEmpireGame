// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Units/UnitCombatState.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUnitCombatStateTest, "RomanEmpireGame.Units.CombatState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FUnitCombatStateTest::RunTest(const FString& Parameters)
{
	FUnitCombatState State;
	TestTrue(TEXT("Default is ready"), State.CanAttack());
	TestFalse(TEXT("Default has no attack in progress"), State.IsAttackInProgress());
	TestFalse(TEXT("Default is not blocking"), State.IsBlocking());
	TestEqual(TEXT("Default recovery is zero"), State.GetRecoveryRemaining(), 0.0f);

	const float InvalidValues[] = {
		0.0f, -0.0f, -1.0f, -MAX_flt,
		std::numeric_limits<float>::quiet_NaN(),
		std::numeric_limits<float>::infinity(),
		-std::numeric_limits<float>::infinity()
	};
	State.SetBlocking(true);
	State.SetBlocking(true);
	for (const float InvalidValue : InvalidValues)
	{
		TestFalse(TEXT("Invalid duration cannot start an attack"), State.TryBeginAttack(InvalidValue));
		State.Advance(InvalidValue);
		TestEqual(TEXT("Invalid inputs preserve ready recovery"), State.GetRecoveryRemaining(), 0.0f);
		TestTrue(TEXT("Invalid inputs preserve blocking"), State.IsBlocking());
		TestTrue(TEXT("Invalid inputs preserve readiness"), State.CanAttack());
	}

	TestTrue(TEXT("Blocking remains orthogonal to starting an attack"), State.TryBeginAttack(2.0f));
	TestTrue(TEXT("Attack does not clear blocking"), State.IsBlocking());
	TestEqual(TEXT("Caller supplied duration is stored without another rate calculation"), State.GetRecoveryRemaining(), 2.0f);
	TestFalse(TEXT("Recovery prevents another attack"), State.CanAttack());
	TestTrue(TEXT("Attack in progress is derived from recovery"), State.IsAttackInProgress());
	for (const float InvalidValue : InvalidValues)
	{
		TestFalse(TEXT("Invalid duration during recovery is rejected"), State.TryBeginAttack(InvalidValue));
		State.Advance(InvalidValue);
		TestEqual(TEXT("Invalid inputs cannot alter active recovery"), State.GetRecoveryRemaining(), 2.0f);
		TestTrue(TEXT("Invalid inputs cannot alter active blocking"), State.IsBlocking());
	}
	TestFalse(TEXT("Valid duration cannot replace an active attack"), State.TryBeginAttack(10.0f));
	TestEqual(TEXT("Rejected overlapping attack is atomic"), State.GetRecoveryRemaining(), 2.0f);
	State.Advance(0.5f);
	TestEqual(TEXT("One advance consumes its delta exactly once"), State.GetRecoveryRemaining(), 1.5f);
	State.SetBlocking(false);
	State.SetBlocking(false);
	TestFalse(TEXT("Clearing blocking is idempotent"), State.IsBlocking());
	TestEqual(TEXT("Blocking change does not alter recovery"), State.GetRecoveryRemaining(), 1.5f);
	State.Advance(1.5f);
	TestEqual(TEXT("Exact boundary completes recovery"), State.GetRecoveryRemaining(), 0.0f);
	TestTrue(TEXT("Exact boundary allows another attack"), State.CanAttack());
	TestFalse(TEXT("Exact boundary ends attack in progress"), State.IsAttackInProgress());
	State.Advance(MAX_flt);
	State.Advance(1.0f);
	TestEqual(TEXT("Advancing ready state never retriggers an attack"), State.GetRecoveryRemaining(), 0.0f);
	TestTrue(TEXT("Ready state remains ready after excess elapsed time"), State.CanAttack());

	FUnitCombatState OneFrame;
	FUnitCombatState Partitioned;
	OneFrame.TryBeginAttack(2.0f);
	Partitioned.TryBeginAttack(2.0f);
	OneFrame.Advance(0.875f);
	Partitioned.Advance(0.5f);
	Partitioned.Advance(0.25f);
	Partitioned.Advance(0.125f);
	TestEqual(TEXT("Frame partitions consume equal elapsed time"), Partitioned.GetRecoveryRemaining(), OneFrame.GetRecoveryRemaining());
	OneFrame.Advance(1.125f);
	Partitioned.Advance(0.125f);
	Partitioned.Advance(1.0f);
	TestTrue(TEXT("Single frame reaches ready"), OneFrame.CanAttack());
	TestTrue(TEXT("Partitioned frames reach ready at same boundary"), Partitioned.CanAttack());
	TestTrue(TEXT("New attack can begin after recovery"), State.TryBeginAttack(0.25f));
	State.Advance(1.0f);
	TestEqual(TEXT("Overshoot clamps at zero instead of retaining time credit"), State.GetRecoveryRemaining(), 0.0f);
	TestTrue(TEXT("Overshoot does not discount the next attack"), State.TryBeginAttack(3.0f));
	TestEqual(TEXT("Next attack receives full requested recovery"), State.GetRecoveryRemaining(), 3.0f);

	State.SetBlocking(true);
	State.Reset();
	State.Reset();
	TestTrue(TEXT("Reset idempotently restores readiness"), State.CanAttack());
	TestFalse(TEXT("Reset cancels attack in progress"), State.IsAttackInProgress());
	TestFalse(TEXT("Reset clears blocking"), State.IsBlocking());
	TestEqual(TEXT("Reset clears remaining recovery"), State.GetRecoveryRemaining(), 0.0f);
	TestTrue(TEXT("Largest finite duration is accepted"), State.TryBeginAttack(MAX_flt));
	State.Advance(MAX_flt * 0.5f);
	TestEqual(TEXT("Large finite advance stays finite"), State.GetRecoveryRemaining(), MAX_flt * 0.5f);
	State.Advance(MAX_flt);
	TestTrue(TEXT("Large finite overshoot returns to ready"), State.CanAttack());
	TestEqual(TEXT("Large finite overshoot remains bounded"), State.GetRecoveryRemaining(), 0.0f);
	TestTrue(TEXT("Smallest normalized positive duration is accepted"), State.TryBeginAttack(std::numeric_limits<float>::min()));
	State.Advance(std::numeric_limits<float>::min());
	TestTrue(TEXT("Small positive exact boundary completes"), State.CanAttack());
	return true;
}
#endif
