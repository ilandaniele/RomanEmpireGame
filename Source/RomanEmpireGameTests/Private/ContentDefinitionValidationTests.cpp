// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Content/RomanContentRegistry.h"
#include "Engine/AssetManager.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	template <typename TDefinition>
	void LoadDefinitions(
		UAssetManager& AssetManager,
		const FPrimaryAssetType& AssetType,
		TArray<TDefinition*>& OutDefinitions)
	{
		TArray<FPrimaryAssetId> AssetIDs;
		AssetManager.GetPrimaryAssetIdList(AssetType, AssetIDs);
		for (const FPrimaryAssetId& AssetID : AssetIDs)
		{
			UObject* Object = AssetManager.GetPrimaryAssetObject(AssetID);
			if (!Object)
			{
				Object = AssetManager.GetPrimaryAssetPath(AssetID).TryLoad();
			}
			if (TDefinition* Definition = Cast<TDefinition>(Object))
			{
				OutDefinitions.Add(Definition);
			}
		}
	}

	bool ContainsFailure(
		const FContentDefinitionValidationReport& Report,
		EContentDefinitionFailure Failure)
	{
		for (const FContentDefinitionIssue& Issue : Report.Issues)
		{
			if (Issue.Failure == Failure)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FContentDefinitionFailureValidationTest,
	"RomanEmpireGame.Content.Definitions.ValidationFailures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FContentDefinitionFailureValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	URomanFactionDefinition* Faction = NewObject<URomanFactionDefinition>();
	Faction->Definition.FactionID = TEXT("Rome");
	Faction->Definition.RuntimeFaction = EFactionID::Rome;
	Faction->Definition.DisplayName = FText::FromString(TEXT("Roman Republic"));

	URomanUnitDefinition* Unit = NewObject<URomanUnitDefinition>();
	Unit->Definition.UnitID = TEXT("Legionary");
	Unit->Definition.UnitType = EUnitType::Legionary;
	Unit->Definition.DisplayName = FText::FromString(TEXT("Legionary"));
	Unit->Definition.BaseStats.MaxHealth = 1;
	Unit->Definition.BaseStats.Speed = 1.0f;
	Unit->Definition.BaseStats.Stamina = 1.0f;
	Unit->Definition.BaseStats.AttackSpeed = 1.0f;
	Unit->Definition.TrainingTime = 1.0f;
	Unit->Definition.AttackRange = 100.0f;
	Unit->UnitClass = TSoftClassPtr<AUnitBase>(
		FSoftObjectPath(TEXT("/Script/RomanEmpireGame.Legionary")));
	Unit->AvailableToFactions.Add(TEXT("Rome"));

	URomanBuildingDefinition* Building = NewObject<URomanBuildingDefinition>();
	Building->Definition.BuildingID = TEXT("Barracks");
	Building->Definition.BuildingType = EBuildingType::Barracks;
	Building->Definition.DisplayName = FText::FromString(TEXT("Barracks"));
	Building->Definition.ConstructionTime = 1.0f;
	Building->Definition.MaxHealth = 1;
	Building->Definition.FootprintSize = FVector2D(1.0f, 1.0f);
	Building->Definition.PrototypeScale = FVector(1.0f, 1.0f, 1.0f);
	Building->Definition.TrainableUnits.Add(TEXT("Legionary"));
	Building->BuildingClass = TSoftClassPtr<ABuildingBase>(
		FSoftObjectPath(TEXT("/Script/RomanEmpireGame.Barracks")));
	Building->AvailableToFactions.Add(TEXT("Rome"));
	Building->QuickSlot = 1;

	URomanTerritoryDefinition* Territory = NewObject<URomanTerritoryDefinition>();
	Territory->Definition.TerritoryID = TEXT("Rome");
	Territory->Definition.DisplayName = FText::FromString(TEXT("Italia"));
	Territory->Definition.StartingOwner = EFactionID::Rome;

	TArray<URomanFactionDefinition*> Factions = { Faction };
	TArray<URomanUnitDefinition*> Units = { Unit };
	TArray<URomanBuildingDefinition*> Buildings = { Building };
	TArray<URomanTerritoryDefinition*> Territories = { Territory };

	TestTrue(
		TEXT("A complete definition set is valid"),
		RomanEmpireContent::ValidateDefinitionSet(Factions, Units, Buildings, Territories).IsValid());

	URomanFactionDefinition* DuplicateFaction = NewObject<URomanFactionDefinition>();
	DuplicateFaction->Definition = Faction->Definition;
	DuplicateFaction->Definition.RuntimeFaction = EFactionID::Gaul;
	Factions.Add(DuplicateFaction);
	const FContentDefinitionValidationReport DuplicateReport =
		RomanEmpireContent::ValidateDefinitionSet(Factions, Units, Buildings, Territories);
	TestTrue(
		TEXT("Duplicate stable IDs are reported"),
		ContainsFailure(DuplicateReport, EContentDefinitionFailure::DuplicateStableID));
	Factions.Pop();

	Unit->AvailableToFactions.Add(TEXT("MissingFaction"));
	const FContentDefinitionValidationReport FactionReferenceReport =
		RomanEmpireContent::ValidateDefinitionSet(Factions, Units, Buildings, Territories);
	TestTrue(
		TEXT("Unknown faction references are reported"),
		ContainsFailure(FactionReferenceReport, EContentDefinitionFailure::InvalidFactionReference));
	Unit->AvailableToFactions.Pop();

	Building->Definition.Cost.Gold = -1;
	const FContentDefinitionValidationReport CostReport =
		RomanEmpireContent::ValidateDefinitionSet(Factions, Units, Buildings, Territories);
	TestTrue(
		TEXT("Negative content costs are reported"),
		ContainsFailure(CostReport, EContentDefinitionFailure::InvalidDefinition));
	Building->Definition.Cost.Gold = 0;

	const TArray<URomanUnitDefinition*> MissingUnits;
	const FContentDefinitionValidationReport MissingTypeReport =
		RomanEmpireContent::ValidateDefinitionSet(Factions, MissingUnits, Buildings, Territories);
	TestTrue(
		TEXT("Missing Primary Asset categories are reported"),
		ContainsFailure(MissingTypeReport, EContentDefinitionFailure::MissingAssetType));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPrimaryContentDefinitionDiscoveryTest,
	"RomanEmpireGame.Content.Definitions.PrimaryAssetDiscovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPrimaryContentDefinitionDiscoveryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UAssetManager& AssetManager = UAssetManager::Get();
	TArray<URomanFactionDefinition*> Factions;
	TArray<URomanUnitDefinition*> Units;
	TArray<URomanBuildingDefinition*> Buildings;
	TArray<URomanTerritoryDefinition*> Territories;
	LoadDefinitions(AssetManager, RomanEmpireContent::FactionAssetType, Factions);
	LoadDefinitions(AssetManager, RomanEmpireContent::UnitAssetType, Units);
	LoadDefinitions(AssetManager, RomanEmpireContent::BuildingAssetType, Buildings);
	LoadDefinitions(AssetManager, RomanEmpireContent::TerritoryAssetType, Territories);

	TestEqual(TEXT("Three faction definitions are discovered"), Factions.Num(), 3);
	TestEqual(TEXT("One unit definition is discovered"), Units.Num(), 1);
	TestEqual(TEXT("Six building definitions are discovered"), Buildings.Num(), 6);
	TestEqual(TEXT("Nine territory definitions are discovered"), Territories.Num(), 9);

	const FContentDefinitionValidationReport Report =
		RomanEmpireContent::ValidateDefinitionSet(Factions, Units, Buildings, Territories);
	for (const FContentDefinitionIssue& Issue : Report.Issues)
	{
		AddError(FString::Printf(
			TEXT("Unexpected content issue %s for %s: %s"),
			RomanEmpireContent::GetDefinitionFailureName(Issue.Failure),
			*Issue.DefinitionID.ToString(),
			*Issue.Details));
	}
	TestTrue(TEXT("Authored Primary Data Assets pass cross-content validation"), Report.IsValid());

	const URomanFactionDefinition* Rome = nullptr;
	for (const URomanFactionDefinition* Candidate : Factions)
	{
		if (Candidate && Candidate->Definition.FactionID == TEXT("Rome"))
		{
			Rome = Candidate;
			break;
		}
	}
	if (TestNotNull(TEXT("Rome resolves through its stable content ID"), Rome))
	{
		TestEqual(TEXT("Rome's authored starting gold is canonical"),
			Rome->Definition.StartingResources.Gold, 1100);
		TestEqual(TEXT("Rome's authored display name drives presentation"),
			Rome->Definition.DisplayName.ToString(), FString(TEXT("Roman Republic")));
	}

	const URomanUnitDefinition* Legionary = Units.Num() == 1 ? Units[0] : nullptr;
	if (TestNotNull(TEXT("Legionary definition is available"), Legionary))
	{
		TestEqual(TEXT("Legionary uses a stable ID"), Legionary->Definition.UnitID, FName(TEXT("Legionary")));
		TestEqual(TEXT("Legionary training cost comes from content"), Legionary->Definition.TrainingCost.Gold, 150);
		TestEqual(TEXT("Legionary combat range comes from content"), Legionary->Definition.AttackRange, 150.0f);
		TestEqual(TEXT("Legionary factory class comes from content"),
			Legionary->UnitClass.ToSoftObjectPath().ToString(),
			FString(TEXT("/Script/RomanEmpireGame.Legionary")));
	}

	const URomanBuildingDefinition* Barracks = nullptr;
	for (const URomanBuildingDefinition* Candidate : Buildings)
	{
		if (Candidate && Candidate->Definition.BuildingID == TEXT("Barracks"))
		{
			Barracks = Candidate;
			break;
		}
	}
	if (TestNotNull(TEXT("Barracks resolves through its stable content ID"), Barracks))
	{
		TestEqual(TEXT("Barracks quick slot is authored"), Barracks->QuickSlot, 1);
		TestEqual(TEXT("Barracks cost is authored once"), Barracks->Definition.Cost.Gold, 400);
		TestEqual(TEXT("Barracks trains the authored stable unit ID"),
			Barracks->Definition.TrainableUnits[0], FName(TEXT("Legionary")));
	}

	const URomanTerritoryDefinition* Africa = nullptr;
	for (const URomanTerritoryDefinition* Candidate : Territories)
	{
		if (Candidate && Candidate->Definition.TerritoryID == TEXT("Africa"))
		{
			Africa = Candidate;
			break;
		}
	}
	if (TestNotNull(TEXT("Africa resolves through its stable territory ID"), Africa))
	{
		TestEqual(TEXT("Africa's starting owner is authored"),
			Africa->Definition.StartingOwner, EFactionID::Carthage);
		TestEqual(TEXT("Africa's settlement label is authored"),
			Africa->StartingSettlementName.ToString(), FString(TEXT("Carthago")));
		TestEqual(TEXT("Africa's map position is authored"),
			Africa->MapLocation, FVector(-10000.0f, -10000.0f, 0.0f));
	}

	return true;
}

#endif
