// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UnitOrderState.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EUnitOrderType : uint8
{
	None,
	Move,
	Attack,
	Hold
};

/** Single owner of an actor's active order; no movement, scans, combat or callbacks.
 * The actor coordinator validates command eligibility, target faction, world and self.
 * Invalid requests return false without changing any state. Accepted no-ops return true.
 * Targets are weak: queries never expose a destroyed target, and ClearInvalidTarget
 * explicitly retires its stale Attack order. A paused pursuit remains an Attack order.
 */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FUnitOrderState
{
	GENERATED_BODY()

	/** Replaces any previous order and target with a finite destination. */
	bool IssueMove(const FVector& NewDestination);
	/** Replaces any previous order and begins pursuing a valid target. */
	bool IssueAttack(AActor* NewTarget, const FVector& InitialDestination);
	/** Starts/resumes pursuit only for an Attack with a still-valid target. */
	bool UpdatePursuitDestination(const FVector& NewDestination);
	/** Completes Move to None; pauses Attack pursuit while retaining its target. */
	void StopMovement();
	/** Cancels motion/target and persists Hold until explicitly replaced or cleared. */
	void Hold();
	/** Resets order, destination, target and pursuit; safe to repeat during teardown. */
	void Clear();
	/** Clears an Attack whose weak target expired/is being destroyed, once. */
	bool ClearInvalidTarget();

	EUnitOrderType GetOrderType() const { return OrderType; }
	/** Changes only on replacement/cancellation, not moving-target position updates. */
	uint64 GetRevision() const { return Revision; }
	/** Last requested destination; zero for None/Hold. */
	FVector GetDestination() const { return Destination; }
	AActor* GetTarget() const;
	/** True for Move or a pursuing Attack whose target is still valid. */
	bool HasMoveCommand() const;

private:
	uint64 Revision = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit|Orders", meta=(AllowPrivateAccess="true"))
	EUnitOrderType OrderType = EUnitOrderType::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit|Orders", meta=(AllowPrivateAccess="true"))
	FVector Destination = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit|Orders", meta=(AllowPrivateAccess="true"))
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Unit|Orders", meta=(AllowPrivateAccess="true"))
	bool bPursuing = false;
};
