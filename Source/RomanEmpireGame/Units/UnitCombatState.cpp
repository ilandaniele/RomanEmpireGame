// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/UnitCombatState.h"

bool FUnitCombatState::TryBeginAttack(float RecoveryDuration)
{
	if (!CanAttack() || !FMath::IsFinite(RecoveryDuration) || RecoveryDuration <= 0.0f)
	{
		return false;
	}
	RecoveryRemaining = RecoveryDuration;
	return true;
}

void FUnitCombatState::Advance(float DeltaSeconds)
{
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
	RecoveryRemaining = FMath::Max(0.0f, RecoveryRemaining - DeltaSeconds);
}

void FUnitCombatState::Reset()
{
	RecoveryRemaining = 0.0f;
	bBlocking = false;
}
