// Copyright Roman Empire Game. All Rights Reserved.

#include "Content/RomanContentRegistry.h"

#include "Building/BuildingBase.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "RomanEmpireGame.h"
#include "Units/UnitBase.h"

namespace
{
	template <typename TDefinition>
	void LoadPrimaryDefinitions(
		UAssetManager& AssetManager,
		const FPrimaryAssetType& AssetType,
		TArray<TDefinition*>& OutDefinitions,
		FContentDefinitionValidationReport& OutReport)
	{
		TArray<FPrimaryAssetId> AssetIDs;
		AssetManager.GetPrimaryAssetIdList(AssetType, AssetIDs);
		AssetIDs.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right)
		{
			return Left.ToString() < Right.ToString();
		});

		for (const FPrimaryAssetId& AssetID : AssetIDs)
		{
			UObject* LoadedObject = AssetManager.GetPrimaryAssetObject(AssetID);
			if (!LoadedObject)
			{
				const FSoftObjectPath AssetPath = AssetManager.GetPrimaryAssetPath(AssetID);
				LoadedObject = AssetPath.TryLoad();
			}

			if (TDefinition* Definition = Cast<TDefinition>(LoadedObject))
			{
				OutDefinitions.Add(Definition);
			}
			else
			{
				OutReport.Add(
					EContentDefinitionFailure::MissingAsset,
					nullptr,
					NAME_None,
					AssetID.PrimaryAssetName,
					FString::Printf(TEXT("Primary asset %s could not be loaded."), *AssetID.ToString()));
			}
		}
	}

	template <typename TValue>
	bool AddUniqueOrReport(
		TSet<TValue>& Seen,
		const TValue& Value,
		FContentDefinitionValidationReport& Report,
		const UObject* Asset,
		FName DefinitionID,
		EContentDefinitionFailure Failure,
		const TCHAR* Label)
	{
		if (Seen.Contains(Value))
		{
			Report.Add(
				Failure,
				Asset,
				DefinitionID,
				NAME_None,
				FString::Printf(TEXT("Duplicate %s for definition %s."), Label, *DefinitionID.ToString()));
			return false;
		}
		Seen.Add(Value);
		return true;
	}
}

void FContentDefinitionValidationReport::Add(
	EContentDefinitionFailure Failure,
	const UObject* Asset,
	FName DefinitionID,
	FName ReferenceID,
	const FString& Details)
{
	FContentDefinitionIssue& Issue = Issues.AddDefaulted_GetRef();
	Issue.Failure = Failure;
	Issue.AssetName = Asset ? Asset->GetFName() : NAME_None;
	Issue.DefinitionID = DefinitionID;
	Issue.ReferenceID = ReferenceID;
	Issue.Details = Details;
}

FContentDefinitionValidationReport RomanEmpireContent::ValidateDefinitionSet(
	const TArray<URomanFactionDefinition*>& Factions,
	const TArray<URomanUnitDefinition*>& Units,
	const TArray<URomanBuildingDefinition*>& Buildings,
	const TArray<URomanTerritoryDefinition*>& Territories)
{
	FContentDefinitionValidationReport Report;
	if (Factions.IsEmpty())
	{
		Report.Add(EContentDefinitionFailure::MissingAssetType, nullptr, NAME_None, TEXT("RomanFaction"),
			TEXT("No faction Primary Data Assets were discovered."));
	}
	if (Units.IsEmpty())
	{
		Report.Add(EContentDefinitionFailure::MissingAssetType, nullptr, NAME_None, TEXT("RomanUnit"),
			TEXT("No unit Primary Data Assets were discovered."));
	}
	if (Buildings.IsEmpty())
	{
		Report.Add(EContentDefinitionFailure::MissingAssetType, nullptr, NAME_None, TEXT("RomanBuilding"),
			TEXT("No building Primary Data Assets were discovered."));
	}
	if (Territories.IsEmpty())
	{
		Report.Add(EContentDefinitionFailure::MissingAssetType, nullptr, NAME_None, TEXT("RomanTerritory"),
			TEXT("No territory Primary Data Assets were discovered."));
	}

	TSet<FName> FactionIDs;
	TSet<EFactionID> RuntimeFactionIDs;
	for (const URomanFactionDefinition* Asset : Factions)
	{
		if (!Asset)
		{
			Report.Add(EContentDefinitionFailure::MissingAsset, nullptr, NAME_None, NAME_None,
				TEXT("Faction definition pointer is null."));
			continue;
		}

		const FFactionInfo& Definition = Asset->Definition;
		AddUniqueOrReport(FactionIDs, Definition.FactionID, Report, Asset, Definition.FactionID,
			EContentDefinitionFailure::DuplicateStableID, TEXT("faction stable ID"));
		AddUniqueOrReport(RuntimeFactionIDs, Definition.RuntimeFaction, Report, Asset, Definition.FactionID,
			EContentDefinitionFailure::DuplicateRuntimeID, TEXT("faction runtime ID"));
		if (!IsStableID(Definition.FactionID) || !IsRuntimeFaction(Definition.RuntimeFaction) ||
			Definition.DisplayName.IsEmpty() || !Definition.StartingResources.IsNonNegative() ||
			Definition.InfantryBonus <= 0.0f || Definition.CavalryBonus <= 0.0f ||
			Definition.EconomyBonus <= 0.0f)
		{
			Report.Add(EContentDefinitionFailure::InvalidDefinition, Asset, Definition.FactionID, NAME_None,
				TEXT("Faction identity, display data, bonuses, or starting resources are invalid."));
		}
	}

	TSet<FName> UnitIDs;
	TSet<EUnitType> UnitTypes;
	for (const URomanUnitDefinition* Asset : Units)
	{
		if (!Asset)
		{
			Report.Add(EContentDefinitionFailure::MissingAsset, nullptr, NAME_None, NAME_None,
				TEXT("Unit definition pointer is null."));
			continue;
		}

		const FUnitData& Definition = Asset->Definition;
		AddUniqueOrReport(UnitIDs, Definition.UnitID, Report, Asset, Definition.UnitID,
			EContentDefinitionFailure::DuplicateStableID, TEXT("unit stable ID"));
		AddUniqueOrReport(UnitTypes, Definition.UnitType, Report, Asset, Definition.UnitID,
			EContentDefinitionFailure::DuplicateRuntimeID, TEXT("unit runtime type"));
		if (!IsStableID(Definition.UnitID) || Definition.UnitType == EUnitType::None ||
			Definition.DisplayName.IsEmpty() || !Definition.TrainingCost.IsNonNegative() ||
			Definition.TrainingTime <= 0.0f || Definition.AttackRange <= 0.0f ||
			!AreUnitStatsValid(Definition.BaseStats) || Definition.StartingRangedAmmo < 0 ||
			Definition.SpecialRangedDamage < 0.0f || Definition.SpecialRangedRange < 0.0f ||
			(Definition.bCanUseRanged &&
				(Definition.StartingRangedAmmo <= 0 || Definition.SpecialRangedDamage <= 0.0f ||
					Definition.SpecialRangedRange <= 0.0f)) ||
			Definition.DefensiveFormationSpeedMultiplier <= 0.0f ||
			Definition.DefensiveFormationMeleeDefenseBonus < 0 ||
			Definition.DefensiveFormationRangedDefenseBonus < 0)
		{
			Report.Add(EContentDefinitionFailure::InvalidDefinition, Asset, Definition.UnitID, NAME_None,
				TEXT("Unit identity, display data, cost, duration, range, or combat values are invalid."));
		}
		if (Asset->UnitClass.IsNull())
		{
			Report.Add(EContentDefinitionFailure::MissingFactoryClass, Asset, Definition.UnitID, NAME_None,
				TEXT("Unit factory class is missing."));
		}
	}

	TSet<FName> BuildingIDs;
	TSet<EBuildingType> BuildingTypes;
	TSet<int32> QuickSlots;
	for (const URomanBuildingDefinition* Asset : Buildings)
	{
		if (!Asset)
		{
			Report.Add(EContentDefinitionFailure::MissingAsset, nullptr, NAME_None, NAME_None,
				TEXT("Building definition pointer is null."));
			continue;
		}

		const FBuildingData& Definition = Asset->Definition;
		AddUniqueOrReport(BuildingIDs, Definition.BuildingID, Report, Asset, Definition.BuildingID,
			EContentDefinitionFailure::DuplicateStableID, TEXT("building stable ID"));
		AddUniqueOrReport(BuildingTypes, Definition.BuildingType, Report, Asset, Definition.BuildingID,
			EContentDefinitionFailure::DuplicateRuntimeID, TEXT("building runtime type"));
		if (Asset->QuickSlot > 0)
		{
			AddUniqueOrReport(QuickSlots, Asset->QuickSlot, Report, Asset, Definition.BuildingID,
				EContentDefinitionFailure::DuplicateQuickSlot, TEXT("building quick slot"));
		}
		if (!IsStableID(Definition.BuildingID) || Definition.BuildingType == EBuildingType::None ||
			Definition.DisplayName.IsEmpty() || !Definition.Cost.IsNonNegative() ||
			Definition.ConstructionTime <= 0.0f || Definition.MaxHealth <= 0 ||
			Definition.Armor < 0 || Definition.FootprintSize.X <= 0.0f ||
			Definition.FootprintSize.Y <= 0.0f || Definition.PrototypeScale.X <= 0.0f ||
			Definition.PrototypeScale.Y <= 0.0f || Definition.PrototypeScale.Z <= 0.0f ||
			Definition.ProductionRate < 0 ||
			Asset->QuickSlot < 0 || Asset->QuickSlot > 9)
		{
			Report.Add(EContentDefinitionFailure::InvalidDefinition, Asset, Definition.BuildingID, NAME_None,
				TEXT("Building identity, display data, cost, duration, stats, footprint, production, or quick slot is invalid."));
		}
		if (Asset->BuildingClass.IsNull())
		{
			Report.Add(EContentDefinitionFailure::MissingFactoryClass, Asset, Definition.BuildingID, NAME_None,
				TEXT("Building factory class is missing."));
		}
	}

	TSet<FName> TerritoryIDs;
	for (const URomanTerritoryDefinition* Asset : Territories)
	{
		if (!Asset)
		{
			Report.Add(EContentDefinitionFailure::MissingAsset, nullptr, NAME_None, NAME_None,
				TEXT("Territory definition pointer is null."));
			continue;
		}

		const FTerritoryDefinition& Definition = Asset->Definition;
		AddUniqueOrReport(TerritoryIDs, Definition.TerritoryID, Report, Asset, Definition.TerritoryID,
			EContentDefinitionFailure::DuplicateStableID, TEXT("territory stable ID"));
		if (!Definition.IsValid() || Asset->MapLocation.ContainsNaN())
		{
			Report.Add(EContentDefinitionFailure::InvalidDefinition, Asset, Definition.TerritoryID, NAME_None,
				TEXT("Territory definition or map location is invalid."));
		}
		if (Definition.StartingOwner != EFactionID::None && !RuntimeFactionIDs.Contains(Definition.StartingOwner))
		{
			Report.Add(EContentDefinitionFailure::InvalidFactionReference, Asset, Definition.TerritoryID, NAME_None,
				TEXT("Territory starting owner has no faction definition."));
		}
	}

	for (const URomanUnitDefinition* Asset : Units)
	{
		if (!Asset)
		{
			continue;
		}
		for (FName FactionID : Asset->AvailableToFactions)
		{
			if (!FactionIDs.Contains(FactionID))
			{
				Report.Add(EContentDefinitionFailure::InvalidFactionReference, Asset,
					Asset->Definition.UnitID, FactionID, TEXT("Unit availability references an unknown faction."));
			}
		}
	}

	for (const URomanBuildingDefinition* Asset : Buildings)
	{
		if (!Asset)
		{
			continue;
		}
		for (FName FactionID : Asset->AvailableToFactions)
		{
			if (!FactionIDs.Contains(FactionID))
			{
				Report.Add(EContentDefinitionFailure::InvalidFactionReference, Asset,
					Asset->Definition.BuildingID, FactionID, TEXT("Building availability references an unknown faction."));
			}
		}
		for (FName UnitID : Asset->Definition.TrainableUnits)
		{
			if (!UnitIDs.Contains(UnitID))
			{
				Report.Add(EContentDefinitionFailure::InvalidUnitReference, Asset,
					Asset->Definition.BuildingID, UnitID, TEXT("Building trainable-unit list references an unknown unit."));
			}
		}
	}

	for (const URomanFactionDefinition* Asset : Factions)
	{
		if (!Asset)
		{
			continue;
		}
		for (FName UnitID : Asset->Definition.UniqueUnitTypes)
		{
			if (!UnitIDs.Contains(UnitID))
			{
				Report.Add(EContentDefinitionFailure::InvalidUnitReference, Asset,
					Asset->Definition.FactionID, UnitID, TEXT("Faction unit list references an unknown unit."));
			}
		}
		for (FName BuildingID : Asset->Definition.UniqueBuildingTypes)
		{
			if (!BuildingIDs.Contains(BuildingID))
			{
				Report.Add(EContentDefinitionFailure::InvalidDefinition, Asset,
					Asset->Definition.FactionID, BuildingID, TEXT("Faction building list references an unknown building."));
			}
		}
	}

	return Report;
}

const TCHAR* RomanEmpireContent::GetDefinitionFailureName(EContentDefinitionFailure Failure)
{
	switch (Failure)
	{
	case EContentDefinitionFailure::None: return TEXT("None");
	case EContentDefinitionFailure::MissingAssetType: return TEXT("MissingAssetType");
	case EContentDefinitionFailure::MissingAsset: return TEXT("MissingAsset");
	case EContentDefinitionFailure::InvalidDefinition: return TEXT("InvalidDefinition");
	case EContentDefinitionFailure::DuplicateStableID: return TEXT("DuplicateStableID");
	case EContentDefinitionFailure::DuplicateRuntimeID: return TEXT("DuplicateRuntimeID");
	case EContentDefinitionFailure::DuplicateQuickSlot: return TEXT("DuplicateQuickSlot");
	case EContentDefinitionFailure::MissingFactoryClass: return TEXT("MissingFactoryClass");
	case EContentDefinitionFailure::InvalidFactionReference: return TEXT("InvalidFactionReference");
	case EContentDefinitionFailure::InvalidUnitReference: return TEXT("InvalidUnitReference");
	default: return TEXT("Unknown");
	}
}

void URomanContentRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ReloadDefinitions();
}

void URomanContentRegistrySubsystem::Deinitialize()
{
	ResetRegistry();
	Super::Deinitialize();
}

URomanContentRegistrySubsystem* URomanContentRegistrySubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<URomanContentRegistrySubsystem>() : nullptr;
}

bool URomanContentRegistrySubsystem::ReloadDefinitions()
{
	ResetRegistry();

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<URomanFactionDefinition*> Factions;
	TArray<URomanUnitDefinition*> Units;
	TArray<URomanBuildingDefinition*> Buildings;
	TArray<URomanTerritoryDefinition*> Territories;
	LoadPrimaryDefinitions(AssetManager, RomanEmpireContent::FactionAssetType, Factions, ValidationReport);
	LoadPrimaryDefinitions(AssetManager, RomanEmpireContent::UnitAssetType, Units, ValidationReport);
	LoadPrimaryDefinitions(AssetManager, RomanEmpireContent::BuildingAssetType, Buildings, ValidationReport);
	LoadPrimaryDefinitions(AssetManager, RomanEmpireContent::TerritoryAssetType, Territories, ValidationReport);

	FContentDefinitionValidationReport SetReport = RomanEmpireContent::ValidateDefinitionSet(
		Factions, Units, Buildings, Territories);
	ValidationReport.Issues.Append(SetReport.Issues);
	if (!ValidationReport.IsValid())
	{
		for (const FContentDefinitionIssue& Issue : ValidationReport.Issues)
		{
			UE_LOG(LogRomanEmpire, Error, TEXT("Content definition error [%s] asset=%s id=%s reference=%s: %s"),
				RomanEmpireContent::GetDefinitionFailureName(Issue.Failure),
				*Issue.AssetName.ToString(),
				*Issue.DefinitionID.ToString(),
				*Issue.ReferenceID.ToString(),
				*Issue.Details);
		}
		return false;
	}

	for (URomanFactionDefinition* Asset : Factions)
	{
		FactionsByID.Add(Asset->Definition.FactionID, Asset);
		FactionsByRuntimeID.Add(Asset->Definition.RuntimeFaction, Asset);
	}
	for (URomanUnitDefinition* Asset : Units)
	{
		UClass* FactoryClass = Asset->UnitClass.LoadSynchronous();
		if (!FactoryClass)
		{
			ValidationReport.Add(EContentDefinitionFailure::MissingFactoryClass, Asset,
				Asset->Definition.UnitID, NAME_None, TEXT("Unit factory class failed to load at startup."));
			continue;
		}
		LoadedFactoryClasses.AddUnique(FactoryClass);
		UnitsByID.Add(Asset->Definition.UnitID, Asset);
	}
	for (URomanBuildingDefinition* Asset : Buildings)
	{
		UClass* FactoryClass = Asset->BuildingClass.LoadSynchronous();
		if (!FactoryClass)
		{
			ValidationReport.Add(EContentDefinitionFailure::MissingFactoryClass, Asset,
				Asset->Definition.BuildingID, NAME_None, TEXT("Building factory class failed to load at startup."));
			continue;
		}
		LoadedFactoryClasses.AddUnique(FactoryClass);
		BuildingsByID.Add(Asset->Definition.BuildingID, Asset);
		if (Asset->QuickSlot > 0)
		{
			BuildingsByQuickSlot.Add(Asset->QuickSlot, Asset);
		}
	}
	for (URomanTerritoryDefinition* Asset : Territories)
	{
		TerritoriesByID.Add(Asset->Definition.TerritoryID, Asset);
	}

	bIsReady = ValidationReport.IsValid();
	if (bIsReady)
	{
		UE_LOG(LogRomanEmpire, Log,
			TEXT("Content registry initialized: %d factions, %d units, %d buildings, %d territories"),
			FactionsByID.Num(), UnitsByID.Num(), BuildingsByID.Num(), TerritoriesByID.Num());
	}
	else
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Content registry failed after loading factory classes"));
	}
	return bIsReady;
}

const URomanFactionDefinition* URomanContentRegistrySubsystem::FindFaction(FName FactionID) const
{
	const TObjectPtr<URomanFactionDefinition>* Found = FactionsByID.Find(FactionID);
	return Found ? Found->Get() : nullptr;
}

const URomanFactionDefinition* URomanContentRegistrySubsystem::FindFaction(EFactionID RuntimeFaction) const
{
	const TObjectPtr<URomanFactionDefinition>* Found = FactionsByRuntimeID.Find(RuntimeFaction);
	return Found ? Found->Get() : nullptr;
}

const URomanUnitDefinition* URomanContentRegistrySubsystem::FindUnit(FName UnitID) const
{
	const TObjectPtr<URomanUnitDefinition>* Found = UnitsByID.Find(UnitID);
	return Found ? Found->Get() : nullptr;
}

const URomanBuildingDefinition* URomanContentRegistrySubsystem::FindBuilding(FName BuildingID) const
{
	const TObjectPtr<URomanBuildingDefinition>* Found = BuildingsByID.Find(BuildingID);
	return Found ? Found->Get() : nullptr;
}

const URomanBuildingDefinition* URomanContentRegistrySubsystem::FindBuildingByQuickSlot(int32 QuickSlot) const
{
	const TObjectPtr<URomanBuildingDefinition>* Found = BuildingsByQuickSlot.Find(QuickSlot);
	return Found ? Found->Get() : nullptr;
}

const URomanTerritoryDefinition* URomanContentRegistrySubsystem::FindTerritory(FName TerritoryID) const
{
	const TObjectPtr<URomanTerritoryDefinition>* Found = TerritoriesByID.Find(TerritoryID);
	return Found ? Found->Get() : nullptr;
}

TArray<const URomanFactionDefinition*> URomanContentRegistrySubsystem::GetAllFactions() const
{
	TArray<const URomanFactionDefinition*> Result;
	for (const TPair<FName, TObjectPtr<URomanFactionDefinition>>& Pair : FactionsByID)
	{
		Result.Add(Pair.Value.Get());
	}
	Result.Sort([](const URomanFactionDefinition& Left, const URomanFactionDefinition& Right)
	{
		return Left.Definition.FactionID.LexicalLess(Right.Definition.FactionID);
	});
	return Result;
}

TArray<const URomanUnitDefinition*> URomanContentRegistrySubsystem::GetAllUnits() const
{
	TArray<const URomanUnitDefinition*> Result;
	for (const TPair<FName, TObjectPtr<URomanUnitDefinition>>& Pair : UnitsByID)
	{
		Result.Add(Pair.Value.Get());
	}
	Result.Sort([](const URomanUnitDefinition& Left, const URomanUnitDefinition& Right)
	{
		return Left.Definition.UnitID.LexicalLess(Right.Definition.UnitID);
	});
	return Result;
}

TArray<const URomanBuildingDefinition*> URomanContentRegistrySubsystem::GetAllBuildings() const
{
	TArray<const URomanBuildingDefinition*> Result;
	for (const TPair<FName, TObjectPtr<URomanBuildingDefinition>>& Pair : BuildingsByID)
	{
		Result.Add(Pair.Value.Get());
	}
	Result.Sort([](const URomanBuildingDefinition& Left, const URomanBuildingDefinition& Right)
	{
		if (Left.QuickSlot != Right.QuickSlot)
		{
			return Left.QuickSlot < Right.QuickSlot;
		}
		return Left.Definition.BuildingID.LexicalLess(Right.Definition.BuildingID);
	});
	return Result;
}

TArray<const URomanTerritoryDefinition*> URomanContentRegistrySubsystem::GetAllTerritories() const
{
	TArray<const URomanTerritoryDefinition*> Result;
	for (const TPair<FName, TObjectPtr<URomanTerritoryDefinition>>& Pair : TerritoriesByID)
	{
		Result.Add(Pair.Value.Get());
	}
	Result.Sort([](const URomanTerritoryDefinition& Left, const URomanTerritoryDefinition& Right)
	{
		return Left.Definition.TerritoryID.LexicalLess(Right.Definition.TerritoryID);
	});
	return Result;
}

void URomanContentRegistrySubsystem::ResetRegistry()
{
	bIsReady = false;
	FactionsByID.Reset();
	FactionsByRuntimeID.Reset();
	UnitsByID.Reset();
	BuildingsByID.Reset();
	BuildingsByQuickSlot.Reset();
	TerritoriesByID.Reset();
	LoadedFactoryClasses.Reset();
	ValidationReport.Issues.Reset();
}
