// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Faction/FactionData.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFactionResourceDefaultsTest,
	"RomanEmpireGame.Rules.Resources.ZeroDefaultsAndStartingDefinitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FFactionResourceDefaultsTest::RunTest(const FString& Parameters)
{
	const FFactionResourceAmounts EmptyAmounts;
	const FFactionResourceDelta EmptyDelta;
	TestTrue(TEXT("Resource amounts default to zero"), EmptyAmounts.IsZero());
	TestTrue(TEXT("Resource deltas default to zero"), EmptyDelta.IsZero());

	const FFactionResourceAmounts Rome = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Rome);
	const FFactionResourceAmounts Carthage = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Carthage);
	const FFactionResourceAmounts Gaul = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::Gaul);
	const FFactionResourceAmounts Invalid = RomanEmpireEconomy::GetPrototypeStartingResources(EFactionID::None);

	TestEqual(TEXT("Rome has an explicit starting gold amount"), Rome.Gold, 1100);
	TestEqual(TEXT("Carthage has an explicit starting gold amount"), Carthage.Gold, 1300);
	TestEqual(TEXT("Gaul has an explicit starting gold amount"), Gaul.Gold, 900);
	TestEqual(TEXT("Starting food is explicitly defined"), Rome.Food, 500);
	TestEqual(TEXT("Starting population is explicitly defined"), Rome.Population, 100);
	TestTrue(TEXT("An invalid faction does not receive implicit resources"), Invalid.IsZero());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFactionResourceAtomicTransactionTest,
	"RomanEmpireGame.Rules.Resources.AtomicTransactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FFactionResourceAtomicTransactionTest::RunTest(const FString& Parameters)
{
	FFactionResourceAmounts Balance;
	Balance.Gold = 500;
	Balance.Food = 200;
	Balance.Iron = 100;
	Balance.Wood = 150;
	Balance.Stone = 75;
	Balance.Population = 20;

	FFactionResourceAmounts AffordableCost;
	AffordableCost.Gold = 150;
	AffordableCost.Food = 50;
	AffordableCost.Population = 1;

	const FFactionResourceTransactionResult Success =
		RomanEmpireEconomy::TrySpendResources(Balance, AffordableCost);
	TestTrue(TEXT("Affordable cost succeeds"), Success.WasSuccessful());
	TestEqual(TEXT("Successful transaction has no failure"),
		static_cast<uint8>(Success.Failure), static_cast<uint8>(EResourceTransactionFailure::None));
	TestEqual(TEXT("Gold is deducted once"), Balance.Gold, 350);
	TestEqual(TEXT("Food is deducted once"), Balance.Food, 150);
	TestEqual(TEXT("Population is deducted once"), Balance.Population, 19);

	FFactionResourceAmounts UnaffordableCost;
	UnaffordableCost.Gold = 351;
	UnaffordableCost.Food = 1;
	const FFactionResourceAmounts BeforeRejectedSpend = Balance;
	const FFactionResourceTransactionResult RejectedSpend =
		RomanEmpireEconomy::TrySpendResources(Balance, UnaffordableCost);
	TestEqual(TEXT("Rejection reports the exact scarce resource"),
		static_cast<uint8>(RejectedSpend.Failure),
		static_cast<uint8>(EResourceTransactionFailure::InsufficientGold));
	TestEqual(TEXT("Rejected spend leaves gold unchanged"), Balance.Gold, BeforeRejectedSpend.Gold);
	TestEqual(TEXT("Rejected spend leaves food unchanged"), Balance.Food, BeforeRejectedSpend.Food);

	FFactionResourceAmounts InvalidCost;
	InvalidCost.Wood = -1;
	const FFactionResourceTransactionResult InvalidSpend =
		RomanEmpireEconomy::TrySpendResources(Balance, InvalidCost);
	TestEqual(TEXT("Negative costs are rejected"),
		static_cast<uint8>(InvalidSpend.Failure),
		static_cast<uint8>(EResourceTransactionFailure::InvalidCost));
	TestEqual(TEXT("Invalid cost cannot mint wood"), Balance.Wood, BeforeRejectedSpend.Wood);

	FFactionResourceDelta RejectedDelta;
	RejectedDelta.Gold = 25;
	RejectedDelta.Food = -151;
	const FFactionResourceTransactionResult DeltaFailure =
		RomanEmpireEconomy::TryApplyResourceDelta(Balance, RejectedDelta);
	TestEqual(TEXT("Underflowing delta reports food shortage"),
		static_cast<uint8>(DeltaFailure.Failure),
		static_cast<uint8>(EResourceTransactionFailure::InsufficientFood));
	TestEqual(TEXT("Rejected multi-resource delta does not add gold"), Balance.Gold, BeforeRejectedSpend.Gold);
	TestEqual(TEXT("Rejected multi-resource delta does not remove food"), Balance.Food, BeforeRejectedSpend.Food);

	FFactionResourceDelta Production;
	Production.Gold = 25;
	Production.Food = 10;
	const FFactionResourceTransactionResult ProductionResult =
		RomanEmpireEconomy::TryApplyResourceDelta(Balance, Production);
	TestTrue(TEXT("Valid production delta succeeds"), ProductionResult.WasSuccessful());
	TestEqual(TEXT("Production gold applies once"), Balance.Gold, 375);
	TestEqual(TEXT("Production food applies once"), Balance.Food, 160);

	return true;
}

#endif
