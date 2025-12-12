// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RomanEmpireGame/Faction/FactionData.h"
#include "MaterialManager.generated.h"

class UMaterialParameterCollection;
class UMaterialInstanceDynamic;

/**
 * Material type identifier
 */
UENUM(BlueprintType)
enum class EMaterialType : uint8
{
	// Buildings
	BuildingStone,
	BuildingWood,
	BuildingRoof,
	BuildingMarble,
	
	// Units
	UnitArmor,
	UnitCloth,
	UnitSkin,
	UnitLeather,
	
	// Terrain
	TerrainGrass,
	TerrainDirt,
	TerrainSand,
	TerrainRock,
	TerrainWater,
	
	// Effects
	GhostValid,
	GhostInvalid,
	Selection,
	FactionHighlight
};

/**
 * Manages materials and visual styles
 * Handles faction colors, building states, and terrain materials
 */
UCLASS()
class ROMANEMPIREGAME_API AMaterialManager : public AActor
{
	GENERATED_BODY()

public:
	AMaterialManager();

	virtual void BeginPlay() override;

	// Create faction-colored material instance
	UFUNCTION(BlueprintCallable, Category = "Materials")
	UMaterialInstanceDynamic* CreateFactionMaterial(UMaterialInterface* BaseMaterial, EFactionID Faction);

	// Get material by type
	UFUNCTION(BlueprintPure, Category = "Materials")
	UMaterialInterface* GetMaterial(EMaterialType MaterialType) const;

	// Get faction color
	UFUNCTION(BlueprintPure, Category = "Materials")
	FLinearColor GetFactionColor(EFactionID Faction) const;

	// Update global material parameters
	UFUNCTION(BlueprintCallable, Category = "Materials")
	void SetGlobalFactionColor(EFactionID Faction);

	// Create ghost/placement materials
	UFUNCTION(BlueprintCallable, Category = "Materials")
	UMaterialInstanceDynamic* CreateGhostMaterial(bool bValid);

	// Singleton access
	UFUNCTION(BlueprintPure, Category = "Materials", meta = (WorldContext = "WorldContextObject"))
	static AMaterialManager* GetMaterialManager(UObject* WorldContextObject);

protected:
	// Base materials
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials|Base")
	TMap<EMaterialType, UMaterialInterface*> BaseMaterials;

	// Material Parameter Collection for global shader parameters
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials|Parameters")
	UMaterialParameterCollection* GlobalParameterCollection;

	// Faction colors
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials|Factions")
	TMap<EFactionID, FLinearColor> FactionColors;

	// Ghost materials
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials|Ghost")
	UMaterialInterface* GhostValidBaseMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Materials|Ghost")
	UMaterialInterface* GhostInvalidBaseMaterial;

private:
	void InitializeDefaultColors();
	void InitializeDefaultMaterials();
};
