// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/CampaignEncounterState.h"
#include "Content/RomanContentDefinitions.h"
#include "UObject/Class.h"

namespace
{
	const FCampaignArmyUnitStack* FindStack(const FCampaignArmy& Army, FName ID)
	{
		return Army.Units.FindByPredicate([ID](const FCampaignArmyUnitStack& Stack) { return Stack.UnitID == ID; });
	}
	bool SameArmy(const FCampaignArmy& A, const FCampaignArmy& B)
	{
		if (A.ArmyID != B.ArmyID || A.Faction != B.Faction || A.TerritoryID != B.TerritoryID || A.Units.Num() != B.Units.Num()) return false;
		for (const FCampaignArmyUnitStack& Stack : A.Units)
		{
			const FCampaignArmyUnitStack* Other = FindStack(B, Stack.UnitID);
			if (!Other || Other->Count != Stack.Count) return false;
		}
		return true;
	}
}

bool FCampaignEncounterState::HasPendingEncounter() const
{
	return Plan.EncounterID.IsValid() && !HasApplied(Plan.EncounterID);
}

ECampaignEncounterFailure FCampaignEncounterState::Begin(FGuid EncounterID, FName ObjectiveID,
	FGuid AttackingArmyID, FName TargetTerritoryID, EFactionID Attacker,
	const FCampaignArmyState& Armies, const FTerritoryCampaignState& Territories, EFactionID PlayerFaction)
{
	if (!EncounterID.IsValid()) return ECampaignEncounterFailure::InvalidEncounterID;
	if (HasApplied(EncounterID)) return ECampaignEncounterFailure::AlreadyApplied;
	if (HasPendingEncounter()) return ECampaignEncounterFailure::EncounterPending;
	if (!RomanEmpireContent::IsStableID(ObjectiveID)) return ECampaignEncounterFailure::InvalidObjectiveID;
	FCampaignArmy Army;
	EFactionID SourceOwner = EFactionID::None, TargetOwner = EFactionID::None;
	if (!RomanEmpireContent::IsRuntimeFaction(Attacker) || !Armies.TryGetArmy(AttackingArmyID, Army) ||
		Army.Faction != Attacker || !Territories.TryGetOwner(Army.TerritoryID, SourceOwner) || SourceOwner != Attacker)
		return ECampaignEncounterFailure::InvalidAttacker;
	if (!Territories.TryGetOwner(TargetTerritoryID, TargetOwner) || !RomanEmpireContent::IsRuntimeFaction(TargetOwner) || TargetOwner == Attacker)
		return ECampaignEncounterFailure::InvalidTarget;
	const EFactionID Human = PlayerFaction == EFactionID::None ? Attacker : PlayerFaction;
	if (Human != Attacker && Human != TargetOwner) return ECampaignEncounterFailure::InvalidTarget;
	if (!Territories.AreAdjacent(Army.TerritoryID, TargetTerritoryID)) return ECampaignEncounterFailure::NotAdjacent;
	FCampaignEncounterPlan Candidate;
	Candidate.EncounterID = EncounterID; Candidate.ObjectiveID = ObjectiveID;
	Candidate.AttackingArmyID = AttackingArmyID; Candidate.TargetTerritoryID = TargetTerritoryID;
	Candidate.Attacker = Attacker; Candidate.Defender = TargetOwner;
	Candidate.PlayerFaction = Human;
	for (const FCampaignArmy& Current : Armies.GetArmies())
	{
		if (Current.ArmyID == AttackingArmyID || (Current.TerritoryID == TargetTerritoryID && Current.Faction == TargetOwner))
			Candidate.Armies.Add(Current);
		else if (Current.TerritoryID == TargetTerritoryID)
			return ECampaignEncounterFailure::InvalidTarget; // No implicit third-party force policy.
	}
	if (Candidate.Armies.Num() < 2) return ECampaignEncounterFailure::NoDefenders;
	Plan = MoveTemp(Candidate);
	Bindings.Reset();
	return ECampaignEncounterFailure::None;
}

bool FCampaignEncounterState::CancelUnboundPending()
{
	if (!HasPendingEncounter() || !Bindings.IsEmpty()) return false;
	Plan = FCampaignEncounterPlan(); return true;
}

ECampaignEncounterFailure FCampaignEncounterState::BindSquad(FGuid EncounterID, const FCampaignSquadBinding& Binding)
{
	if (HasApplied(EncounterID)) return ECampaignEncounterFailure::AlreadyApplied;
	if (!HasPendingEncounter()) return ECampaignEncounterFailure::NoEncounter;
	if (EncounterID != Plan.EncounterID) return ECampaignEncounterFailure::ResultMismatch;
	if (!Binding.SquadID.IsValid() || Binding.UnitCount <= 0 || !RomanEmpireContent::IsStableID(Binding.UnitID))
		return ECampaignEncounterFailure::InvalidBinding;
	// A tactical handle cannot masquerade as any strategic army identity.
	if (Plan.Armies.ContainsByPredicate([&](const FCampaignArmy& Army) { return Army.ArmyID == Binding.SquadID; }))
		return ECampaignEncounterFailure::InvalidBinding;
	if (Bindings.ContainsByPredicate([&](const FCampaignSquadBinding& Other) { return Other.SquadID == Binding.SquadID; }))
		return ECampaignEncounterFailure::DuplicateSquad;
	const FCampaignArmy* Army = Plan.Armies.FindByPredicate([&](const FCampaignArmy& Other) { return Other.ArmyID == Binding.ArmyID; });
	const FCampaignArmyUnitStack* Stack = Army ? FindStack(*Army, Binding.UnitID) : nullptr;
	if (!Stack) return ECampaignEncounterFailure::InvalidBinding;
	int64 Total = Binding.UnitCount;
	for (const FCampaignSquadBinding& Other : Bindings)
		if (Other.ArmyID == Binding.ArmyID && Other.UnitID == Binding.UnitID) Total += Other.UnitCount;
	if (Total > Stack->Count) return ECampaignEncounterFailure::OverDeployment;
	Bindings.Add(Binding);
	return ECampaignEncounterFailure::None;
}

ECampaignEncounterFailure FCampaignEncounterState::EnrollReinforcement(FGuid EncounterID, FGuid ReceiptID,
	const FCampaignSquadBinding& Binding, FCampaignArmyState& Armies, const FTerritoryCampaignState& Territories)
{
	if (HasApplied(EncounterID)) return ECampaignEncounterFailure::AlreadyApplied;
	if (!HasPendingEncounter()) return ECampaignEncounterFailure::NoEncounter;
	if (EncounterID != Plan.EncounterID) return ECampaignEncounterFailure::ResultMismatch;
	if (!ReceiptID.IsValid() || EnrolledReceipts.Contains(ReceiptID) || Binding.UnitCount != 1 ||
		!RomanEmpireContent::IsStableID(Binding.UnitID)) return ECampaignEncounterFailure::InvalidBinding;
	const ECampaignEncounterFailure Current = ValidateDeployment(Armies, Territories);
	if (Current != ECampaignEncounterFailure::None) return Current;
	FCampaignEncounterState Candidate = *this;
	FCampaignArmy* Planned = Candidate.Plan.Armies.FindByPredicate(
		[&](const FCampaignArmy& Army) { return Army.ArmyID == Binding.ArmyID; });
	if (!Planned) return ECampaignEncounterFailure::InvalidBinding;
	FCampaignArmyState CandidateArmies = Armies;
	if (!CandidateArmies.AddUnits(Binding.ArmyID, Binding.UnitID, 1).WasSuccessful())
		return ECampaignEncounterFailure::MutationFailed;
	if (!CandidateArmies.TryGetArmy(Binding.ArmyID, *Planned)) return ECampaignEncounterFailure::MutationFailed;
	const ECampaignEncounterFailure Bound = Candidate.BindSquad(EncounterID, Binding);
	if (Bound != ECampaignEncounterFailure::None) return Bound;
	Candidate.EnrolledReceipts.Add(ReceiptID);
	Armies = MoveTemp(CandidateArmies);
	*this = MoveTemp(Candidate);
	return ECampaignEncounterFailure::None;
}

bool FCampaignEncounterState::IsFullyBound() const
{
	for (const FCampaignArmy& Army : Plan.Armies)
		for (const FCampaignArmyUnitStack& Stack : Army.Units)
		{
			int64 Total = 0;
			for (const FCampaignSquadBinding& Binding : Bindings)
				if (Binding.ArmyID == Army.ArmyID && Binding.UnitID == Stack.UnitID) Total += Binding.UnitCount;
			if (Total != Stack.Count) return false;
		}
	return true;
}

ECampaignEncounterFailure FCampaignEncounterState::ValidateCurrent(const FCampaignArmyState& Armies,
	const FTerritoryCampaignState& Territories) const
{
	for (const FCampaignArmy& Snapshot : Plan.Armies)
	{
		FCampaignArmy Current;
		if (!Armies.TryGetArmy(Snapshot.ArmyID, Current) || !SameArmy(Snapshot, Current))
			return ECampaignEncounterFailure::StaleDeployment;
		EFactionID Owner = EFactionID::None;
		if (!Territories.TryGetOwner(Current.TerritoryID, Owner) || Owner != Current.Faction)
			return ECampaignEncounterFailure::StaleDeployment;
		if (Current.ArmyID == Plan.AttackingArmyID && !Territories.AreAdjacent(Current.TerritoryID, Plan.TargetTerritoryID))
			return ECampaignEncounterFailure::StaleDeployment;
	}
	// Arrival of another defender/third-party army invalidates the complete deployment too.
	for (const FCampaignArmy& Current : Armies.GetArmies())
		if (Current.TerritoryID == Plan.TargetTerritoryID && !Plan.Armies.ContainsByPredicate(
			[&](const FCampaignArmy& Snapshot) { return Snapshot.ArmyID == Current.ArmyID; }))
			return ECampaignEncounterFailure::StaleDeployment;
	return ECampaignEncounterFailure::None;
}

ECampaignEncounterFailure FCampaignEncounterState::ValidateDeployment(const FCampaignArmyState& Armies,
	const FTerritoryCampaignState& Territories) const
{
	if (!HasPendingEncounter()) return ECampaignEncounterFailure::NoEncounter;
	const ECampaignEncounterFailure Failure = ValidateCurrent(Armies, Territories);
	if (Failure != ECampaignEncounterFailure::None) return Failure;
	return IsFullyBound() ? ECampaignEncounterFailure::None : ECampaignEncounterFailure::IncompleteDeployment;
}

ECampaignEncounterFailure FCampaignEncounterState::Apply(const FTacticalBattleResult& Result,
	FCampaignArmyState& Armies, FTerritoryCampaignState& Territories)
{
	if (HasApplied(Result.EncounterID)) return ECampaignEncounterFailure::AlreadyApplied;
	if (!HasPendingEncounter()) return ECampaignEncounterFailure::NoEncounter;
	if (Result.EncounterID != Plan.EncounterID || Result.ObjectiveID != Plan.ObjectiveID)
		return ECampaignEncounterFailure::ResultMismatch;
	const ECampaignEncounterFailure CurrentFailure = ValidateCurrent(Armies, Territories);
	if (CurrentFailure != ECampaignEncounterFailure::None) return CurrentFailure;
	if (!IsFullyBound()) return ECampaignEncounterFailure::IncompleteDeployment;
	if (Result.Squads.Num() != Bindings.Num()) return ECampaignEncounterFailure::ResultMismatch;
	TSet<FGuid> Seen;
	int64 AttackerSurvivors = 0, DefenderSurvivors = 0;
	for (const FTacticalBattleSquadResult& Squad : Result.Squads)
	{
		const FCampaignSquadBinding* Binding = Bindings.FindByPredicate([&](const FCampaignSquadBinding& Other) { return Other.SquadID == Squad.SquadID; });
		if (!Binding || Seen.Contains(Squad.SquadID)) return ECampaignEncounterFailure::ResultMismatch;
		const FCampaignArmy* Army = Plan.Armies.FindByPredicate([&](const FCampaignArmy& Other) { return Other.ArmyID == Binding->ArmyID; });
		if (!Army || Squad.Faction != Army->Faction || Squad.InitialUnits != Binding->UnitCount || Squad.Survivors < 0 ||
			Squad.Casualties < 0 || static_cast<int64>(Squad.Survivors) + Squad.Casualties != Squad.InitialUnits)
			return ECampaignEncounterFailure::InvalidResult;
		Seen.Add(Squad.SquadID);
		if (Squad.Faction == Plan.Attacker) AttackerSurvivors += Squad.Survivors;
		else DefenderSurvivors += Squad.Survivors;
	}
	bool bCapture = false;
	if (Result.Outcome == ETacticalBattleOutcome::Draw)
	{
		if (Result.Winner != EFactionID::None || !Result.CapturedObjectiveID.IsNone() || AttackerSurvivors != 0 || DefenderSurvivors != 0)
			return ECampaignEncounterFailure::InvalidResult;
	}
	else if (Result.Outcome == ETacticalBattleOutcome::PlayerVictory || Result.Outcome == ETacticalBattleOutcome::PlayerDefeat)
	{
		if ((Result.Outcome == ETacticalBattleOutcome::PlayerVictory) != (Result.Winner == Plan.PlayerFaction)) return ECampaignEncounterFailure::InvalidResult;
		if (Result.Winner == Plan.Attacker)
		{
			if (Result.CapturedObjectiveID != Plan.ObjectiveID || AttackerSurvivors <= 0 || DefenderSurvivors != 0) return ECampaignEncounterFailure::InvalidResult;
			bCapture = true;
		}
		else if (Result.Winner != Plan.Defender || !Result.CapturedObjectiveID.IsNone() || AttackerSurvivors != 0 || DefenderSurvivors <= 0)
			return ECampaignEncounterFailure::InvalidResult;
	}
	else return ECampaignEncounterFailure::InvalidResult;
	FCampaignArmyState NextArmies = Armies;
	FTerritoryCampaignState NextTerritories = Territories;
	for (const FTacticalBattleSquadResult& Squad : Result.Squads)
	{
		const FCampaignSquadBinding& Binding = *Bindings.FindByPredicate([&](const FCampaignSquadBinding& Other) { return Other.SquadID == Squad.SquadID; });
		if (Squad.Casualties > 0 && !NextArmies.RemoveUnits(Binding.ArmyID, Binding.UnitID, Squad.Casualties).WasSuccessful())
			return ECampaignEncounterFailure::MutationFailed;
	}
	if (bCapture && (!NextArmies.RelocateArmy(Plan.AttackingArmyID, Plan.TargetTerritoryID).WasSuccessful() ||
		!NextTerritories.ChangeOwner(Plan.TargetTerritoryID, Plan.Attacker).WasSuccessful())) return ECampaignEncounterFailure::MutationFailed;
	// No external calls or events between validation and the complete value commit.
	Armies = MoveTemp(NextArmies);
	Territories = MoveTemp(NextTerritories);
	AppliedEncounters.Add(Plan.EncounterID);
	return ECampaignEncounterFailure::None;
}

ECampaignEncounterFailure FCampaignEncounterState::ExportCheckpoint(FCampaignEncounterCheckpoint& OutCheckpoint) const
{
	if (HasPendingEncounter() && !Bindings.IsEmpty()) return ECampaignEncounterFailure::EncounterAlreadyRunning;
	FCampaignEncounterCheckpoint Candidate;
	if (HasPendingEncounter()) Candidate.PendingPlan = Plan;
	Candidate.AppliedEncounterIDs = AppliedEncounters;
	Candidate.EnrolledReceiptIDs = EnrolledReceipts;
	OutCheckpoint = MoveTemp(Candidate);
	return ECampaignEncounterFailure::None;
}

ECampaignEncounterFailure FCampaignEncounterState::RestoreCheckpoint(const FCampaignEncounterCheckpoint& Checkpoint,
	const FCampaignArmyState& Armies, const FTerritoryCampaignState& Territories)
{
	const auto ValidIDs = [](const TArray<FGuid>& IDs)
	{
		TSet<FGuid> Seen;
		for (FGuid ID : IDs) { if (!ID.IsValid() || Seen.Contains(ID)) return false; Seen.Add(ID); }
		return true;
	};
	if (!ValidIDs(Checkpoint.AppliedEncounterIDs) || !ValidIDs(Checkpoint.EnrolledReceiptIDs)) return ECampaignEncounterFailure::InvalidResult;
	FCampaignEncounterState Candidate;
	Candidate.AppliedEncounters = Checkpoint.AppliedEncounterIDs;
	Candidate.EnrolledReceipts = Checkpoint.EnrolledReceiptIDs;
	const FCampaignEncounterPlan& Pending = Checkpoint.PendingPlan;
	if (Pending.EncounterID.IsValid())
	{
		if (Pending.PlayerFaction != Pending.Attacker && Pending.PlayerFaction != Pending.Defender) return ECampaignEncounterFailure::InvalidResult;
		const ECampaignEncounterFailure Began = Candidate.Begin(Pending.EncounterID, Pending.ObjectiveID,
			Pending.AttackingArmyID, Pending.TargetTerritoryID, Pending.Attacker, Armies, Territories, Pending.PlayerFaction);
		if (Began != ECampaignEncounterFailure::None) return Began;
		if (Candidate.Plan.Defender != Pending.Defender || Candidate.Plan.Armies.Num() != Pending.Armies.Num())
			return ECampaignEncounterFailure::StaleDeployment;
		TSet<FGuid> Seen;
		for (const FCampaignArmy& Snapshot : Pending.Armies)
		{
			const FCampaignArmy* Actual = Candidate.Plan.Armies.FindByPredicate(
				[&](const FCampaignArmy& Army) { return Army.ArmyID == Snapshot.ArmyID; });
			if (FCampaignArmyState::ValidateArmy(Snapshot) != ECampaignArmyFailure::None || !Actual ||
				Seen.Contains(Snapshot.ArmyID) || !SameArmy(Snapshot, *Actual)) return ECampaignEncounterFailure::StaleDeployment;
			Seen.Add(Snapshot.ArmyID);
		}
	}
	else
	{
		const FCampaignEncounterPlan Empty;
		if (!FCampaignEncounterPlan::StaticStruct()->CompareScriptStruct(&Pending, &Empty, 0)) return ECampaignEncounterFailure::InvalidResult;
	}
	*this = MoveTemp(Candidate);
	return ECampaignEncounterFailure::None;
}

void FCampaignEncounterState::Reset()
{
	Plan = FCampaignEncounterPlan();
	Bindings.Reset();
	AppliedEncounters.Reset();
	EnrolledReceipts.Reset();
}
