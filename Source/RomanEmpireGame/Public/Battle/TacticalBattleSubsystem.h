// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Battle/TacticalBattleState.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TacticalBattleSubsystem.generated.h"

class AUnitBase;

/** One world-local tactical result owner; campaign outcome remains in CampaignManager. */
UCLASS()
class ROMANEMPIREGAME_API UTacticalBattleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	bool BeginEncounter(FName ObjectiveID, FGuid PlayerSquad, FGuid EnemySquad);
	bool RegisterReinforcement(AUnitBase* Unit);
	/** Called only after an objective actor has validated its world-space capture rule. */
	bool CaptureObjective(FName ObjectiveID, EFactionID CapturingFaction);

	UFUNCTION(BlueprintPure, Category = "Battle")
	FTacticalBattleResult GetResult() const { return Battle.GetResult(); }

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTacticalBattleChanged, FTacticalBattleResult, Result);

	UPROPERTY(BlueprintAssignable, Category = "Battle")
	FOnTacticalBattleChanged OnTacticalBattleChanged;

private:
	UFUNCTION()
	void HandleUnitDied(AUnitBase* Unit);

	void TrackUnit(AUnitBase* Unit, FGuid SquadID);
	void BroadcastResult();

	FTacticalBattleState Battle;
	TMap<TWeakObjectPtr<AUnitBase>, FGuid> ActiveRoster;
	bool bDeinitializing = false;
};
