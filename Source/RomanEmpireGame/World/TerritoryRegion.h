// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "TerritoryRegion.generated.h"

class ABuildingBase;
class AUnitBase;
class UBoxComponent;

/**
 * Terrain type affecting movement and building
 */
UENUM(BlueprintType)
enum class ETerrainType : uint8
{
	Plains		UMETA(DisplayName = "Plains"),
	Forest		UMETA(DisplayName = "Forest"),
	Mountain	UMETA(DisplayName = "Mountain"),
	Desert		UMETA(DisplayName = "Desert"),
	Coast		UMETA(DisplayName = "Coast")
};

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

	// Info
	UFUNCTION(BlueprintPure, Category = "Territory")
	FName GetTerritoryID() const { return TerritoryID; }

	UFUNCTION(BlueprintPure, Category = "Territory")
	FText GetDisplayName() const { return DisplayName; }

	UFUNCTION(BlueprintPure, Category = "Territory")
	ETerrainType GetTerrainType() const { return TerrainType; }

	// Ownership
	UFUNCTION(BlueprintPure, Category = "Territory")
	EFactionID GetOwnerFaction() const { return OwnerFaction; }

	UFUNCTION(BlueprintCallable, Category = "Territory")
	void SetOwnerFaction(EFactionID NewOwner);

	UFUNCTION(BlueprintPure, Category = "Territory")
	bool IsContested() const;

	// Resources
	UFUNCTION(BlueprintPure, Category = "Territory|Resources")
	FFactionResources GetResourceProduction() const { return ResourceProduction; }

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
	TArray<ABuildingBase*> GetBuildings() const { return Buildings; }

	UFUNCTION(BlueprintCallable, Category = "Territory|Buildings")
	void RegisterBuilding(ABuildingBase* Building);

	// Units in this territory
	UFUNCTION(BlueprintPure, Category = "Territory|Units")
	TArray<AUnitBase*> GetUnitsInTerritory() const;

	// Visual
	UFUNCTION(BlueprintCallable, Category = "Territory|Visual")
	void UpdateTerritoryColor();

protected:
	// Identification
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	FName TerritoryID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	ETerrainType TerrainType;

	// Ownership
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|State")
	EFactionID OwnerFaction;

	// Resources produced per turn
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Resources")
	FFactionResources ResourceProduction;

	// Bonus resources from terrain type
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Resources")
	EResourceType BonusResource;

	// Settlement
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	bool bHasSettlement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	FText SettlementName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	int32 Population;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Settlement")
	int32 MaxSettlementSlots; // How many buildings can be placed

	// Buildings
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Territory|Buildings")
	TArray<ABuildingBase*> Buildings;

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UBoxComponent* TerritoryBounds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* TerritoryMesh;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTerritoryOwnerChanged, ATerritoryRegion*, Territory, EFactionID, NewOwner);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSettlementFounded, ATerritoryRegion*, Territory);

	UPROPERTY(BlueprintAssignable, Category = "Territory|Events")
	FOnTerritoryOwnerChanged OnTerritoryOwnerChanged;

	UPROPERTY(BlueprintAssignable, Category = "Territory|Events")
	FOnSettlementFounded OnSettlementFounded;
};
