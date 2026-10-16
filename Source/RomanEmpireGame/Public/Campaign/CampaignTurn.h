// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CampaignTurn.generated.h"

/** Ordered stages in the single campaign turn pipeline. */
UENUM(BlueprintType)
enum class ECampaignTurnStep : uint8
{
	ValidateRequest		UMETA(DisplayName = "Validate Request"),
	Production			UMETA(DisplayName = "Production"),
	Upkeep				UMETA(DisplayName = "Upkeep"),
	AIActions			UMETA(DisplayName = "AI Actions"),
	ConflictResolution	UMETA(DisplayName = "Conflict Resolution"),
	VictoryEvaluation	UMETA(DisplayName = "Victory Evaluation"),
	IncrementTurn		UMETA(DisplayName = "Increment Turn"),
	BroadcastTurnCompleted UMETA(DisplayName = "Broadcast Turn Completed"),
	Autosave			UMETA(DisplayName = "Autosave"),
	/** Post-pipeline dispatch, not one of the nine turn stages. */
	AITacticalTravel UMETA(DisplayName = "AI Tactical Travel")
};

/** Typed failure; the audit result records progress, never an implicit rollback.
 * AI travel can fail after a committed turn and be retried without running stages. */
UENUM(BlueprintType)
enum class ECampaignTurnFailure : uint8
{
	None					UMETA(DisplayName = "None"),
	CampaignInactive		UMETA(DisplayName = "Campaign Inactive"),
	MissingCampaignManager	UMETA(DisplayName = "Missing Campaign Manager"),
	MissingFactionManager	UMETA(DisplayName = "Missing Faction Manager"),
	MissingWorldMapManager	UMETA(DisplayName = "Missing World Map Manager"),
	ResourceTransactionFailed UMETA(DisplayName = "Resource Transaction Failed"),
	AutosaveFailed			UMETA(DisplayName = "Autosave Failed"),
	TurnAlreadyProcessing	UMETA(DisplayName = "Turn Already Processing"),
	EncounterResolved		UMETA(DisplayName = "Encounter Resolved"),
	EncounterPending		UMETA(DisplayName = "Encounter Pending"),
	AITacticalTravelFailed UMETA(DisplayName = "AI Tactical Travel Failed")
};

/** Audit result for one attempted campaign turn. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignTurnResult
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	ECampaignTurnFailure Failure = ECampaignTurnFailure::CampaignInactive;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	ECampaignTurnStep FailedStep = ECampaignTurnStep::ValidateRequest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	int32 StartingTurn = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	int32 CompletedTurn = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	TArray<ECampaignTurnStep> CompletedSteps;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Turn")
	bool bWasAITacticalTravelRetry = false;

	bool WasSuccessful() const { return Failure == ECampaignTurnFailure::None; }
};

namespace RomanEmpireCampaign
{
	ROMANEMPIREGAME_API const TArray<ECampaignTurnStep>& GetCanonicalTurnPipeline();
	ROMANEMPIREGAME_API const TCHAR* GetTurnStepName(ECampaignTurnStep Step);
	ROMANEMPIREGAME_API const TCHAR* GetTurnFailureName(ECampaignTurnFailure Failure);
}
