// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/UnitOrderState.h"
#include "GameFramework/Actor.h"

namespace
{
	bool IsFiniteDestination(const FVector& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
	}

	bool IsUsableTarget(const AActor* Actor)
	{
		return IsValid(Actor) && !Actor->IsActorBeingDestroyed();
	}
}

bool FUnitOrderState::IssueMove(const FVector& NewDestination)
{
	if (!IsFiniteDestination(NewDestination)) return false;
	OrderType = EUnitOrderType::Move;
	Destination = NewDestination;
	Target.Reset();
	bPursuing = false;
	return true;
}

bool FUnitOrderState::IssueAttack(AActor* NewTarget, const FVector& InitialDestination)
{
	if (!IsUsableTarget(NewTarget) || !IsFiniteDestination(InitialDestination)) return false;
	OrderType = EUnitOrderType::Attack;
	Destination = InitialDestination;
	Target = NewTarget;
	bPursuing = true;
	return true;
}

bool FUnitOrderState::UpdatePursuitDestination(const FVector& NewDestination)
{
	if (OrderType != EUnitOrderType::Attack || !GetTarget() || !IsFiniteDestination(NewDestination)) return false;
	Destination = NewDestination;
	bPursuing = true;
	return true;
}

void FUnitOrderState::StopMovement()
{
	if (OrderType == EUnitOrderType::Move)
	{
		Clear();
	}
	else if (OrderType == EUnitOrderType::Attack)
	{
		bPursuing = false;
	}
}

void FUnitOrderState::Hold()
{
	Clear();
	OrderType = EUnitOrderType::Hold;
}

void FUnitOrderState::Clear()
{
	OrderType = EUnitOrderType::None;
	Destination = FVector::ZeroVector;
	Target.Reset();
	bPursuing = false;
}

bool FUnitOrderState::ClearInvalidTarget()
{
	if (OrderType != EUnitOrderType::Attack || GetTarget()) return false;
	Clear();
	return true;
}

AActor* FUnitOrderState::GetTarget() const
{
	AActor* Actor = Target.Get();
	return OrderType == EUnitOrderType::Attack && IsUsableTarget(Actor) ? Actor : nullptr;
}

bool FUnitOrderState::HasMoveCommand() const
{
	return OrderType == EUnitOrderType::Move ||
		(OrderType == EUnitOrderType::Attack && bPursuing && GetTarget());
}
