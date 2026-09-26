// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RomanUnitRegistry.generated.h"

class AActor;
class AUnitBase;

/** Live world index only. Unit actors own lifecycle/faction; this never keeps them alive. */
UCLASS()
class ROMANEMPIREGAME_API URomanUnitRegistrySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Deinitialize() override;

	/** Immediate snapshot; consumers must revalidate actors after any callback. No stable ordering. */
	TArray<AUnitBase*> GetUnitsForFaction(EFactionID Faction) const;
	bool ContainsUnit(const AActor* Actor) const;
	int32 GetRegisteredUnitCount() const { return RegisteredUnits.Num(); }

	/** Existing idle-acquisition rule: different non-None faction, strictly inside a 2D radius.
	 * Does not implement diplomacy, navigation or visibility. Equal distances use lexical actor names.
	 */
	AUnitBase* FindNearestEnemy(const FVector& Origin, EFactionID Faction, double Radius) const;

private:
	friend class AUnitBase;
	void SynchronizeUnit(AUnitBase* Unit);
	void RemoveUnit(AUnitBase* Unit);
	bool IsLiveUnit(const AUnitBase* Unit) const;

	// The recorded faction is index metadata, never a gameplay authority.
	TMap<TWeakObjectPtr<AUnitBase>, EFactionID> RegisteredUnits;
	TMap<EFactionID, TSet<TWeakObjectPtr<AUnitBase>>> UnitsByFaction;
	bool bDeinitializing = false;
};
