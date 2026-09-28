// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanUnitAIController.h"
#include "Units/UnitBase.h"
#include "Navigation/PathFollowingComponent.h"

void ARomanUnitAIController::CancelOrderNavigation()
{
	// Invalidate before AbortMove can synchronously publish completion callbacks.
	ActiveRequest = FAIRequestID::InvalidRequest;
	ActiveUnit.Reset();
	StopMovement();
}

void ARomanUnitAIController::OnUnPossess()
{
	CancelOrderNavigation();
	Super::OnUnPossess();
}

void ARomanUnitAIController::SynchronizeOrder(AUnitBase* Unit)
{
	if (!IsValid(Unit) || GetPawn() != Unit || !Unit->CanAcceptCommands() ||
		Unit->IsPossessedByPlayer() || !Unit->Orders.HasMoveCommand())
	{
		CancelOrderNavigation();
		return;
	}
	const uint64 Revision = Unit->Orders.GetRevision();
	if (ActiveRequest.IsValid() && ActiveUnit == Unit && ActiveRevision == Revision) return;
	CancelOrderNavigation();
	if (!IsValid(Unit) || GetPawn() != Unit || !Unit->CanAcceptCommands() || Unit->IsPossessedByPlayer() ||
		Unit->Orders.GetRevision() != Revision || !Unit->Orders.HasMoveCommand()) return;

	FAIMoveRequest Request;
	const bool bAttack = Unit->Orders.GetOrderType() == EUnitOrderType::Attack;
	if (bAttack) Request.SetGoalActor(Unit->Orders.GetTarget());
	else Request.SetGoalLocation(Unit->Orders.GetDestination());
	Request.SetUsePathfinding(true);
	Request.SetAllowPartialPath(false);
	Request.SetProjectGoalLocation(true);
	Request.SetRequireNavigableEndLocation(true);
	Request.SetReachTestIncludesAgentRadius(false);
	Request.SetReachTestIncludesGoalRadius(false);
	Request.SetAcceptanceRadius(bAttack ? FMath::Max(10.0f, Unit->GetUnitData().AttackRange * 0.8f) : 40.0f);
	const FPathFollowingRequestResult Result = MoveTo(Request);
	// MoveTo may finish/abort immediately and external engine observers can replace intent.
	if (!IsValid(Unit) || GetPawn() != Unit || Unit->Orders.GetRevision() != Revision ||
		!Unit->CanAcceptCommands() || Unit->IsPossessedByPlayer())
	{
		// Cancel only this stale submission, never a newer observer-owned request.
		if (Result.Code == EPathFollowingRequestResult::RequestSuccessful && GetPathFollowingComponent())
			GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, Result.MoveId);
		return;
	}
	if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		ActiveUnit = Unit;
		ActiveRevision = Revision;
		ActiveRequest = Result.MoveId;
		Unit->NavigationResult = EUnitNavigationResult::Moving;
	}
	else
	{
		Unit->CompleteNavigation(Revision, Result.Code == EPathFollowingRequestResult::AlreadyAtGoal);
	}
}

void ARomanUnitAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (RequestID == ActiveRequest && ActiveRequest.IsValid())
	{
		AUnitBase* Unit = ActiveUnit.Get();
		const uint64 Revision = ActiveRevision;
		ActiveRequest = FAIRequestID::InvalidRequest;
		ActiveUnit.Reset();
		if (Unit && GetPawn() == Unit) Unit->CompleteNavigation(Revision, Result.IsSuccess());
	}
	// Observers see committed completion, and may safely issue a replacement order.
	Super::OnMoveCompleted(RequestID, Result);
}
