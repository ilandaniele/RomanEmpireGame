// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitCombatState.generated.h"

/** Sole owner of attack recovery, pending impact and blocking. No world, stamina or damage effects.
 * The coordinator derives durations once and advances this state once per tick.
 * An impact is consumed exactly once when windup expires; cancellation does not refund recovery.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FUnitCombatState
{
	GENERATED_BODY()

	bool TryBeginAttack(float RecoveryDuration, float WindupDuration = 0.0f);
	bool CanAttack() const { return RecoveryRemaining == 0.0f; }
	void SetBlocking(bool bNewBlocking) { bBlocking = bNewBlocking; }
	bool IsBlocking() const { return bBlocking; }
	bool IsAttackInProgress() const { return RecoveryRemaining > 0.0f; }
	bool HasPendingImpact() const { return bImpactPending; }
	float GetWindupRemaining() const { return WindupRemaining; }
	float GetRecoveryRemaining() const { return RecoveryRemaining; }
	/** Returns true only on the one advance that consumes a pending impact. */
	bool Advance(float DeltaSeconds);
	void CancelPendingImpact();
	void Reset();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	float RecoveryRemaining = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	float WindupRemaining = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	bool bImpactPending = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	bool bBlocking = false;
};
