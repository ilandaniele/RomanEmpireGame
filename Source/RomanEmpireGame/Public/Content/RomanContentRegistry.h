// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "Content/RomanContentDefinitions.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RomanContentRegistry.generated.h"

UENUM(BlueprintType)
enum class EContentDefinitionFailure : uint8
{
	None				UMETA(DisplayName = "None"),
	MissingAssetType	UMETA(DisplayName = "Missing Asset Type"),
	MissingAsset		UMETA(DisplayName = "Missing Asset"),
	InvalidDefinition	UMETA(DisplayName = "Invalid Definition"),
	DuplicateStableID	UMETA(DisplayName = "Duplicate Stable ID"),
	DuplicateRuntimeID	UMETA(DisplayName = "Duplicate Runtime ID"),
	DuplicateQuickSlot	UMETA(DisplayName = "Duplicate Quick Slot"),
	MissingFactoryClass	UMETA(DisplayName = "Missing Factory Class"),
	InvalidFactionReference UMETA(DisplayName = "Invalid Faction Reference"),
	InvalidUnitReference	UMETA(DisplayName = "Invalid Unit Reference")
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FContentDefinitionIssue
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	EContentDefinitionFailure Failure = EContentDefinitionFailure::None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	FName AssetName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	FName DefinitionID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	FName ReferenceID = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	FString Details;
};

USTRUCT(BlueprintType)
struct ROMANEMPIREGAME_API FContentDefinitionValidationReport
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Content")
	TArray<FContentDefinitionIssue> Issues;

	bool IsValid() const { return Issues.IsEmpty(); }
	void Add(
		EContentDefinitionFailure Failure,
		const UObject* Asset,
		FName DefinitionID,
		FName ReferenceID,
		const FString& Details);
};

namespace RomanEmpireContent
{
	ROMANEMPIREGAME_API FContentDefinitionValidationReport ValidateDefinitionSet(
		const TArray<URomanFactionDefinition*>& Factions,
		const TArray<URomanUnitDefinition*>& Units,
		const TArray<URomanBuildingDefinition*>& Buildings,
		const TArray<URomanTerritoryDefinition*>& Territories);

	ROMANEMPIREGAME_API const TCHAR* GetDefinitionFailureName(EContentDefinitionFailure Failure);
}

/**
 * Loads and validates all Primary Data Asset definitions once at GameInstance startup.
 * Gameplay systems query stable IDs here and never rebuild immutable content locally.
 */
UCLASS()
class ROMANEMPIREGAME_API URomanContentRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	static URomanContentRegistrySubsystem* Get(const UObject* WorldContext);

	bool ReloadDefinitions();
	bool IsReady() const { return bIsReady; }
	const FContentDefinitionValidationReport& GetValidationReport() const { return ValidationReport; }

	const URomanFactionDefinition* FindFaction(FName FactionID) const;
	const URomanFactionDefinition* FindFaction(EFactionID RuntimeFaction) const;
	const URomanUnitDefinition* FindUnit(FName UnitID) const;
	const URomanBuildingDefinition* FindBuilding(FName BuildingID) const;
	const URomanBuildingDefinition* FindBuildingByQuickSlot(int32 QuickSlot) const;
	const URomanTerritoryDefinition* FindTerritory(FName TerritoryID) const;

	TArray<const URomanFactionDefinition*> GetAllFactions() const;
	TArray<const URomanUnitDefinition*> GetAllUnits() const;
	TArray<const URomanBuildingDefinition*> GetAllBuildings() const;
	TArray<const URomanTerritoryDefinition*> GetAllTerritories() const;

private:
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URomanFactionDefinition>> FactionsByID;

	UPROPERTY(Transient)
	TMap<EFactionID, TObjectPtr<URomanFactionDefinition>> FactionsByRuntimeID;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URomanUnitDefinition>> UnitsByID;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URomanBuildingDefinition>> BuildingsByID;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<URomanBuildingDefinition>> BuildingsByQuickSlot;

	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<URomanTerritoryDefinition>> TerritoriesByID;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UClass>> LoadedFactoryClasses;

	UPROPERTY(Transient)
	FContentDefinitionValidationReport ValidationReport;

	bool bIsReady = false;
	void ResetRegistry();
};
