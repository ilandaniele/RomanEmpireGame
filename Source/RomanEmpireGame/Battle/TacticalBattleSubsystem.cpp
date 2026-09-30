// Copyright Roman Empire Game. All Rights Reserved.

#include "Battle/TacticalBattleSubsystem.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/UnitBase.h"
#include "Engine/World.h"

bool UTacticalBattleSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UTacticalBattleSubsystem::Deinitialize()
{
	bDeinitializing = true;
	for (const TPair<TWeakObjectPtr<AUnitBase>, FGuid>& Entry : ActiveRoster)
	{
		if (AUnitBase* Unit = Entry.Key.Get())
		{
			Unit->OnUnitDied.RemoveDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
		}
	}
	ActiveRoster.Reset();
	Battle.Reset();
	Super::Deinitialize();
}

void UTacticalBattleSubsystem::TrackUnit(AUnitBase* Unit, FGuid SquadID)
{
	ActiveRoster.Add(Unit, SquadID);
	Unit->OnUnitDied.AddUniqueDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
}

bool UTacticalBattleSubsystem::BeginEncounter(FName ObjectiveID, FGuid PlayerSquad, FGuid EnemySquad)
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (bDeinitializing || !Registry || Battle.HasEncounter() || !PlayerSquad.IsValid() ||
		!EnemySquad.IsValid() || PlayerSquad == EnemySquad) return false;
	const TArray<AUnitBase*> Players = Registry->GetSquadMembers(PlayerSquad);
	const TArray<AUnitBase*> Enemies = Registry->GetSquadMembers(EnemySquad);
	if (Players.IsEmpty() || Enemies.IsEmpty() || Registry->GetSquadFaction(PlayerSquad) != EFactionID::Rome ||
		Registry->GetSquadFaction(EnemySquad) != EFactionID::Carthage) return false;
	for (AUnitBase* Unit : Players)
	{
		if (!IsValid(Unit) || Unit->GetWorld() != World || !Unit->IsAlive() ||
			Unit->GetOwnerFaction() != EFactionID::Rome) return false;
	}
	for (AUnitBase* Unit : Enemies)
	{
		if (!IsValid(Unit) || Unit->GetWorld() != World || !Unit->IsAlive() ||
			Unit->GetOwnerFaction() != EFactionID::Carthage) return false;
	}
	if (!Battle.BeginEncounter(ObjectiveID, EFactionID::Rome, EFactionID::Carthage,
		{ { PlayerSquad, EFactionID::Rome, Players.Num() },
		  { EnemySquad, EFactionID::Carthage, Enemies.Num() } })) return false;
	for (AUnitBase* Unit : Players) TrackUnit(Unit, PlayerSquad);
	for (AUnitBase* Unit : Enemies) TrackUnit(Unit, EnemySquad);
	BroadcastResult();
	return true;
}

bool UTacticalBattleSubsystem::RegisterReinforcement(AUnitBase* Unit)
{
	UWorld* World = GetWorld();
	URomanUnitRegistrySubsystem* Registry = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (bDeinitializing || !Battle.HasEncounter() || !Registry || !IsValid(Unit) ||
		Unit->GetWorld() != World || !Unit->IsAlive() || ActiveRoster.Contains(Unit)) return false;
	const FGuid SquadID = Registry->GetUnitSquad(Unit);
	if (!SquadID.IsValid() || Registry->GetSquadMembers(SquadID).Num() != 1) return false;
	if (!Battle.RegisterReinforcement({ SquadID, Unit->GetOwnerFaction(), 1 })) return false;
	TrackUnit(Unit, SquadID);
	BroadcastResult();
	return true;
}

void UTacticalBattleSubsystem::HandleUnitDied(AUnitBase* Unit)
{
	if (bDeinitializing || !Unit) return;
	const FGuid* SquadID = ActiveRoster.Find(Unit);
	if (!SquadID) return;
	const FGuid RetiredSquad = *SquadID;
	ActiveRoster.Remove(Unit);
	Unit->OnUnitDied.RemoveDynamic(this, &UTacticalBattleSubsystem::HandleUnitDied);
	if (Battle.RecordCasualty(RetiredSquad)) BroadcastResult();
}

bool UTacticalBattleSubsystem::CaptureObjective(FName ObjectiveID, EFactionID CapturingFaction)
{
	if (bDeinitializing || ObjectiveID.IsNone() || Battle.GetResult().ObjectiveID != ObjectiveID ||
		!Battle.CaptureObjective(CapturingFaction)) return false;
	BroadcastResult();
	return true;
}

void UTacticalBattleSubsystem::BroadcastResult()
{
	OnTacticalBattleChanged.Broadcast(Battle.GetResult());
}
