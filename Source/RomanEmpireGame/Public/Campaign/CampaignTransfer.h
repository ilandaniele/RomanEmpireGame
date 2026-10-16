// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "Campaign/CampaignEncounterState.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Campaign/TerritoryData.h"
#include "Faction/DiplomacyState.h"
#include "Faction/FactionData.h"
#include "Math/RandomStream.h"
#include "UObject/SoftObjectPath.h"
#include "CampaignTransfer.generated.h"

UENUM(BlueprintType)
enum class ECampaignTransferPhase : uint8 { Strategic, TacticalDeployment };

UENUM(BlueprintType)
enum class ECampaignTransferFailure : uint8
{
	None, UnsupportedVersion, InvalidPhase, InvalidMapReference, InvalidFactionState,
	InvalidTerritories, InvalidArmies, InvalidEncounter, InvalidTurnState,
	Unavailable, MutationInProgress, RefundRejected, InvalidContent
};

/** Both seeds are explicit: Engine FRandomStream's current Seed lacks SaveGame metadata. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignRandomCheckpoint
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 InitialSeed = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 CurrentSeed = 0;
	static FCampaignRandomCheckpoint Capture(const FRandomStream& Stream);
	bool Restore(FRandomStream& OutStream) const;
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignFactionWallet
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") EFactionID Faction = EFactionID::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FFactionResourceAmounts Resources;
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignTurnCheckpoint
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 CurrentTurn = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 MaxTurns = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 ConquestVictoryTerritories = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 EconomicVictoryGold = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 LastAutosaveRequestedTurn = 0;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") bool bCampaignActive = false;
	/** Strategic only: an accepted unbound AI attack awaits post-turn travel/retry. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") bool bAwaitingAITacticalTravel = false;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") ECampaignOutcome Outcome = ECampaignOutcome::InProgress;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") EVictoryCondition VictoryCondition = EVictoryCondition::Conquest;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FCampaignRandomCheckpoint Random;
	bool IsValid() const;
};

/** Temporary transaction staging, NOT a live owner/subsystem or a saved actor graph. */
struct ROMANEMPIREGAME_API FCampaignTransferCandidates
{
	EFactionID PlayerFaction = EFactionID::None;
	TMap<EFactionID, FFactionResourceAmounts> Wallets;
	FDiplomacyState Diplomacy;
	FTerritoryCampaignState Territories;
	FCampaignArmyState Armies;
	FCampaignEncounterState Encounter;
	FCampaignTurnCheckpoint Turn;
};

/** Immutable handover payload at a safe boundary. Runtime settles all paid producer
 * reservations first, validates actual map/content metadata, then commits restored owners
 * before readiness. This value alone is not a session, travel API or disk save. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignTransferSnapshot
{
	GENERATED_BODY()
	static constexpr int32 CurrentSchemaVersion = 2;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") int32 SchemaVersion = CurrentSchemaVersion;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") ECampaignTransferPhase Phase = ECampaignTransferPhase::Strategic;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FSoftObjectPath DestinationMap;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FSoftObjectPath StrategicReturnMap;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") EFactionID PlayerFaction = EFactionID::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") TArray<FCampaignFactionWallet> Factions;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") TArray<FDiplomaticRelation> Diplomacy;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") TArray<FTerritoryRuntimeSnapshot> Territories;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") TArray<FCampaignArmy> Armies;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FCampaignEncounterCheckpoint Encounter;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign") FCampaignTurnCheckpoint Turn;
	/** Rebind against current definitions/participants; invalid payload leaves output intact. */
	ECampaignTransferFailure BuildCandidates(const FTerritoryCampaignState& CurrentCatalog,
		const TArray<EFactionID>& ActiveFactions, FCampaignTransferCandidates& OutCandidates) const;
};
