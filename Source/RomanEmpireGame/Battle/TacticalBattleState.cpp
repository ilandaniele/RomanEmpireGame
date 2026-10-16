// Copyright Roman Empire Game. All Rights Reserved.

#include "Battle/TacticalBattleState.h"

bool FTacticalBattleState::BeginEncounter(FName InObjectiveID, EFactionID InPlayerFaction,
	EFactionID InEnemyFaction, const TArray<FTacticalSquadSeed>& StartingSquads, FGuid InEncounterID, EFactionID InAttackingFaction)
{
	const EFactionID Attacker = InAttackingFaction == EFactionID::None ? InPlayerFaction : InAttackingFaction;
	if (bHasEncounter || InObjectiveID.IsNone() || InPlayerFaction == EFactionID::None ||
		InEnemyFaction == EFactionID::None || InPlayerFaction == InEnemyFaction ||
		(Attacker != InPlayerFaction && Attacker != InEnemyFaction)) return false;
	TSet<FGuid> Seen;
	bool bHasPlayerForce = false;
	bool bHasEnemyForce = false;
	for (const FTacticalSquadSeed& Seed : StartingSquads)
	{
		if (!Seed.SquadID.IsValid() || Seed.UnitCount <= 0 || Seen.Contains(Seed.SquadID) ||
			(Seed.Faction != InPlayerFaction && Seed.Faction != InEnemyFaction)) return false;
		Seen.Add(Seed.SquadID);
		if (Seed.Faction == InPlayerFaction) bHasPlayerForce = true;
		else bHasEnemyForce = true;
	}
	if (!bHasPlayerForce || !bHasEnemyForce) return false;

	EncounterID = InEncounterID.IsValid() ? InEncounterID : FGuid::NewGuid();
	ObjectiveID = InObjectiveID;
	PlayerFaction = InPlayerFaction;
	EnemyFaction = InEnemyFaction;
	AttackingFaction = Attacker;
	bHasEncounter = true;
	for (const FTacticalSquadSeed& Seed : StartingSquads)
	{
		RegisterReinforcement(Seed);
	}
	return true;
}

bool FTacticalBattleState::IsValidSeed(const FTacticalSquadSeed& Squad) const
{
	return Squad.SquadID.IsValid() && Squad.UnitCount > 0 &&
		(Squad.Faction == PlayerFaction || Squad.Faction == EnemyFaction) &&
		!SquadIndices.Contains(Squad.SquadID);
}

bool FTacticalBattleState::RegisterReinforcement(const FTacticalSquadSeed& Squad)
{
	if (!bHasEncounter || Outcome != ETacticalBattleOutcome::InProgress || !IsValidSeed(Squad)) return false;
	FTacticalBattleSquadResult Record;
	Record.SquadID = Squad.SquadID;
	Record.Faction = Squad.Faction;
	Record.InitialUnits = Squad.UnitCount;
	Record.Survivors = Squad.UnitCount;
	SquadIndices.Add(Squad.SquadID, Squads.Add(Record));
	return true;
}

bool FTacticalBattleState::RecordCasualty(FGuid SquadID, int32 Count)
{
	const int32* Index = SquadIndices.Find(SquadID);
	if (!bHasEncounter || Outcome != ETacticalBattleOutcome::InProgress || !Index ||
		Count <= 0 || Count > Squads[*Index].Survivors) return false;
	FTacticalBattleSquadResult& Squad = Squads[*Index];
	Squad.Survivors -= Count;
	Squad.Casualties += Count;
	EvaluateCasualties();
	return true;
}

bool FTacticalBattleState::HasSurvivors(EFactionID Faction) const
{
	for (const FTacticalBattleSquadResult& Squad : Squads)
	{
		if (Squad.Faction == Faction && Squad.Survivors > 0) return true;
	}
	return false;
}

void FTacticalBattleState::EvaluateCasualties()
{
	const bool bPlayerAlive = HasSurvivors(PlayerFaction);
	const bool bEnemyAlive = HasSurvivors(EnemyFaction);
	if (!bPlayerAlive && !bEnemyAlive) Outcome = ETacticalBattleOutcome::Draw;
	else if (!HasSurvivors(AttackingFaction))
		Outcome = AttackingFaction == PlayerFaction ? ETacticalBattleOutcome::PlayerDefeat : ETacticalBattleOutcome::PlayerVictory;
}

bool FTacticalBattleState::CaptureObjective(EFactionID CapturingFaction)
{
	if (!bHasEncounter || Outcome != ETacticalBattleOutcome::InProgress ||
		CapturingFaction != AttackingFaction || !HasSurvivors(AttackingFaction) || HasSurvivors(GetDefendingFaction())) return false;
	CapturedObjectiveID = ObjectiveID;
	Outcome = AttackingFaction == PlayerFaction ? ETacticalBattleOutcome::PlayerVictory : ETacticalBattleOutcome::PlayerDefeat;
	return true;
}

FTacticalBattleResult FTacticalBattleState::GetResult() const
{
	FTacticalBattleResult Result;
	Result.EncounterID = EncounterID;
	Result.Outcome = Outcome;
	Result.ObjectiveID = ObjectiveID;
	Result.CapturedObjectiveID = CapturedObjectiveID;
	Result.Squads = Squads;
	if (Outcome == ETacticalBattleOutcome::PlayerVictory) Result.Winner = PlayerFaction;
	else if (Outcome == ETacticalBattleOutcome::PlayerDefeat) Result.Winner = EnemyFaction;
	return Result;
}

void FTacticalBattleState::Reset()
{
	bHasEncounter = false;
	EncounterID.Invalidate();
	ObjectiveID = NAME_None;
	CapturedObjectiveID = NAME_None;
	PlayerFaction = EFactionID::None;
	EnemyFaction = EFactionID::None;
	AttackingFaction = EFactionID::None;
	Outcome = ETacticalBattleOutcome::InProgress;
	Squads.Reset();
	SquadIndices.Reset();
}
