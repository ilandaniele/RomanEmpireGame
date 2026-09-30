// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Battle/TacticalBattleResult.h"
#include "CoreMinimal.h"

struct FTacticalSquadSeed
{
	FGuid SquadID;
	EFactionID Faction = EFactionID::None;
	int32 UnitCount = 0;
};

/** Pure, single-owner tactical result evaluator. World actors supply validated events. */
class ROMANEMPIREGAME_API FTacticalBattleState
{
public:
	bool BeginEncounter(FName InObjectiveID, EFactionID InPlayerFaction,
		EFactionID InEnemyFaction, const TArray<FTacticalSquadSeed>& StartingSquads);
	bool RegisterReinforcement(const FTacticalSquadSeed& Squad);
	bool RecordCasualty(FGuid SquadID, int32 Count = 1);
	bool CaptureObjective(EFactionID CapturingFaction);
	FTacticalBattleResult GetResult() const;
	bool HasEncounter() const { return bHasEncounter; }
	void Reset();

private:
	bool IsValidSeed(const FTacticalSquadSeed& Squad) const;
	bool HasSurvivors(EFactionID Faction) const;
	void EvaluateCasualties();

	bool bHasEncounter = false;
	FName ObjectiveID = NAME_None;
	FName CapturedObjectiveID = NAME_None;
	EFactionID PlayerFaction = EFactionID::None;
	EFactionID EnemyFaction = EFactionID::None;
	ETacticalBattleOutcome Outcome = ETacticalBattleOutcome::InProgress;
	TArray<FTacticalBattleSquadResult> Squads;
	TMap<FGuid, int32> SquadIndices;
};
