// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Campaign/TerritoryData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorldMapManager.generated.h"

class ATerritoryRegion;

/**
 * Manages the world map, territories, and strategic layer
 */
UCLASS()
class ROMANEMPIREGAME_API AWorldMapManager : public AActor
{
	GENERATED_BODY()

public:
	AWorldMapManager();

	virtual void BeginPlay() override;

	// Territory access
	UFUNCTION(BlueprintPure, Category = "World")
	TArray<ATerritoryRegion*> GetAllTerritories() const { return Territories; }

	UFUNCTION(BlueprintPure, Category = "World")
	ATerritoryRegion* GetTerritoryByID(FName TerritoryID) const;

	UFUNCTION(BlueprintPure, Category = "World")
	ATerritoryRegion* GetTerritoryAtLocation(const FVector& Location) const;

	UFUNCTION(BlueprintPure, Category = "World|Territory")
	FTerritoryDefinition GetTerritoryDefinition(FName TerritoryID) const;

	UFUNCTION(BlueprintPure, Category = "World|Territory")
	EFactionID GetTerritoryOwner(FName TerritoryID) const;

	UFUNCTION(BlueprintCallable, Category = "World|Territory")
	FTerritoryOwnershipChangeResult ChangeTerritoryOwner(
		FName TerritoryID,
		EFactionID NewOwner);

	UFUNCTION(BlueprintPure, Category = "World|Territory")
	int32 GetFactionTerritoryCount(EFactionID FactionID) const;

	// Map generation
	UFUNCTION(BlueprintCallable, Category = "World")
	void GenerateDefaultMap();

	// Adjacency
	UFUNCTION(BlueprintPure, Category = "World")
	TArray<ATerritoryRegion*> GetAdjacentTerritories(ATerritoryRegion* Territory) const;

	UFUNCTION(BlueprintPure, Category = "World")
	bool AreTerritoriesAdjacent(ATerritoryRegion* Territory1, ATerritoryRegion* Territory2) const;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
		FOnTerritoryOwnerChanged,
		FName, TerritoryID,
		EFactionID, PreviousOwner,
		EFactionID, NewOwner);

	UPROPERTY(BlueprintAssignable, Category = "World|Territory")
	FOnTerritoryOwnerChanged OnTerritoryOwnerChanged;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World")
	TArray<ATerritoryRegion*> Territories;

	// Map size
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World|Size")
	int32 MapWidth;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World|Size")
	int32 MapHeight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World|Size")
	float TerritorySize;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World|Campaign State")
	FTerritoryCampaignState CampaignTerritoryState;

private:
	UPROPERTY()
	TMap<FName, TObjectPtr<ATerritoryRegion>> TerritoryActorsByID;

	ATerritoryRegion* CreateTerritory(
		const FTerritoryDefinition& Definition,
		const FVector& Location);
	bool RegisterExistingTerritory(ATerritoryRegion* Territory);
};

