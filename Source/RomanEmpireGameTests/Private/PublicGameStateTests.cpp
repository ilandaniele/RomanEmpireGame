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
		FName(TEXT("GetResolvedVictoryCondition")) })
	{
		const UFunction* Function = State->FindFunction(FunctionName);
		TestTrue(*FString::Printf(TEXT("%s is an exported pure read"), *FunctionName.ToString()),
			Function && Function->HasAllFunctionFlags(FUNC_BlueprintPure | FUNC_Const));
	}
	return true;
}

#endif
