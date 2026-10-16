// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignSave.h"
#include "Campaign/CampaignTurn.h"
#include "UObject/Object.h"
#include "CampaignSaveProbe.generated.h"
class ARomanEmpireGameState;
class UCampaignSaveSubsystem;
class URomanTerritoryDefinition;
class FAutomationTestBase;
UCLASS()
class UCampaignSaveProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UCampaignSaveSubsystem> Save;
	TWeakObjectPtr<URomanTerritoryDefinition> Target;
	bool bBreakMap = false;
	FCampaignSaveRecord Expected;
	int32 ReadyNotifications = 0, SaveNotifications = 0, TurnStages = 0;
	UFUNCTION() void OnReady();
	UFUNCTION() void OnSave();
	UFUNCTION() void OnSession();
	UFUNCTION() void OnStep(int32 Turn, ECampaignTurnStep Step);
};
