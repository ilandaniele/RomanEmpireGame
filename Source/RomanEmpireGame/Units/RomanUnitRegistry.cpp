// Copyright Roman Empire Game. All Rights Reserved.
#include "Units/RomanUnitRegistry.h"
#include "Engine/World.h"
#include "Units/UnitBase.h"

bool URomanUnitRegistrySubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void URomanUnitRegistrySubsystem::Deinitialize()
{
	bDeinitializing = true;
	SquadByUnit.Empty();
	Squads.Empty();
	RegisteredUnits.Empty();
	UnitsByFaction.Empty();
	OnUnitRegistryChanged.Clear();
	Super::Deinitialize();
}

bool URomanUnitRegistrySubsystem::IsLiveUnit(const AUnitBase* Unit) const
{
	return !bDeinitializing && IsValid(Unit) && Unit->GetWorld() == GetWorld() &&
		Unit->HasActorBegunPlay() && Unit->IsAlive() && !Unit->IsActorBeingDestroyed();
}

void URomanUnitRegistrySubsystem::SynchronizeUnit(AUnitBase* Unit)
{
	if (!IsLiveUnit(Unit))
	{
		RemoveUnit(Unit);
		return;
	}
	const EFactionID Faction = Unit->GetOwnerFaction();
	const TWeakObjectPtr<AUnitBase> Key(Unit);
	if (const EFactionID* IndexedFaction = RegisteredUnits.Find(Key))
	{
		if (*IndexedFaction == Faction) return;
		// Reindex atomically before notifying. No intermediate "removed" event
		// may re-enter SynchronizeUnit and leave the two buckets inconsistent.
		RemoveSquadMembership(Key);
		if (auto* Bucket = UnitsByFaction.Find(*IndexedFaction))
		{
			Bucket->Remove(Key);
			if (Bucket->IsEmpty()) UnitsByFaction.Remove(*IndexedFaction);
		}
	}
	RegisteredUnits.Add(Key, Faction);
	UnitsByFaction.FindOrAdd(Faction).Add(Key);
	OnUnitRegistryChanged.Broadcast();
}

void URomanUnitRegistrySubsystem::RemoveUnit(AUnitBase* Unit)
{
	if (!Unit) return;
	const TWeakObjectPtr<AUnitBase> Key(Unit);
	EFactionID Faction;
	RemoveSquadMembership(Key);
	if (!RegisteredUnits.RemoveAndCopyValue(Key, Faction)) return;
	if (TSet<TWeakObjectPtr<AUnitBase>>* Bucket = UnitsByFaction.Find(Faction))
	{
		Bucket->Remove(Key);
		if (Bucket->IsEmpty()) UnitsByFaction.Remove(Faction);
	}
	OnUnitRegistryChanged.Broadcast();
}

TArray<AUnitBase*> URomanUnitRegistrySubsystem::GetRegisteredUnits() const
{
	TArray<AUnitBase*> Result;
	for (const auto& Pair : RegisteredUnits)
	{
		AUnitBase* Unit = Pair.Key.Get();
		if (IsLiveUnit(Unit) && Unit->GetOwnerFaction() == Pair.Value) Result.Add(Unit);
	}
	return Result;
}

FGuid URomanUnitRegistrySubsystem::CreateSquad(const TArray<AUnitBase*>& Members)
{
	if (bDeinitializing || Members.IsEmpty()) return FGuid();
	FRuntimeSquad Candidate;
	TSet<TWeakObjectPtr<AUnitBase>> Seen;
	for (AUnitBase* Unit : Members)
	{
		if (!IsLiveUnit(Unit)) return FGuid();
		const TWeakObjectPtr<AUnitBase> Key(Unit);
		const EFactionID Faction = Unit->GetOwnerFaction();
		const EFactionID* IndexedFaction = RegisteredUnits.Find(Key);
		if (!IndexedFaction || *IndexedFaction != Faction || Faction == EFactionID::None ||
			SquadByUnit.Contains(Key)) return FGuid();
		if (Candidate.Faction != EFactionID::None && Candidate.Faction != Faction) return FGuid();
		Candidate.Faction = Faction;
		if (!Seen.Contains(Key))
		{
			Seen.Add(Key);
			Candidate.Members.Add(Key);
		}
	}
	FGuid Handle;
	do { Handle = FGuid::NewGuid(); } while (!Handle.IsValid() || Squads.Contains(Handle));
	for (const TWeakObjectPtr<AUnitBase>& Key : Candidate.Members) SquadByUnit.Add(Key, Handle);
	Squads.Add(Handle, MoveTemp(Candidate));
	return Handle;
}

bool URomanUnitRegistrySubsystem::DisbandSquad(FGuid Squad)
{
	if (bDeinitializing) return false;
	FRuntimeSquad Removed;
	if (!Squads.RemoveAndCopyValue(Squad, Removed)) return false;
	for (const TWeakObjectPtr<AUnitBase>& Key : Removed.Members)
	{
		const FGuid* IndexedSquad = SquadByUnit.Find(Key);
		if (IndexedSquad && *IndexedSquad == Squad) SquadByUnit.Remove(Key);
	}
	return true;
}

bool URomanUnitRegistrySubsystem::IsValidSquadMember(const TWeakObjectPtr<AUnitBase>& Key, FGuid Squad, EFactionID Faction) const
{
	const AUnitBase* Unit = Key.Get();
	const EFactionID* IndexedFaction = RegisteredUnits.Find(Key);
	const FGuid* IndexedSquad = SquadByUnit.Find(Key);
	return Faction != EFactionID::None && IsLiveUnit(Unit) && Unit->GetOwnerFaction() == Faction &&
		IndexedFaction && *IndexedFaction == Faction && IndexedSquad && *IndexedSquad == Squad;
}

FGuid URomanUnitRegistrySubsystem::GetUnitSquad(AUnitBase* Unit) const
{
	if (!IsLiveUnit(Unit)) return FGuid();
	const TWeakObjectPtr<AUnitBase> Key(Unit);
	const FGuid* Handle = SquadByUnit.Find(Key);
	const FRuntimeSquad* Squad = Handle ? Squads.Find(*Handle) : nullptr;
	return Squad && Squad->Members.Contains(Key) && IsValidSquadMember(Key, *Handle, Squad->Faction) ? *Handle : FGuid();
}

TArray<AUnitBase*> URomanUnitRegistrySubsystem::GetSquadMembers(FGuid Squad) const
{
	TArray<AUnitBase*> Result;
	if (bDeinitializing) return Result;
	if (const FRuntimeSquad* Group = Squads.Find(Squad))
	{
		Result.Reserve(Group->Members.Num());
		for (const TWeakObjectPtr<AUnitBase>& Key : Group->Members)
		{
			if (IsValidSquadMember(Key, Squad, Group->Faction)) Result.Add(Key.Get());
		}
	}
	return Result;
}

EFactionID URomanUnitRegistrySubsystem::GetSquadFaction(FGuid Squad) const
{
	const TArray<AUnitBase*> Members = GetSquadMembers(Squad);
	return Members.IsEmpty() ? EFactionID::None : Members[0]->GetOwnerFaction();
}

int32 URomanUnitRegistrySubsystem::GetSquadCount() const
{
	if (bDeinitializing) return 0;
	int32 Count = 0;
	for (const auto& Pair : Squads)
	{
		for (const TWeakObjectPtr<AUnitBase>& Key : Pair.Value.Members)
		{
			if (IsValidSquadMember(Key, Pair.Key, Pair.Value.Faction))
			{
				++Count;
				break;
			}
		}
	}
	return Count;
}

void URomanUnitRegistrySubsystem::RemoveSquadMembership(const TWeakObjectPtr<AUnitBase>& Unit)
{
	FGuid Handle;
	if (!SquadByUnit.RemoveAndCopyValue(Unit, Handle)) return;
	FRuntimeSquad* Squad = Squads.Find(Handle);
	if (!Squad) return;
	for (int32 Index = Squad->Members.Num() - 1; Index >= 0; --Index)
	{
		const TWeakObjectPtr<AUnitBase> Key = Squad->Members[Index];
		if (Key == Unit || !IsValidSquadMember(Key, Handle, Squad->Faction))
		{
			const FGuid* IndexedSquad = SquadByUnit.Find(Key);
			if (IndexedSquad && *IndexedSquad == Handle) SquadByUnit.Remove(Key);
			Squad->Members.RemoveAt(Index);
		}
	}
	if (Squad->Members.IsEmpty()) Squads.Remove(Handle);
}

bool URomanUnitRegistrySubsystem::ContainsUnit(const AActor* Actor) const
{
	const AUnitBase* Unit = Cast<AUnitBase>(Actor);
	return IsLiveUnit(Unit) && RegisteredUnits.Contains(TWeakObjectPtr<AUnitBase>(const_cast<AUnitBase*>(Unit)));
}

TArray<AUnitBase*> URomanUnitRegistrySubsystem::GetUnitsForFaction(EFactionID Faction) const
{
	TArray<AUnitBase*> Result;
	if (bDeinitializing) return Result;
	if (const TSet<TWeakObjectPtr<AUnitBase>>* Bucket = UnitsByFaction.Find(Faction))
	{
		Result.Reserve(Bucket->Num());
		for (const TWeakObjectPtr<AUnitBase>& Key : *Bucket)
		{
			AUnitBase* Unit = Key.Get();
			if (IsLiveUnit(Unit) && Unit->GetOwnerFaction() == Faction) Result.Add(Unit);
		}
	}
	return Result;
}

AUnitBase* URomanUnitRegistrySubsystem::FindNearestEnemy(const FVector& Origin, EFactionID Faction, double Radius) const
{
	const double RadiusSquared = Radius * Radius;
	if (bDeinitializing || Origin.ContainsNaN() || !FMath::IsFinite(Radius) || Radius <= 0.0 || !FMath::IsFinite(RadiusSquared)) return nullptr;
	AUnitBase* Nearest = nullptr;
	double BestDistanceSquared = RadiusSquared;
	for (const auto& Pair : UnitsByFaction)
	{
		if (Pair.Key == Faction || Pair.Key == EFactionID::None) continue;
		for (const TWeakObjectPtr<AUnitBase>& Key : Pair.Value)
		{
			AUnitBase* Unit = Key.Get();
			if (!IsLiveUnit(Unit) || Unit->GetOwnerFaction() != Pair.Key || !Unit->CanParticipateInWorld()) continue;
			const double DistanceSquared = FVector::DistSquared2D(Origin, Unit->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared || (Nearest && DistanceSquared == BestDistanceSquared &&
				Unit->GetFName().LexicalLess(Nearest->GetFName())))
			{
				BestDistanceSquared = DistanceSquared;
				Nearest = Unit;
			}
		}
	}
	return Nearest;
}
