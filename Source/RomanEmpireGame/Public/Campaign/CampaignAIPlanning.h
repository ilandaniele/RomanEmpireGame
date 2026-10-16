// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "CampaignAIPlanning.generated.h"

UENUM(BlueprintType)
enum class ECampaignAIAction : uint8 { None, Defend, Recruit, Attack };

/** Derived proposal, not a reservation, another owner, or permission to mutate. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignAIProposal
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	ECampaignAIAction Action = ECampaignAIAction::None;
	/** Existing source/destination army; invalid only for a proposed new recruitment army. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	FGuid ArmyID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	FName TerritoryID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	FName UnitID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	int32 Count = 0;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|AI")
	FFactionResourceAmounts Cost;
};

/** Detached planner inputs copied from the existing world authorities/catalog.
 * They have no actor references or independently mutable campaign state. */
struct FCampaignAIRegion
{
	FName TerritoryID;
	EFactionID Owner = EFactionID::None;
	bool bHasSettlement = false;
	bool bHasTacticalMap = false;
	TArray<FName> Neighbors;
};
struct FCampaignAIRecruitOption
{
	FName UnitID;
	FFactionResourceAmounts PerUnitCost;
};
struct FCampaignAIPlanningInput
{
	EFactionID Faction = EFactionID::None;
	EFactionID Enemy = EFactionID::None;
	bool bAtWar = false;
	FFactionResourceAmounts Wallet;
	TArray<FCampaignAIRegion> Regions;
	TArray<FCampaignArmy> Armies;
	/** Caller filters loaded factories and current faction availability. */
	TArray<FCampaignAIRecruitOption> Recruitment;
};

namespace RomanEmpireCampaignAI
{
	inline constexpr int32 MaxRecruitPerTurn = 2;
	inline constexpr int32 TargetSettlementGarrison = 15;
	inline constexpr int32 MinimumSourceGarrison = 5;
	/** Exactly one bounded proposal. Stable lexical IDs/GUIDs break ties; no RNG consumed.
	 * Defend adjacent threatened settlements, then recruit against unresolved threats,
	 * then attack a supported adjacent enemy force, then rebuild settlement garrisons.
	 * Malformed snapshots fail closed. Execution must revalidate the current world. */
	ROMANEMPIREGAME_API FCampaignAIProposal Plan(const FCampaignAIPlanningInput& Input);
}
