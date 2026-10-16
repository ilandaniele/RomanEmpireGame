// Copyright Roman Empire Game. All Rights Reserved.
#include "Campaign/TerritoryData.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampaignAdjacencyTest,
	"RomanEmpireGame.Rules.Campaign.StableAdjacency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FCampaignAdjacencyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	auto Make = [](FName ID, TArray<FName> Neighbors)
	{
		FTerritoryDefinition Definition;
		Definition.TerritoryID = ID;
		Definition.DisplayName = FText::FromName(ID);
		Definition.AdjacentTerritoryIDs = MoveTemp(Neighbors);
		return Definition;
	};
	FTerritoryCampaignState State;
	TestTrue(TEXT("Empty state has no territory IDs"), State.GetTerritoryIDs().IsEmpty());
	FTerritoryDefinition Rome = Make(TEXT("Rome"), { TEXT("Sicily"), TEXT("Alps") });
	TestTrue(TEXT("Forward references can register before their neighbors"), State.RegisterTerritory(Rome).WasSuccessful());
	TestFalse(TEXT("Incomplete graph is rejected"), State.HasValidAdjacency());
	TestFalse(TEXT("Incomplete graph cannot expose edges"), State.AreAdjacent(TEXT("Rome"), TEXT("Sicily")));
	TestTrue(TEXT("Second endpoint registers"), State.RegisterTerritory(Make(TEXT("Sicily"), { TEXT("Rome") })).WasSuccessful());
	TestTrue(TEXT("Third endpoint registers"), State.RegisterTerritory(Make(TEXT("Alps"), { TEXT("Rome") })).WasSuccessful());
	TestTrue(TEXT("An isolated territory is legal"), State.RegisterTerritory(Make(TEXT("Island"), {})).WasSuccessful());
	TestTrue(TEXT("Known reciprocal graph is valid"), State.HasValidAdjacency());
	TestTrue(TEXT("All registered IDs have deterministic lexical order"),
		State.GetTerritoryIDs() == TArray<FName>({ TEXT("Alps"), TEXT("Island"), TEXT("Rome"), TEXT("Sicily") }));
	auto IDs = State.GetTerritoryIDs();
	IDs.Reset();
	TestEqual(TEXT("Editing returned IDs cannot edit canonical state"), State.GetTerritoryIDs().Num(), 4);
	TestTrue(TEXT("Forward edge"), State.AreAdjacent(TEXT("Rome"), TEXT("Sicily")));
	TestTrue(TEXT("Reverse edge"), State.AreAdjacent(TEXT("Sicily"), TEXT("Rome")));
	TestFalse(TEXT("No self edge"), State.AreAdjacent(TEXT("Rome"), TEXT("Rome")));
	TestFalse(TEXT("No invented nearby connection"), State.AreAdjacent(TEXT("Alps"), TEXT("Sicily")));
	TestFalse(TEXT("Unknown territory cannot connect"), State.AreAdjacent(TEXT("Missing"), TEXT("Rome")));
	TestTrue(TEXT("Neighbors have deterministic lexical order"),
		State.GetAdjacentIDs(TEXT("Rome")) == TArray<FName>({ TEXT("Alps"), TEXT("Sicily") }));
	Rome.AdjacentTerritoryIDs.Reset();
	TestTrue(TEXT("Mutating the original definition cannot alter registered graph"), State.AreAdjacent(TEXT("Rome"), TEXT("Sicily")));
	auto Returned = State.GetAdjacentIDs(TEXT("Rome"));
	Returned.Reset();
	State.ChangeOwner(TEXT("Rome"), EFactionID::Carthage);
	TestEqual(TEXT("Returned arrays and ownership changes cannot alter edges"), State.GetAdjacentIDs(TEXT("Rome")).Num(), 2);
	TestTrue(TEXT("Unknown query is empty"), State.GetAdjacentIDs(TEXT("Missing")).IsEmpty());
	State.Reset();
	TestTrue(TEXT("Reset drops the ID catalog"), State.GetTerritoryIDs().IsEmpty());
	TestTrue(TEXT("Reset drops edges"), State.GetAdjacentIDs(TEXT("Rome")).IsEmpty());
	State.RegisterTerritory(Make(TEXT("Rome"), { TEXT("Sicily") }));
	State.RegisterTerritory(Make(TEXT("Sicily"), {}));
	TestFalse(TEXT("One-way graph invalid even with known IDs"), State.HasValidAdjacency());
	TestFalse(TEXT("One-way edge cannot be used"), State.AreAdjacent(TEXT("Rome"), TEXT("Sicily")));
	for (const TArray<FName>& Bad : { TArray<FName>{ NAME_None }, TArray<FName>{ TEXT("Rome") },
		TArray<FName>{ TEXT("Sicily"), TEXT("Sicily") } })
	{
		FTerritoryCampaignState Invalid;
		TestFalse(TEXT("Empty, self and duplicate neighbor IDs cannot register"),
			Invalid.RegisterTerritory(Make(TEXT("Rome"), Bad)).WasSuccessful());
		TestEqual(TEXT("Bad registration remains atomic"), Invalid.GetTerritoryCount(), 0);
	}
	return true;
}
#endif
