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
	Autosave			UMETA(DisplayName = "Autosave")
};

/** Typed failure returned without advancing the canonical turn counter. */
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
	TurnAlreadyProcessing	UMETA(DisplayName = "Turn Already Processing")
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

	bool WasSuccessful() const { return Failure == ECampaignTurnFailure::None; }
};

namespace RomanEmpireCampaign
{
	ROMANEMPIREGAME_API const TArray<ECampaignTurnStep>& GetCanonicalTurnPipeline();
	ROMANEMPIREGAME_API const TCHAR* GetTurnStepName(ECampaignTurnStep Step);
	ROMANEMPIREGAME_API const TCHAR* GetTurnFailureName(ECampaignTurnFailure Failure);
}
