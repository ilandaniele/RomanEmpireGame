// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CampaignObjectiveState.generated.h"

UENUM(BlueprintType)
enum class EVictoryCondition : uint8
{
	Conquest UMETA(DisplayName = "Conquest"),
	Economic UMETA(DisplayName = "Economic"),
	Military UMETA(DisplayName = "Military"),
	Diplomatic UMETA(DisplayName = "Diplomatic")
};

UENUM(BlueprintType)
enum class ECampaignOutcome : uint8
{
	InProgress UMETA(DisplayName = "In Progress"),
	Victory UMETA(DisplayName = "Victory"),
	Defeat UMETA(DisplayName = "Defeat"),
	Draw UMETA(DisplayName = "Draw")
};

/** Read-only objective projection derived from canonical campaign authorities. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignObjectiveState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Objectives")
	int32 ControlledTerritories = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Objectives")
	int32 RequiredTerritories = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Objectives")
	int32 CurrentGold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Objectives")
	int32 RequiredGold = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Objectives")
	ECampaignOutcome Outcome = ECampaignOutcome::InProgress;

	float GetConquestProgress() const
	{
		return RequiredTerritories > 0
			? FMath::Clamp(static_cast<float>(ControlledTerritories) / RequiredTerritories, 0.0f, 1.0f)
			: 0.0f;
	}

	float GetEconomicProgress() const
	{
		return RequiredGold > 0
			? FMath::Clamp(static_cast<float>(CurrentGold) / RequiredGold, 0.0f, 1.0f)
			: 0.0f;
	}

	bool IsResolved() const
	{
		return Outcome != ECampaignOutcome::InProgress;
	}
};
