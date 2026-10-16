// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignTurn.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "CampaignAIExecutionProbe.generated.h"
class ARomanEmpireGameState;
class UCampaignSessionSubsystem;
class URomanTerritoryDefinition;
class FAutomationTestBase;
UCLASS()
class UCampaignAIExecutionProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UObject> Manager;
	TWeakObjectPtr<UCampaignSessionSubsystem> Session;
	TWeakObjectPtr<URomanTerritoryDefinition> Target;
	FFactionResourceAmounts ExpectedWallet;
	FGuid AIArmy, AcceptedEncounter;
	int32 ExpectedCount = 0, RecruitmentEvents = 0;
	bool bBreakMap = false;
	ECampaignTurnStep CurrentStep = ECampaignTurnStep::ValidateRequest;
	TArray<ECampaignTurnStep> Steps;
	UFUNCTION() void OnStep(int32 Turn, ECampaignTurnStep Step);
	UFUNCTION() void OnResources(EFactionID Faction, FFactionResourceAmounts Balance);
	UFUNCTION() void OnEncounter();
	UFUNCTION() void OnSession();
};
