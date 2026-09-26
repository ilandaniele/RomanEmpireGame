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
	RegisteredUnits.Empty();
	UnitsByFaction.Empty();
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
		RemoveUnit(Unit);
	}
	RegisteredUnits.Add(Key, Faction);
	UnitsByFaction.FindOrAdd(Faction).Add(Key);
}

void URomanUnitRegistrySubsystem::RemoveUnit(AUnitBase* Unit)
{
	if (!Unit) return;
	const TWeakObjectPtr<AUnitBase> Key(Unit);
	EFactionID Faction;
	if (!RegisteredUnits.RemoveAndCopyValue(Key, Faction)) return;
	if (TSet<TWeakObjectPtr<AUnitBase>>* Bucket = UnitsByFaction.Find(Faction))
	{
		Bucket->Remove(Key);
		if (Bucket->IsEmpty()) UnitsByFaction.Remove(Faction);
	}
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
			if (!IsLiveUnit(Unit) || Unit->GetOwnerFaction() != Pair.Key || !Unit->CanAcceptCommands()) continue;
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
