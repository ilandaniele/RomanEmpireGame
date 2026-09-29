// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/UnitCombatState.h"

bool FUnitCombatState::TryBeginAttack(float RecoveryDuration, float WindupDuration)
{
	if (!CanAttack() || !FMath::IsFinite(RecoveryDuration) || RecoveryDuration <= 0.0f ||
		!FMath::IsFinite(WindupDuration) || WindupDuration < 0.0f || WindupDuration >= RecoveryDuration)
	{
		return false;
	}
	RecoveryRemaining = RecoveryDuration;
	WindupRemaining = WindupDuration;
	bImpactPending = WindupDuration > 0.0f;
	return true;
}

bool FUnitCombatState::Advance(float DeltaSeconds)
{
	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return false;
	RecoveryRemaining = FMath::Max(0.0f, RecoveryRemaining - DeltaSeconds);
	if (!bImpactPending) return false;
	WindupRemaining = FMath::Max(0.0f, WindupRemaining - DeltaSeconds);
	if (WindupRemaining > 0.0f) return false;
	bImpactPending = false; // Commit before the actor broadcasts damage.
	return true;
}

void FUnitCombatState::CancelPendingImpact()
{
	bImpactPending = false;
	WindupRemaining = 0.0f;
}

void FUnitCombatState::Reset()
{
	RecoveryRemaining = 0.0f;
	CancelPendingImpact();
	bBlocking = false;
}
