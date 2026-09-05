// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Faction/FactionData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFactionResourceAffordabilityTest,
	"RomanEmpireGame.Rules.Resources.AffordabilityAndDeduction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FFactionResourceAffordabilityTest::RunTest(const FString& Parameters)
{
	FFactionResources Balance;
	Balance.Gold = 500;
	Balance.Food = 200;
	Balance.Iron = 100;
	Balance.Wood = 150;
	Balance.Stone = 75;
	Balance.Population = 20;

	FFactionResources AffordableCost;
	AffordableCost.Gold = 150;
	AffordableCost.Food = 50;
	AffordableCost.Iron = 0;
	AffordableCost.Wood = 0;
	AffordableCost.Stone = 0;
	AffordableCost.Population = 1;

	TestTrue(TEXT("Balance can afford the declared cost"), Balance.CanAfford(AffordableCost));
	Balance.Deduct(AffordableCost);
	TestEqual(TEXT("Gold is deducted once"), Balance.Gold, 350);
	TestEqual(TEXT("Food is deducted once"), Balance.Food, 150);
	TestEqual(TEXT("Population is deducted once"), Balance.Population, 19);

	FFactionResources UnaffordableCost = AffordableCost;
	UnaffordableCost.Gold = 351;
	TestFalse(TEXT("Cost above the remaining balance is rejected"), Balance.CanAfford(UnaffordableCost));

	return true;
}

#endif
