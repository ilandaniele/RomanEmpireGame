// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "CampaignTravelProbe.generated.h"
class ARomanEmpireGameState;
class UCampaignSessionSubsystem;
class FAutomationTestBase;
UCLASS()
class UCampaignTravelProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UCampaignSessionSubsystem> Session;
	TWeakObjectPtr<UObject> Producer, Factions, Map;
	FFactionResourceAmounts ExpectedWallet;
	int32 RefundNotifications = 0;
	int32 ReadyNotifications = 0;
	int32 ExpectedTurn = 1;
	FGuid ArmyID;
	bool bExpectedRetired = false;
	UFUNCTION() void OnRefund(EFactionID Faction, FFactionResourceAmounts Balance);
	UFUNCTION() void OnReady();
};
