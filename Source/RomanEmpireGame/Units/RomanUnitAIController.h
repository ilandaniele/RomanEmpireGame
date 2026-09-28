// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "AIController.h"
#include "AITypes.h"
#include "RomanUnitAIController.generated.h"

class AUnitBase;

/** Executes the pawn's canonical order through engine path following. No independent orders. */
UCLASS()
class ARomanUnitAIController : public AAIController
{
	GENERATED_BODY()
public:
	void SynchronizeOrder(AUnitBase* Unit);
	void CancelOrderNavigation();
protected:
	virtual void OnUnPossess() override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
private:
	FAIRequestID ActiveRequest = FAIRequestID::InvalidRequest;
	uint64 ActiveRevision = 0;
	TWeakObjectPtr<AUnitBase> ActiveUnit;
};
