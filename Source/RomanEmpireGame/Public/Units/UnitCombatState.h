// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitCombatState.generated.h"

/** Sole owner of attack recovery and blocking. No world, stamina or damage effects.
 * The coordinator derives a recovery duration once and advances this state once per tick.
 * Recovery is not an authored windup/impact window; blocking remains orthogonal to it.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FUnitCombatState
{
	GENERATED_BODY()

	bool TryBeginAttack(float RecoveryDuration);
	bool CanAttack() const { return RecoveryRemaining == 0.0f; }
	void SetBlocking(bool bNewBlocking) { bBlocking = bNewBlocking; }
	bool IsBlocking() const { return bBlocking; }
	bool IsAttackInProgress() const { return RecoveryRemaining > 0.0f; }
	float GetRecoveryRemaining() const { return RecoveryRemaining; }
	void Advance(float DeltaSeconds);
	void Reset();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	float RecoveryRemaining = 0.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit", meta=(AllowPrivateAccess="true"))
	bool bBlocking = false;
};
