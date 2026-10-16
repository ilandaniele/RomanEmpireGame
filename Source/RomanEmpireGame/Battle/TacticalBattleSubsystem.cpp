// Copyright Roman Empire Game. All Rights Reserved.

#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/UnitBase.h"
#include "Engine/World.h"
#include "Core/RomanEmpireGameMode.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

bool UTacticalBattleSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTacticalBattleSubsystem::Deinitialize()
{
	bDeinitializing = true;
	GetWorld()->GetTimerManager().ClearTimer(AIAttackTimer);
	LastAIAttackTargets.Reset(); OccupationAttempted.Reset();
	for (const TPair<TWeakObjectPtr<AUnitBase>, FGuid>& Entry : ActiveRoster)
	{
		if (AUnitBase* Unit = Entry.Key.Get())
		{
			Unit->OnUnitDied.RemoveDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
		}
	}
	ActiveRoster.Reset();
	ObjectiveActor.Reset();
	Battle.Reset();
	bCampaignDeployment = false;
	Super::Deinitialize();
}

void UTacticalBattleSubsystem::TrackUnit(AUnitBase* Unit, FGuid SquadID)
{
	ActiveRoster.Add(Unit, SquadID);
	Unit->OnUnitDied.AddUniqueDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
}

bool UTacticalBattleSubsystem::BeginEncounter(FName ObjectiveID, FGuid PlayerSquad, FGuid EnemySquad, FGuid EncounterID)
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (!Registry) return false;
	return BeginFromSquads(ObjectiveID, EFactionID::Rome, EFactionID::Carthage,
		{ { PlayerSquad, EFactionID::Rome, Registry->GetSquadMembers(PlayerSquad).Num() },
		  { EnemySquad, EFactionID::Carthage, Registry->GetSquadMembers(EnemySquad).Num() } }, EncounterID);
}

bool UTacticalBattleSubsystem::BeginCampaignDeployment(const FCampaignEncounterPlan& Plan,
	const TArray<FCampaignSquadBinding>& Bindings)
{
	if (!Plan.EncounterID.IsValid() || !ObjectiveActor.IsValid() || ObjectiveActor->GetObjectiveID() != Plan.ObjectiveID) return false;
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (!Registry) return false;
	TArray<FTacticalSquadSeed> Seeds;
	for (const FCampaignSquadBinding& Binding : Bindings)
	{
		const FCampaignArmy* Army = Plan.Armies.FindByPredicate([&](const FCampaignArmy& Value) { return Value.ArmyID == Binding.ArmyID; });
		if (!Army) return false;
		for (AUnitBase* Unit : Registry->GetSquadMembers(Binding.SquadID))
			if (!IsValid(Unit) || Unit->GetUnitDefinitionID() != Binding.UnitID) return false;
		Seeds.Add({ Binding.SquadID, Army->Faction, Binding.UnitCount });
	}
	if (Plan.PlayerFaction != Plan.Attacker && Plan.PlayerFaction != Plan.Defender) return false;
	const auto Enemy = Plan.PlayerFaction == Plan.Attacker ? Plan.Defender : Plan.Attacker;
	if (!BeginFromSquads(Plan.ObjectiveID, Plan.PlayerFaction, Enemy, Seeds, Plan.EncounterID, true, Plan.Attacker)) return false;
	if (Plan.Attacker != Plan.PlayerFaction)
		GetWorld()->GetTimerManager().SetTimer(AIAttackTimer, this, &UTacticalBattleSubsystem::DriveAIAttackers, 0.5f, true);
	return true;
}

bool UTacticalBattleSubsystem::BeginFromSquads(FName ObjectiveID, EFactionID PlayerFaction, EFactionID EnemyFaction,
	const TArray<FTacticalSquadSeed>& Seeds, FGuid EncounterID, bool bCampaign, EFactionID Attacker)
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (bDeinitializing || !Registry || Battle.HasEncounter()) return false;
	if (ObjectiveActor.IsValid() && ObjectiveActor->GetObjectiveID() != ObjectiveID) return false;
	TSet<AUnitBase*> Seen;
	for (const FTacticalSquadSeed& Seed : Seeds)
	{
		const TArray<AUnitBase*> Units = Registry->GetSquadMembers(Seed.SquadID);
		if (Units.IsEmpty() || Units.Num() != Seed.UnitCount || Registry->GetSquadFaction(Seed.SquadID) != Seed.Faction) return false;
		for (AUnitBase* Unit : Units)
		{
			if (!IsValid(Unit) || Unit->GetWorld() != World || !Unit->IsAlive() || Unit->GetOwnerFaction() != Seed.Faction || Seen.Contains(Unit)) return false;
			Seen.Add(Unit);
		}
	}
	if (!Battle.BeginEncounter(ObjectiveID, PlayerFaction, EnemyFaction, Seeds, EncounterID, Attacker)) return false;
	bCampaignDeployment = bCampaign;
	for (const FTacticalSquadSeed& Seed : Seeds)
		for (AUnitBase* Unit : Registry->GetSquadMembers(Seed.SquadID)) TrackUnit(Unit, Seed.SquadID);
	BroadcastResult();
	return true;
}

bool UTacticalBattleSubsystem::RegisterReinforcement(AUnitBase* Unit)
{
	if (bCampaignDeployment) return false; // Paid campaign enrollment is private to its authority.
	FTacticalBattleState Prepared;
	FGuid SquadID;
	if (!PrepareReinforcement(Unit, Prepared, SquadID)) return false;
	CommitReinforcement(Unit, SquadID, MoveTemp(Prepared));
	BroadcastResult();
	return true;
}

bool UTacticalBattleSubsystem::PrepareReinforcement(AUnitBase* Unit, FTacticalBattleState& OutBattle, FGuid& OutSquad) const
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (bDeinitializing || !Battle.HasEncounter() || !Registry || !IsValid(Unit) ||
		Unit->GetWorld() != World || !Unit->IsAlive() || Unit->IsActorBeingDestroyed() || ActiveRoster.Contains(Unit)) return false;
	const FGuid SquadID = Registry->GetUnitSquad(Unit);
	const TArray<AUnitBase*> Members = Registry->GetSquadMembers(SquadID);
	if (!SquadID.IsValid() || Members.Num() != 1 || Members[0] != Unit ||
		Registry->GetSquadFaction(SquadID) != Unit->GetOwnerFaction()) return false;
	OutBattle = Battle;
	if (!OutBattle.RegisterReinforcement({ SquadID, Unit->GetOwnerFaction(), 1 })) return false;
	OutSquad = SquadID;
	return true;
}

void UTacticalBattleSubsystem::CommitReinforcement(AUnitBase* Unit, FGuid SquadID, FTacticalBattleState&& Prepared)
{
	Battle = MoveTemp(Prepared);
	TrackUnit(Unit, SquadID);
}

void UTacticalBattleSubsystem::HandleUnitDied(AUnitBase* Unit)
{
	if (bDeinitializing || !Unit) return;
	const FGuid* SquadID = ActiveRoster.Find(Unit);
	if (!SquadID) return;
	const FGuid RetiredSquad = *SquadID;
	ActiveRoster.Remove(Unit);
	LastAIAttackTargets.Remove(Unit); OccupationAttempted.Remove(Unit);
	Unit->OnUnitDied.RemoveDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
	if (Battle.RecordCasualty(RetiredSquad)) BroadcastResult();
}

ATacticalObjective* UTacticalBattleSubsystem::GetObjectiveActor() const
{
	return ObjectiveActor.Get();
}

bool UTacticalBattleSubsystem::RegisterObjective(ATacticalObjective* Objective)
{
	if (bDeinitializing || ObjectiveActor.IsValid() || !IsValid(Objective) ||
		Objective->GetWorld() != GetWorld() || Objective->IsActorBeingDestroyed() || Objective->GetObjectiveID().IsNone() ||
		(Battle.HasEncounter() && Objective->GetObjectiveID() != Battle.GetResult().ObjectiveID) || Battle.IsResolved()) return false;
	ObjectiveActor = Objective;
	return true;
}

bool UTacticalBattleSubsystem::CaptureObjective(ATacticalObjective* Objective, AUnitBase* Occupant)
{
	if (bDeinitializing || ObjectiveActor.Get() != Objective || !IsValid(Objective) ||
		Objective->GetWorld() != GetWorld() || Objective->IsActorBeingDestroyed() || !IsValid(Occupant) ||
		Occupant->GetWorld() != GetWorld() || !Occupant->CanParticipateInWorld() ||
		Battle.GetResult().ObjectiveID != Objective->GetObjectiveID() ||
		!Battle.CaptureObjective(Occupant->GetOwnerFaction())) return false;
	BroadcastResult();
	return true;
}

void UTacticalBattleSubsystem::DriveAIAttackers()
{
	UWorld* World = GetWorld();
	const auto* Mode = World ? World->GetAuthGameMode<ARomanEmpireGameMode>() : nullptr;
	if (bDeinitializing || Battle.IsResolved() || !bCampaignDeployment || !Mode || !Mode->IsGameplayInitialized() ||
		UNavigationSystemV1::IsNavigationBeingBuiltOrLocked(World)) return;
	TArray<AUnitBase*> Attackers, Defenders;
	for (const auto& Entry : ActiveRoster)
		if (auto* Unit = Entry.Key.Get(); Unit && Unit->CanParticipateInWorld())
		{
			if (Unit->GetOwnerFaction() == Battle.GetAttackingFaction()) Attackers.Add(Unit);
			else if (Unit->GetOwnerFaction() == Battle.GetDefendingFaction()) Defenders.Add(Unit);
		}
	for (auto* Unit : Attackers)
	{
		if (Defenders.IsEmpty())
		{
			LastAIAttackTargets.Remove(Unit);
			if (!ObjectiveActor.IsValid() || Unit->GetOrderType() == EUnitOrderType::Move ||
				(OccupationAttempted.Contains(Unit) && Unit->GetNavigationResult() == EUnitNavigationResult::Unreachable)) continue;
			OccupationAttempted.Add(Unit); Unit->CommandMoveTo(ObjectiveActor->GetOccupationApproachLocation()); continue;
		}
		OccupationAttempted.Remove(Unit);
		AUnitBase* Target = Cast<AUnitBase>(Unit->GetAttackTarget());
		if (!Defenders.Contains(Target))
		{
			Target = nullptr;
			for (auto* Candidate : Defenders)
				if (!Target || FVector::DistSquared(Unit->GetActorLocation(), Candidate->GetActorLocation()) < FVector::DistSquared(Unit->GetActorLocation(), Target->GetActorLocation()) ||
					(FVector::DistSquared(Unit->GetActorLocation(), Candidate->GetActorLocation()) == FVector::DistSquared(Unit->GetActorLocation(), Target->GetActorLocation()) && Candidate->GetUniqueID() < Target->GetUniqueID())) Target = Candidate;
		}
		if (Unit->GetOrderType() == EUnitOrderType::Attack && Unit->GetAttackTarget() == Target) continue;
		if (Unit->GetNavigationResult() == EUnitNavigationResult::Unreachable && LastAIAttackTargets.FindRef(Unit).Get() == Target) continue;
		LastAIAttackTargets.Add(Unit, Target); Unit->CommandAttack(Target);
	}
}

void UTacticalBattleSubsystem::BroadcastResult()
{
	if (Battle.IsResolved())
	{
		GetWorld()->GetTimerManager().ClearTimer(AIAttackTimer);
		LastAIAttackTargets.Reset(); OccupationAttempted.Reset();
		// Cleanup publishes actor events: iterate a weak snapshot, not the mutable roster.
		TArray<TWeakObjectPtr<AUnitBase>> Survivors;
		ActiveRoster.GenerateKeyArray(Survivors);
		for (const TWeakObjectPtr<AUnitBase>& Survivor : Survivors)
		{
			if (AUnitBase* Unit = Survivor.Get(); Unit && Unit->CanParticipateInWorld())
				Unit->StopForBattleResolution();
		}
	}
	OnTacticalBattleChanged.Broadcast(Battle.GetResult());
}
