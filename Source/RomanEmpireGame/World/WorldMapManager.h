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

	/** Registered canonical IDs; no dependency on actor validity or placement. */
	UFUNCTION(BlueprintPure, Category = "World|Territory")
	TArray<FName> GetTerritoryIDs() const;

	UFUNCTION(BlueprintCallable, Category = "World|Settlement")
	FSettlementFoundingResult FoundTerritorySettlement(FName TerritoryID, const FText& Name);
	UFUNCTION(BlueprintPure, Category = "World|Settlement")
	bool TryGetTerritorySettlement(FName TerritoryID, FTerritorySettlementState& OutSettlement) const;
	UFUNCTION(BlueprintPure, Category = "World|Territory")
	bool TryGetTerritoryProduction(FName TerritoryID, FFactionResourceDelta& OutProduction) const;
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTerritorySettlementChanged, FName, TerritoryID);
	UPROPERTY(BlueprintAssignable, Category = "World|Settlement")
	FOnTerritorySettlementChanged OnTerritorySettlementChanged;

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
	UFUNCTION(BlueprintPure, Category = "World|Territory")
	TArray<FName> GetAdjacentTerritoryIDs(FName TerritoryID) const;

	UFUNCTION(BlueprintPure, Category = "World|Territory")
	bool AreTerritoryIDsAdjacent(FName FirstID, FName SecondID) const;

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
	friend class ACampaignManager;
	// Only the campaign coordinator may stage a paired army/territory transaction.
	bool bCampaignTransactionActive = false;
	void PublishOwnershipChange(const FTerritoryOwnershipChangeResult& Result);
	UPROPERTY()
	TMap<FName, TObjectPtr<ATerritoryRegion>> TerritoryActorsByID;

	ATerritoryRegion* CreateTerritory(
		const FTerritoryDefinition& Definition,
		const FVector& Location);
	bool RegisterExistingTerritory(ATerritoryRegion* Territory);
};

