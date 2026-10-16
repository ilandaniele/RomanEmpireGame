// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Battle/TacticalBattleResult.h"
#include "Campaign/CampaignArmyState.h"
#include "Campaign/TerritoryData.h"
#include "CampaignEncounterState.generated.h"

UENUM(BlueprintType)
enum class ECampaignEncounterFailure : uint8
{
	None, InvalidEncounterID, EncounterPending, AlreadyApplied, NoEncounter,
	InvalidObjectiveID, InvalidAttacker, InvalidTarget, NoDefenders, NotAdjacent,
	StaleDeployment, InvalidBinding, DuplicateSquad, OverDeployment,
	IncompleteDeployment, ResultMismatch, InvalidResult, MutationFailed,
	Unavailable, WrongMode, NotAtWar, InvalidContent, MutationInProgress,
	EncounterAlreadyRunning, UnsupportedEncounter
};

/** Frozen strategic deployment, not a second mutable army authority. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignEncounterPlan
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") FGuid EncounterID;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") FName ObjectiveID = NAME_None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") FGuid AttackingArmyID;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") FName TargetTerritoryID = NAME_None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") EFactionID Attacker = EFactionID::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") EFactionID Defender = EFactionID::None;
	/** Human perspective is independent of attacking/defending role. */
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") EFactionID PlayerFaction = EFactionID::None;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") TArray<FCampaignArmy> Armies;
};

/** Safe travel boundary only: unmaterialized plan or consumed ledger, never local squads. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignEncounterCheckpoint
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") FCampaignEncounterPlan PendingPlan;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") TArray<FGuid> AppliedEncounterIDs;
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Campaign|Encounter") TArray<FGuid> EnrolledReceiptIDs;
};

/** Explicit world-local squad -> strategic army/content identity. Never infer from GUID order. */
USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FCampaignSquadBinding
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Encounter") FGuid SquadID;
	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Encounter") FGuid ArmyID;
	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Encounter") FName UnitID = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "Campaign|Encounter") int32 UnitCount = 0;
};

/** Pure transaction coordinator. Runtime validates active factions/content/phase and supplies
 * its canonical tactical result. Apply publishes no events: both live owners must commit
 * before their runtime invalidations. Squad bindings are transient and not a disk-save schema. */
USTRUCT()
struct ROMANEMPIREGAME_API FCampaignEncounterState
{
	GENERATED_BODY()
public:
	ECampaignEncounterFailure Begin(FGuid EncounterID, FName ObjectiveID, FGuid AttackingArmyID,
		FName TargetTerritoryID, EFactionID Attacker, const FCampaignArmyState& Armies,
		const FTerritoryCampaignState& Territories, EFactionID PlayerFaction = EFactionID::None);
	/** Cancel only an unmaterialized plan, preserving consumed identity/receipt ledgers. */
	bool CancelUnboundPending();
	ECampaignEncounterFailure BindSquad(FGuid EncounterID, const FCampaignSquadBinding& Binding);
	/** Atomically extend the frozen plan, explicit binding and canonical inventory for one
	 * validated paid receipt. Runtime owns payment/producer/content validation. */
	ECampaignEncounterFailure EnrollReinforcement(FGuid EncounterID, FGuid ReceiptID,
		const FCampaignSquadBinding& Binding, FCampaignArmyState& Armies,
		const FTerritoryCampaignState& Territories);
	ECampaignEncounterFailure Apply(const FTacticalBattleResult& Result,
		FCampaignArmyState& Armies, FTerritoryCampaignState& Territories);
	ECampaignEncounterFailure ValidateDeployment(const FCampaignArmyState& Armies,
		const FTerritoryCampaignState& Territories) const;
	FCampaignEncounterPlan GetPlan() const { return Plan; }
	TArray<FCampaignSquadBinding> GetBindings() const { return Bindings; }
	bool HasPendingEncounter() const;
	ECampaignEncounterFailure ExportCheckpoint(FCampaignEncounterCheckpoint& OutCheckpoint) const;
	ECampaignEncounterFailure RestoreCheckpoint(const FCampaignEncounterCheckpoint& Checkpoint,
		const FCampaignArmyState& Armies, const FTerritoryCampaignState& Territories);
	bool HasApplied(FGuid EncounterID) const { return AppliedEncounters.Contains(EncounterID); }
	void Reset();
private:
	ECampaignEncounterFailure ValidateCurrent(const FCampaignArmyState& Armies,
		const FTerritoryCampaignState& Territories) const;
	bool IsFullyBound() const;
	UPROPERTY() FCampaignEncounterPlan Plan;
	UPROPERTY() TArray<FCampaignSquadBinding> Bindings;
	UPROPERTY() TArray<FGuid> AppliedEncounters;
	UPROPERTY() TArray<FGuid> EnrolledReceipts;
};
