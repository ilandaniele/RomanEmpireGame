// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Core/RomanEmpireGameState.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPublicGameStateDefaultsTest,
	"RomanEmpireGame.Rules.PublicGameState.UninitializedReadContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::SmokeFilter)

bool FPublicGameStateDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	const ARomanEmpireGameState* State = GetDefault<ARomanEmpireGameState>();
	if (!TestNotNull(TEXT("Public GameState native defaults exist"), State)) return false;
	TestFalse(TEXT("Read model does not poll or simulate via Tick"), State->PrimaryActorTick.bCanEverTick);
	TestFalse(TEXT("A state without authorities is not ready"), State->IsGameplayInitialized());
	TestFalse(TEXT("A state without authorities cannot have an active campaign"), State->IsCampaignActive());
	TestEqual(TEXT("An uninitialized state has no turn"), State->GetCurrentTurn(), 0);
	TestEqual(TEXT("An uninitialized state has no player faction"), State->GetPlayerFaction(), EFactionID::None);
	TestEqual(TEXT("An uninitialized state has no controlled territories"), State->GetFactionTerritoryCount(EFactionID::Rome), 0);
	TestTrue(TEXT("Uninitialized territorial catalog is empty"), State->GetTerritorySnapshots().IsEmpty());
	TestTrue(TEXT("Uninitialized army catalog is empty"), State->GetCampaignArmies().IsEmpty());
	TestFalse(TEXT("Uninitialized encounter has no identity"), State->GetCampaignEncounterPlan().EncounterID.IsValid());
	TestFalse(TEXT("Uninitialized encounter has no consumed identity"), State->HasAppliedCampaignEncounter(FGuid::NewGuid()));
	FCampaignArmy StaleArmy;
	StaleArmy.ArmyID = FGuid::NewGuid(); StaleArmy.TerritoryID = TEXT("Stale");
	TestFalse(TEXT("Uninitialized army lookup rejects"), State->TryGetCampaignArmy(StaleArmy.ArmyID, StaleArmy));
	const FCampaignArmy EmptyArmy;
	TestTrue(TEXT("Failed army lookup clears detached output"), FCampaignArmy::StaticStruct()->CompareScriptStruct(&StaleArmy, &EmptyArmy, 0));
	FTerritoryPresentationSnapshot StaleTerritory;
	StaleTerritory.TerritoryID = TEXT("Stale");
	StaleTerritory.OwnerFaction = EFactionID::Rome;
	StaleTerritory.AdjacentTerritoryIDs.Add(TEXT("StaleNeighbor"));
	TestFalse(TEXT("Uninitialized territorial lookup fails"), State->TryGetTerritorySnapshot(TEXT("Rome"), StaleTerritory));
	const FTerritoryPresentationSnapshot EmptyTerritory;
	TestTrue(TEXT("Failed territorial lookup clears detached output"),
		FTerritoryPresentationSnapshot::StaticStruct()->CompareScriptStruct(&StaleTerritory, &EmptyTerritory, 0));
	const FFactionResourceAmounts EmptyWallet;
	const FFactionResourceAmounts Wallet = State->GetFactionResources(EFactionID::Rome);
	TestTrue(TEXT("Uninitialized wallet fields are all zero"),
		FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Wallet, &EmptyWallet, 0));
	const FCampaignObjectiveState EmptyObjectives;
	const FCampaignObjectiveState Objectives = State->GetObjectiveState();
	TestTrue(TEXT("Uninitialized objective projection has no invented progress or result"),
		FCampaignObjectiveState::StaticStruct()->CompareScriptStruct(&Objectives, &EmptyObjectives, 0));
	for (const FName FunctionName : { FName(TEXT("IsGameplayInitialized")), FName(TEXT("GetCurrentGameplayMode")),
		FName(TEXT("GetCurrentTurn")), FName(TEXT("GetPlayerFaction")), FName(TEXT("GetFactionResources")),
		FName(TEXT("GetObjectiveState")), FName(TEXT("IsCampaignActive")), FName(TEXT("GetFactionTerritoryCount")),
		FName(TEXT("GetResolvedVictoryCondition")), FName(TEXT("GetTerritorySnapshots")),
		FName(TEXT("TryGetTerritorySnapshot")), FName(TEXT("GetCampaignArmies")), FName(TEXT("TryGetCampaignArmy")),
		FName(TEXT("GetCampaignEncounterPlan")), FName(TEXT("HasAppliedCampaignEncounter")) })
	{
		const UFunction* Function = State->FindFunction(FunctionName);
		TestTrue(*FString::Printf(TEXT("%s is an exported pure read"), *FunctionName.ToString()),
			Function && Function->HasAllFunctionFlags(FUNC_BlueprintPure | FUNC_Const));
	}
	return true;
}

#endif
