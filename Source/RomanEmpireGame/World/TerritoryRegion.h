// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Campaign/TerritoryData.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerritoryRegion.generated.h"

class ABuildingBase;
class AUnitBase;
class AWorldMapManager;
class UBoxComponent;
class UStaticMeshComponent;

/**
 * A conquerable territory/region on the world map
 * Contains settlements, resources, and can be owned by factions
 */
UCLASS()
class ROMANEMPIREGAME_API ATerritoryRegion : public AActor
{
	GENERATED_BODY()

public:
	ATerritoryRegion();

	virtual void BeginPlay() override;

	/** Called by the world-map factory before a deferred actor becomes observable. */
	bool InitializeTerritory(
		const FTerritoryDefinition& Definition,
		AWorldMapManager* InWorldMapManager);

	/** Presentation callback used only after the canonical campaign state changes. */
	void RefreshOwnershipPresentation(EFactionID PreviousOwner, EFactionID NewOwner);

	// Info
	UFUNCTION(BlueprintPure, Category = "Territory")
	FName GetTerritoryID() const { return TerritoryDefinition.TerritoryID; }

	UFUNCTION(BlueprintPure, Category = "Territory")
	FText GetDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	ETerrainType GetTerrainType() const;

	FTerritoryDefinition GetDefinition() const;

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Territory")
	EFactionID GetOwnerFaction() const;

	UFUNCTION(BlueprintPure, Category = "Territory")
	bool IsContested() const;

	// Resources
	UFUNCTION(BlueprintPure, Category = "Territory|Resources")
	FFactionResourceDelta GetResourceProduction() const;

	// Settlement
	UFUNCTION(BlueprintPure, Category = "Territory|Settlement")
	bool HasSettlement() const { return bHasSettlement; }

	UFUNCTION(BlueprintCallable, Category = "Territory|Settlement")
	void FoundSettlement(const FText& SettlementName);

	UFUNCTION(BlueprintPure, Category = "Territory|Settlement")
	FText GetSettlementName() const { return SettlementName; }

	UFUNCTION(BlueprintPure, Category = "Territory|Settlement")
	int32 GetPopulation() const { return Population; }

	// Buildings in this territory
	UFUNCTION(BlueprintPure, Category = "Territory|Buildings")
	TArray<ABuildingBase*> GetBuildings() const;

	UFUNCTION(BlueprintCallable, Category = "Territory|Buildings")
	void RegisterBuilding(ABuildingBase* Building);

	// Units in this territory
	UFUNCTION(BlueprintPure, Category = "Territory|Units")
	TArray<AUnitBase*> GetUnitsInTerritory() const;

	// Visual
	UFUNCTION(BlueprintCallable, Category = "Territory|Visual")
	void UpdateTerritoryColor();

protected:
	// Authored definition; runtime factories assign this before FinishSpawningActor.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	FTerritoryDefinition TerritoryDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|State")
	TObjectPtr<AWorldMapManager> WorldMapManager;

	// Settlement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	bool bHasSettlement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	FText SettlementName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	int32 Population;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	FFactionResourceDelta SettlementProductionBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	int32 MaxSettlementSlots; // How many buildings can be placed

	// Buildings
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Buildings")
	TArray<TObjectPtr<ABuildingBase>> Buildings;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> TerritoryBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> TerritoryMesh;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerritoryOwnerChanged, ATerritoryRegion*, Territory, EFactionID, NewOwner);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettlementFounded, ATerritoryRegion*, Territory);

	UPROPERTY(BlueprintAssignable, Category = "Territory|Events")
	FOnTerritoryOwnerChanged OnTerritoryOwnerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Territory|Events")
	FOnSettlementFounded OnSettlementFounded;

private:
	bool bTerritoryInitialized = false;
};

