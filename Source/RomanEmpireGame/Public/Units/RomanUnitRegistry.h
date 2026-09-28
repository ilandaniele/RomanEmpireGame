// Copyright Roman Empire Game. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RomanUnitRegistry.generated.h"

class AActor;
class AUnitBase;

/** World-scoped live unit index and tactical membership. Actors own lifecycle/faction;
 * weak indexes never keep them alive. Squad handles are not campaign/save identities.
 */
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

	/** Runtime world/session-local handle only; never a serialized campaign identity.
	 * Rejects the entire request on invalid membership. Duplicate inputs preserve first occurrence order.
	 */
	UFUNCTION(BlueprintCallable, Category = "Units|Squads")
	FGuid CreateSquad(const TArray<AUnitBase*>& Members);

	/** Removes membership only; does not change actor ownership, orders or selection. */
	UFUNCTION(BlueprintCallable, Category = "Units|Squads")
	bool DisbandSquad(FGuid SquadID);

	UFUNCTION(BlueprintPure, Category = "Units|Squads")
	FGuid GetUnitSquad(AUnitBase* Unit) const;

	/** Immediate validated snapshot in creation order; revalidate after callbacks. */
	UFUNCTION(BlueprintPure, Category = "Units|Squads")
	TArray<AUnitBase*> GetSquadMembers(FGuid SquadID) const;

	UFUNCTION(BlueprintPure, Category = "Units|Squads")
	EFactionID GetSquadFaction(FGuid SquadID) const;

	UFUNCTION(BlueprintPure, Category = "Units|Squads")
	int32 GetSquadCount() const;

	/** Existing idle-acquisition rule: different non-None faction, strictly inside a 2D radius.
	 * Does not implement diplomacy, navigation or visibility. Equal distances use lexical actor names.
	 */
	AUnitBase* FindNearestEnemy(const FVector& Origin, EFactionID Faction, double Radius) const;

private:
	friend class AUnitBase;
	void SynchronizeUnit(AUnitBase* Unit);
	void RemoveUnit(AUnitBase* Unit);
	bool IsLiveUnit(const AUnitBase* Unit) const;
	void RemoveSquadMembership(const TWeakObjectPtr<AUnitBase>& Unit);
	bool IsValidSquadMember(const TWeakObjectPtr<AUnitBase>& Unit, FGuid Squad, EFactionID Faction) const;

	struct FRuntimeSquad
	{
		// Grouping constraint only; each read validates against canonical actor ownership.
		EFactionID Faction = EFactionID::None;
		TArray<TWeakObjectPtr<AUnitBase>> Members;
	};
	// Unreflected runtime indexes hold weak references only and are cleared on world teardown.
	TMap<FGuid, FRuntimeSquad> Squads;
	TMap<TWeakObjectPtr<AUnitBase>, FGuid> SquadByUnit;

	// The recorded faction is index metadata, never a gameplay authority.
	TMap<TWeakObjectPtr<AUnitBase>, EFactionID> RegisteredUnits;
	TMap<EFactionID, TSet<TWeakObjectPtr<AUnitBase>>> UnitsByFaction;
	bool bDeinitializing = false;
};
