// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Battle/TacticalBattleState.h"
#include "Campaign/CampaignEncounterState.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TacticalBattleSubsystem.generated.h"

class AUnitBase;
class ATacticalObjective;

/** One world-local tactical result owner; campaign outcome remains in CampaignManager. */
UCLASS()
class ROMANEMPIREGAME_API UTacticalBattleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	bool BeginEncounter(FName ObjectiveID, FGuid PlayerSquad, FGuid EnemySquad,
		FGuid EncounterID = FGuid());
	bool HasEncounter() const { return Battle.HasEncounter(); }
	bool RegisterReinforcement(AUnitBase* Unit);
	UFUNCTION(BlueprintPure, Category = "Battle")
	ATacticalObjective* GetObjectiveActor() const;

	UFUNCTION(BlueprintPure, Category = "Battle")
	FTacticalBattleResult GetResult() const { return Battle.GetResult(); }

	UFUNCTION(BlueprintPure, Category = "Battle")
	bool IsResolved() const { return Battle.IsResolved(); }
	UFUNCTION(BlueprintPure, Category = "Battle")
	EFactionID GetAttackingFaction() const { return Battle.GetAttackingFaction(); }
	UFUNCTION(BlueprintPure, Category = "Battle")
	EFactionID GetDefendingFaction() const { return Battle.GetDefendingFaction(); }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalBattleChanged, FTacticalBattleResult, Result);

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnTacticalBattleChanged OnTacticalBattleChanged;

private:
	friend class ACampaignManager;
	bool BeginCampaignDeployment(const FCampaignEncounterPlan& Plan, const TArray<FCampaignSquadBinding>& Bindings);
	bool BeginFromSquads(FName ObjectiveID, EFactionID PlayerFaction, EFactionID EnemyFaction,
		const TArray<FTacticalSquadSeed>& Seeds, FGuid EncounterID, bool bCampaign = false, EFactionID Attacker = EFactionID::None);
	bool PrepareReinforcement(AUnitBase* Unit, FTacticalBattleState& OutBattle, FGuid& OutSquad) const;
	void CommitReinforcement(AUnitBase* Unit, FGuid SquadID, FTacticalBattleState&& Prepared);
	friend class ATacticalObjective;
	bool RegisterObjective(ATacticalObjective* Objective);
	bool CaptureObjective(ATacticalObjective* Objective, AUnitBase* Occupant);
	TWeakObjectPtr<ATacticalObjective> ObjectiveActor;
	UFUNCTION()
	void HandleUnitDied(AUnitBase* Unit);

	void TrackUnit(AUnitBase* Unit, FGuid SquadID);
	void BroadcastResult();
	void DriveAIAttackers();
	FTimerHandle AIAttackTimer;
	/** Ephemeral command deduplication, not a roster/order or campaign owner. */
	TMap<TWeakObjectPtr<AUnitBase>, TWeakObjectPtr<AUnitBase>> LastAIAttackTargets;
	TSet<TWeakObjectPtr<AUnitBase>> OccupationAttempted;

	FTacticalBattleState Battle;
	TMap<TWeakObjectPtr<AUnitBase>, FGuid> ActiveRoster;
	bool bDeinitializing = false;
	bool bCampaignDeployment = false;
};
