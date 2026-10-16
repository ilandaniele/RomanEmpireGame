// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignTransfer.h"
#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

FCampaignRandomCheckpoint FCampaignRandomCheckpoint::Capture(const FRandomStream& Stream)
{
	FCampaignRandomCheckpoint Result;
	Result.InitialSeed = Stream.GetInitialSeed(); Result.CurrentSeed = Stream.GetCurrentSeed();
	return Result;
}

bool FCampaignRandomCheckpoint::Restore(FRandomStream& OutStream) const
{
	// Use the engine's reflected field, never assume or reinterpret its private memory layout.
	const FIntProperty* Seed = FindFProperty<FIntProperty>(TBaseStructure<FRandomStream>::Get(), TEXT("Seed"));
	if (!Seed) return false;
	FRandomStream Candidate(InitialSeed);
	Seed->SetPropertyValue_InContainer(&Candidate, CurrentSeed);
	OutStream = Candidate;
	return true;
}

bool FCampaignTurnCheckpoint::IsValid() const
{
	return CurrentTurn >= 1 && MaxTurns >= 1 && MaxTurns < MAX_int32 && int64(CurrentTurn) <= int64(MaxTurns) + 1 &&
		ConquestVictoryTerritories >= 1 && EconomicVictoryGold >= 1 &&
		LastAutosaveRequestedTurn >= 0 && LastAutosaveRequestedTurn <= CurrentTurn &&
		static_cast<uint8>(Outcome) <= static_cast<uint8>(ECampaignOutcome::Draw) &&
		static_cast<uint8>(VictoryCondition) <= static_cast<uint8>(EVictoryCondition::Diplomatic) &&
		bCampaignActive == (Outcome == ECampaignOutcome::InProgress) &&
		(CurrentTurn <= MaxTurns || Outcome != ECampaignOutcome::InProgress) && (!bAwaitingAITacticalTravel || bCampaignActive);
}

namespace
{
	bool IsMapPath(const FSoftObjectPath& Path)
	{
		return Path.IsValid() && Path.GetSubPathString().IsEmpty() &&
			FPackageName::IsValidLongPackageName(Path.GetLongPackageName()) &&
			Path.GetAssetName() == FPackageName::GetShortName(Path.GetLongPackageName());
	}
}

ECampaignTransferFailure FCampaignTransferSnapshot::BuildCandidates(const FTerritoryCampaignState& CurrentCatalog,
	const TArray<EFactionID>& ActiveFactions, FCampaignTransferCandidates& OutCandidates) const
{
	if (SchemaVersion != CurrentSchemaVersion) return ECampaignTransferFailure::UnsupportedVersion;
	if (Phase != ECampaignTransferPhase::Strategic && Phase != ECampaignTransferPhase::TacticalDeployment) return ECampaignTransferFailure::InvalidPhase;
	if (!IsMapPath(DestinationMap) || (Phase == ECampaignTransferPhase::TacticalDeployment && !IsMapPath(StrategicReturnMap)) ||
		(Phase == ECampaignTransferPhase::Strategic && !StrategicReturnMap.IsNull())) return ECampaignTransferFailure::InvalidMapReference;
	if (!Turn.IsValid()) return ECampaignTransferFailure::InvalidTurnState;
	FCampaignTransferCandidates Candidate;
	TSet<EFactionID> Allowed;
	for (EFactionID ID : ActiveFactions)
	{
		if (!RomanEmpireDiplomacy::IsValidFaction(ID) || Allowed.Contains(ID)) return ECampaignTransferFailure::InvalidFactionState;
		Allowed.Add(ID);
	}
	if (Allowed.Num() < 2 || !Allowed.Contains(PlayerFaction) || Factions.Num() != Allowed.Num()) return ECampaignTransferFailure::InvalidFactionState;
	for (const FCampaignFactionWallet& Faction : Factions)
	{
		if (!Allowed.Contains(Faction.Faction) || Candidate.Wallets.Contains(Faction.Faction) || !Faction.Resources.IsNonNegative())
			return ECampaignTransferFailure::InvalidFactionState;
		Candidate.Wallets.Add(Faction.Faction, Faction.Resources);
	}
	for (const FDiplomaticRelation& Relation : Diplomacy)
		if (!Allowed.Contains(Relation.Pair.FirstFaction) || !Allowed.Contains(Relation.Pair.SecondFaction)) return ECampaignTransferFailure::InvalidFactionState;
	if (!Candidate.Diplomacy.ReplaceFromSnapshot(Diplomacy)) return ECampaignTransferFailure::InvalidFactionState;
	Candidate.Territories = CurrentCatalog;
	if (CurrentCatalog.GetTerritoryCount() == 0 || !Candidate.Territories.ReplaceRuntimeSnapshot(Territories)) return ECampaignTransferFailure::InvalidTerritories;
	if (!Candidate.Armies.ReplaceFromSnapshot(Armies).WasSuccessful()) return ECampaignTransferFailure::InvalidArmies;
	for (const FCampaignArmy& Army : Candidate.Armies.GetArmies())
	{
		EFactionID Owner = EFactionID::None;
		if (!Allowed.Contains(Army.Faction) || !Candidate.Territories.TryGetOwner(Army.TerritoryID, Owner) || Owner != Army.Faction)
			return ECampaignTransferFailure::InvalidArmies;
	}
	if (Candidate.Encounter.RestoreCheckpoint(Encounter, Candidate.Armies, Candidate.Territories) != ECampaignEncounterFailure::None)
		return ECampaignTransferFailure::InvalidEncounter;
	const FCampaignEncounterPlan Plan = Candidate.Encounter.GetPlan();
	if (Phase == ECampaignTransferPhase::TacticalDeployment)
	{
		if (!Candidate.Encounter.HasPendingEncounter() || Plan.PlayerFaction != PlayerFaction || !Turn.bCampaignActive || Turn.bAwaitingAITacticalTravel ||
			!Allowed.Contains(Plan.Attacker) || !Allowed.Contains(Plan.Defender) || Candidate.Diplomacy.GetStatus(Plan.Attacker, Plan.Defender) != EDiplomaticStatus::War)
			return ECampaignTransferFailure::InvalidEncounter;
	}
	else if (Candidate.Encounter.HasPendingEncounter() != Turn.bAwaitingAITacticalTravel ||
		(Turn.bAwaitingAITacticalTravel && (Plan.PlayerFaction != PlayerFaction || Plan.Defender != PlayerFaction ||
			!Allowed.Contains(Plan.Attacker) || Candidate.Diplomacy.GetStatus(Plan.Attacker, Plan.Defender) != EDiplomaticStatus::War)))
		return ECampaignTransferFailure::InvalidEncounter;
	Candidate.PlayerFaction = PlayerFaction;
	Candidate.Turn = Turn;
	OutCandidates = MoveTemp(Candidate);
	return ECampaignTransferFailure::None;
}
