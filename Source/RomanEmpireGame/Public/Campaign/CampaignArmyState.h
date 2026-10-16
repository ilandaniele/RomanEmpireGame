// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "CampaignArmyState.generated.h"

/** Content identity plus strategic quantity, not a live actor or a tactical squad. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignArmyUnitStack
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	FName UnitID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	int32 Count = 0;
};

/** ArmyID survives serialization; tactical squad GUIDs must be explicitly mapped later. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignArmy
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	FGuid ArmyID;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	EFactionID Faction = EFactionID::None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	FName TerritoryID = NAME_None;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame, Category = "Campaign|Armies")
	TArray<FCampaignArmyUnitStack> Units;
};

UENUM(BlueprintType)
enum class ECampaignArmyFailure : uint8
{
	None, InvalidArmyID, DuplicateArmyID, UnknownArmy, InvalidFaction,
	InvalidTerritoryID, InvalidUnitID, DuplicateUnitID, InvalidUnitCount,
	CountOverflow, InsufficientUnits, Unavailable, MutationInProgress, WrongMode,
	NotAdjacent, NotOwned, EncounterPending, SameArmy, NotCoLocated,
	InvalidContent, SettlementRequired, CostOverflow, InsufficientResources
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignArmyMutationResult
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Armies")
	ECampaignArmyFailure Failure = ECampaignArmyFailure::UnknownArmy;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Armies")
	FGuid ArmyID;
	bool WasSuccessful() const { return Failure == ECampaignArmyFailure::None; }
};

/** A current-state quote, never a reservation or an additional resource owner. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignRecruitmentQuote
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Armies")
	ECampaignArmyFailure Failure = ECampaignArmyFailure::Unavailable;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Campaign|Armies")
	FFactionResourceAmounts Cost;
	bool IsAvailable() const { return Failure == ECampaignArmyFailure::None; }
};

/** Serializable value owner. World authority validates catalog, graph, payer and phase.
 * Pure mutations validate shape/quantities atomically; queries always return detached values. */
USTRUCT()
struct ROMANEMPIREGAME_API FCampaignArmyState
{
	GENERATED_BODY()
public:
	FCampaignArmyMutationResult RegisterArmy(const FCampaignArmy& Army);
	FCampaignArmyMutationResult AddUnits(FGuid ArmyID, FName UnitID, int32 Count);
	FCampaignArmyMutationResult RemoveUnits(FGuid ArmyID, FName UnitID, int32 Count);
	FCampaignArmyMutationResult RelocateArmy(FGuid ArmyID, FName TerritoryID);
	/** Conserves quantities between co-located same-faction armies; retires an emptied source.
	 * Destination must exist. Any rejection leaves both armies and every other row intact. */
	FCampaignArmyMutationResult TransferUnits(FGuid SourceArmyID, FGuid DestinationArmyID, FName UnitID, int32 Count);
	/** Reject an entire malformed snapshot without replacing any previous army. */
	FCampaignArmyMutationResult ReplaceFromSnapshot(const TArray<FCampaignArmy>& Snapshot);
	bool TryGetArmy(FGuid ArmyID, FCampaignArmy& OutArmy) const;
	TArray<FCampaignArmy> GetArmies() const { return Armies; }
	int32 GetArmyCount() const { return Armies.Num(); }
	void Reset() { Armies.Reset(); }
	static ECampaignArmyFailure ValidateArmy(const FCampaignArmy& Army);
	static int32 GetUnitCount(const FCampaignArmy& Army);
private:
	// Sorted GUID order and sorted unit IDs make export/serialization deterministic.
	UPROPERTY(SaveGame)
	TArray<FCampaignArmy> Armies;
	int32 FindArmy(FGuid ArmyID) const;
	static void SortUnits(FCampaignArmy& Army);
};
