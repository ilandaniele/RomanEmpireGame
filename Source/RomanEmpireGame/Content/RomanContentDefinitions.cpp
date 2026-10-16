// Copyright Roman Empire Game. All Rights Reserved.

#include "Content/RomanContentDefinitions.h"

#include "AssetRegistry/AssetData.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace RomanEmpireContent
{
	const FPrimaryAssetType FactionAssetType(TEXT("RomanFaction"));
	const FPrimaryAssetType UnitAssetType(TEXT("RomanUnit"));
	const FPrimaryAssetType BuildingAssetType(TEXT("RomanBuilding"));
	const FPrimaryAssetType TerritoryAssetType(TEXT("RomanTerritory"));

	bool IsStableID(FName DefinitionID)
	{
		return !DefinitionID.IsNone() && !DefinitionID.ToString().TrimStartAndEnd().IsEmpty();
	}

	bool IsRuntimeFaction(EFactionID FactionID)
	{
		switch (FactionID)
		{
		case EFactionID::Rome:
		case EFactionID::Carthage:
		case EFactionID::Gaul:
		case EFactionID::Greece:
		case EFactionID::Egypt:
		case EFactionID::Britannia:
			return true;
		default:
			return false;
		}
	}
}

FPrimaryAssetId URomanFactionDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(RomanEmpireContent::FactionAssetType, GetFName());
}

FPrimaryAssetId URomanUnitDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(RomanEmpireContent::UnitAssetType, GetFName());
}

FPrimaryAssetId URomanBuildingDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(RomanEmpireContent::BuildingAssetType, GetFName());
}

FPrimaryAssetId URomanTerritoryDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(RomanEmpireContent::TerritoryAssetType, GetFName());
}

namespace RomanEmpireContent
{
	bool AreUnitStatsValid(const FUnitStats& Stats)
	{
		return Stats.MaxHealth > 0 && Stats.MeleeAttack >= 0 && Stats.RangedAttack >= 0 &&
			Stats.MeleeDefense >= 0 && Stats.RangedDefense >= 0 && Stats.Armor >= 0 &&
			Stats.Speed > 0.0f && Stats.Morale >= 0 && Stats.Stamina > 0.0f &&
			Stats.AttackSpeed > 0.0f && Stats.BlockStrength >= 0.0f && Stats.DodgeSpeed >= 0.0f;
	}
}

#if WITH_EDITOR
namespace
{
	void AddError(FDataValidationContext& Context, const TCHAR* Message)
	{
		Context.AddError(FText::FromString(Message));
	}
}

EDataValidationResult URomanFactionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context), EDataValidationResult::Valid);
	if (!RomanEmpireContent::IsStableID(Definition.FactionID))
	{
		AddError(Context, TEXT("Faction definition requires a stable FactionID."));
		Result = EDataValidationResult::Invalid;
	}
	if (!RomanEmpireContent::IsRuntimeFaction(Definition.RuntimeFaction))
	{
		AddError(Context, TEXT("Faction definition references an invalid RuntimeFaction."));
		Result = EDataValidationResult::Invalid;
	}
	if (Definition.DisplayName.IsEmpty() || !Definition.StartingResources.IsNonNegative() ||
		Definition.InfantryBonus <= 0.0f || Definition.CavalryBonus <= 0.0f ||
		Definition.EconomyBonus <= 0.0f)
	{
		AddError(Context, TEXT("Faction display data, bonuses, or starting resources are invalid."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}

EDataValidationResult URomanUnitDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context), EDataValidationResult::Valid);
	if (!RomanEmpireContent::IsStableID(Definition.UnitID) ||
		Definition.UnitType == EUnitType::None || Definition.DisplayName.IsEmpty())
	{
		AddError(Context, TEXT("Unit definition requires stable identity, runtime type, and display name."));
		Result = EDataValidationResult::Invalid;
	}
	if (!Definition.TrainingCost.IsNonNegative() || Definition.TrainingTime <= 0.0f ||
		Definition.AttackRange <= 0.0f || !RomanEmpireContent::AreUnitStatsValid(Definition.BaseStats) ||
		Definition.StartingRangedAmmo < 0 || Definition.SpecialRangedDamage < 0.0f ||
		Definition.SpecialRangedRange < 0.0f ||
		(Definition.bCanUseRanged &&
			(Definition.StartingRangedAmmo <= 0 || Definition.SpecialRangedDamage <= 0.0f ||
				Definition.SpecialRangedRange <= 0.0f)) ||
		Definition.DefensiveFormationSpeedMultiplier <= 0.0f ||
		Definition.DefensiveFormationMeleeDefenseBonus < 0 ||
		Definition.DefensiveFormationRangedDefenseBonus < 0)
	{
		AddError(Context, TEXT("Unit definition contains invalid cost, duration, range, or combat values."));
		Result = EDataValidationResult::Invalid;
	}
	if (UnitClass.IsNull())
	{
		AddError(Context, TEXT("Unit definition requires a factory class."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}

EDataValidationResult URomanBuildingDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context), EDataValidationResult::Valid);
	if (!RomanEmpireContent::IsStableID(Definition.BuildingID) ||
		Definition.BuildingType == EBuildingType::None || Definition.DisplayName.IsEmpty())
	{
		AddError(Context, TEXT("Building definition requires stable identity, runtime type, and display name."));
		Result = EDataValidationResult::Invalid;
	}
	if (!Definition.Cost.IsNonNegative() || Definition.ConstructionTime <= 0.0f ||
		Definition.MaxHealth <= 0 || Definition.Armor < 0 ||
		Definition.FootprintSize.X <= 0.0f || Definition.FootprintSize.Y <= 0.0f ||
		Definition.PrototypeScale.X <= 0.0f || Definition.PrototypeScale.Y <= 0.0f ||
		Definition.PrototypeScale.Z <= 0.0f ||
		Definition.ProductionRate < 0 || QuickSlot < 0 || QuickSlot > 9)
	{
		AddError(Context, TEXT("Building definition contains invalid cost, duration, stats, footprint, production, or quick slot."));
		Result = EDataValidationResult::Invalid;
	}
	if (BuildingClass.IsNull())
	{
		AddError(Context, TEXT("Building definition requires a factory class."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}

EDataValidationResult URomanTerritoryDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(
		Super::IsDataValid(Context), EDataValidationResult::Valid);
	if (!Definition.IsValid() || MapLocation.ContainsNaN())
	{
		AddError(Context, TEXT("Territory definition or prototype-map location is invalid."));
		Result = EDataValidationResult::Invalid;
	}
	if (!HasValidTacticalMap())
	{
		AddError(Context, TEXT("Territory requires an existing top-level World asset as its tactical map."));
		Result = EDataValidationResult::Invalid;
	}
	return Result;
}
#endif

bool URomanTerritoryDefinition::HasValidTacticalMap() const
{
	const FSoftObjectPath Path = TacticalMap.ToSoftObjectPath();
	const UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager || !Path.IsValid() || !Path.GetSubPathString().IsEmpty()) return false;
	FAssetData Metadata;
	return AssetManager->GetAssetDataForPath(Path, Metadata) &&
		Metadata.AssetClassPath == UWorld::StaticClass()->GetClassPathName() &&
		Metadata.GetSoftObjectPath() == Path;
}
