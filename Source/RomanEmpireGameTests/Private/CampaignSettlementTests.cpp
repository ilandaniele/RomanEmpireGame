// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Campaign/TerritoryData.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignSettlementTest, "RomanEmpireGame.Rules.Campaign.Settlement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FCampaignSettlementTest::RunTest(const FString& Parameters)
{
	FTerritoryCampaignState State;
	FTerritoryDefinition Definition;
	Definition.TerritoryID = TEXT("Region");
	Definition.DisplayName = FText::FromString(TEXT("Region"));
	Definition.ResourceProduction.Gold = 11;
	Definition.ResourceProduction.Food = 22;
	Definition.ResourceProduction.Iron = 33;
	Definition.ResourceProduction.Wood = 44;
	Definition.ResourceProduction.Stone = 55;
	Definition.ResourceProduction.Population = 66;
	TestTrue(TEXT("Register live region"), State.RegisterTerritory(Definition).WasSuccessful());
	FTerritorySettlementState Settlement;
	FFactionResourceDelta Production;
	TestTrue(TEXT("Registered empty settlement is readable"), State.TryGetSettlement(Definition.TerritoryID, Settlement));
	TestFalse(TEXT("No settlement by default"), Settlement.bHasSettlement);
	TestTrue(TEXT("Empty settlement produces base"), State.TryGetResourceProduction(Definition.TerritoryID, Production));
	TestTrue(TEXT("All six base values retained"), FFactionResourceDelta::StaticStruct()->CompareScriptStruct(&Production, &Definition.ResourceProduction, 0));
	const FText Name = FText::FromString(TEXT("Roma Nova"));
	TestEqual(TEXT("Unknown founding rejects"), State.FoundSettlement(TEXT("Missing"), Name).Failure, ESettlementFoundingFailure::UnknownTerritory);
	for (const FText& Invalid : { FText::GetEmpty(), FText::FromString(TEXT(" \t\n ")) })
	{
		TestEqual(TEXT("Empty/whitespace name rejects"), State.FoundSettlement(Definition.TerritoryID, Invalid).Failure, ESettlementFoundingFailure::InvalidName);
	}
	State.TryGetSettlement(Definition.TerritoryID, Settlement);
	TestFalse(TEXT("Rejected commands do not found"), Settlement.bHasSettlement);
	TestTrue(TEXT("First founding succeeds"), State.FoundSettlement(Definition.TerritoryID, Name).WasSuccessful());
	TestEqual(TEXT("Second founding rejects"), State.FoundSettlement(Definition.TerritoryID, FText::FromString(TEXT("Overwrite"))).Failure, ESettlementFoundingFailure::AlreadyFounded);
	State.TryGetSettlement(Definition.TerritoryID, Settlement);
	TestTrue(TEXT("Settlement committed"), Settlement.bHasSettlement);
	TestEqual(TEXT("Name not overwritten"), Settlement.SettlementName.ToString(), Name.ToString());
	TestEqual(TEXT("Initial residents independent of production population"), Settlement.Population, 100);
	State.TryGetResourceProduction(Definition.TerritoryID, Production);
	TestEqual(TEXT("Gold bonus exactly once"), Production.Gold, 111);
	TestEqual(TEXT("Food bonus exactly once"), Production.Food, 72);
	TestEqual(TEXT("Iron unchanged"), Production.Iron, 33);
	TestEqual(TEXT("Wood unchanged"), Production.Wood, 44);
	TestEqual(TEXT("Stone unchanged"), Production.Stone, 55);
	TestEqual(TEXT("Population production unchanged"), Production.Population, 66);
	Settlement.Population = 999;
	Settlement.ProductionBonus.Gold = 999;
	State.ChangeOwner(Definition.TerritoryID, EFactionID::Carthage);
	State.TryGetSettlement(Definition.TerritoryID, Settlement);
	TestEqual(TEXT("Ownership/read copies do not reset or edit residents"), Settlement.Population, 100);
	TestEqual(TEXT("Read copy bonus detached"), Settlement.ProductionBonus.Gold, 100);
	for (bool bGoldOverflow : { true, false })
	{
		FTerritoryDefinition Limit = Definition;
		Limit.TerritoryID = bGoldOverflow ? TEXT("GoldLimit") : TEXT("FoodLimit");
		if (bGoldOverflow) Limit.ResourceProduction.Gold = MAX_int32 - 99;
		else Limit.ResourceProduction.Food = MAX_int32 - 49;
		TestTrue(TEXT("High base is valid before settlement"), State.RegisterTerritory(Limit).WasSuccessful());
		TestEqual(TEXT("Overflow rejects founding atomically"), State.FoundSettlement(Limit.TerritoryID, Name).Failure, ESettlementFoundingFailure::ProductionOverflow);
		State.TryGetSettlement(Limit.TerritoryID, Settlement);
		TestFalse(TEXT("Overflow leaves no partial settlement"), Settlement.bHasSettlement);
		TestEqual(TEXT("Overflow leaves population zero"), Settlement.Population, 0);
		TestTrue(TEXT("Overflow leaves all bonus fields zero"), Settlement.ProductionBonus.IsZero());
	}
	FTerritoryDefinition Boundary = Definition;
	Boundary.TerritoryID = TEXT("Boundary");
	Boundary.ResourceProduction.Gold = MAX_int32 - 100;
	Boundary.ResourceProduction.Food = MAX_int32 - 50;
	State.RegisterTerritory(Boundary);
	TestTrue(TEXT("Exact int32 boundary accepted"), State.FoundSettlement(Boundary.TerritoryID, Name).WasSuccessful());
	State.TryGetResourceProduction(Boundary.TerritoryID, Production);
	TestEqual(TEXT("Exact max Gold preserved"), Production.Gold, MAX_int32);
	TestEqual(TEXT("Exact max Food preserved"), Production.Food, MAX_int32);
	TestFalse(TEXT("Duplicate registration cannot reset settlement"), State.RegisterTerritory(Definition).WasSuccessful());
	State.TryGetSettlement(Definition.TerritoryID, Settlement);
	TestTrue(TEXT("Settlement survives duplicate registration"), Settlement.bHasSettlement);
	TestFalse(TEXT("Unknown settlement rejects"), State.TryGetSettlement(NAME_None, Settlement));
	TestFalse(TEXT("Unknown settlement resets output"), Settlement.bHasSettlement);
	TestFalse(TEXT("Unknown production rejects"), State.TryGetResourceProduction(NAME_None, Production));
	TestTrue(TEXT("Unknown production resets output"), Production.IsZero());
	State.Reset();
	TestFalse(TEXT("Reset clears live settlement authority"), State.TryGetSettlement(Definition.TerritoryID, Settlement));
	return true;
}
#endif
