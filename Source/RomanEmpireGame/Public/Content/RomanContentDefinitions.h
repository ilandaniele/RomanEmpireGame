// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Building/BuildingTypes.h"
#include "Campaign/TerritoryData.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Units/UnitTypes.h"
#include "RomanContentDefinitions.generated.h"

class ABuildingBase;
class AUnitBase;
class UWorld;

namespace RomanEmpireContent
{
	ROMANEMPIREGAME_API extern const FPrimaryAssetType FactionAssetType;
	ROMANEMPIREGAME_API extern const FPrimaryAssetType UnitAssetType;
	ROMANEMPIREGAME_API extern const FPrimaryAssetType BuildingAssetType;
	ROMANEMPIREGAME_API extern const FPrimaryAssetType TerritoryAssetType;

	ROMANEMPIREGAME_API bool IsStableID(FName DefinitionID);
	ROMANEMPIREGAME_API bool IsRuntimeFaction(EFactionID FactionID);
	ROMANEMPIREGAME_API bool AreUnitStatsValid(const FUnitStats& Stats);
}

/** Immutable authored definition for one faction. Runtime resource wallets live elsewhere. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanFactionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Faction")
	FFactionInfo Definition;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** Immutable authored definition plus factory class for one unit archetype. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanUnitDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit")
	FUnitData Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Factory")
	TSoftClassPtr<AUnitBase> UnitClass;

	/** Stable faction IDs allowed to create this unit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Unit|Availability")
	TArray<FName> AvailableToFactions;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** Immutable authored definition plus factory/menu data for one building archetype. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanBuildingDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building")
	FBuildingData Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Factory")
	TSoftClassPtr<ABuildingBase> BuildingClass;

	/** Stable faction IDs allowed to construct this building. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|Availability")
	TArray<FName> AvailableToFactions;

	/** 1-based prototype menu/input slot. Zero keeps a definition out of the quick menu. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Building|UI", meta = (ClampMin = "0", ClampMax = "9"))
	int32 QuickSlot = 0;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

/** Immutable territory definition and deterministic prototype-map placement. */
UCLASS(BlueprintType)
class ROMANEMPIREGAME_API URomanTerritoryDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory")
	FTerritoryDefinition Definition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Prototype Map")
	FVector MapLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Prototype Map")
	FText StartingSettlementName;

	/** Destination for a future battle transition; content validation never loads this world. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Territory|Battle")
	TSoftObjectPtr<UWorld> TacticalMap;

	/** Requires an existing top-level World asset, checked through Asset Manager metadata. */
	bool HasValidTacticalMap() const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
