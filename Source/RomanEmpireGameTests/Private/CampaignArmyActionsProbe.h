// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Campaign/CampaignArmyState.h"
#include "UObject/Object.h"
#include "CampaignArmyActionsProbe.generated.h"
class ARomanEmpireGameState;
class FAutomationTestBase;
UCLASS()
class UCampaignArmyActionsProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UObject> Manager, Factions;
	FGuid SourceArmyID;
	FFactionResourceAmounts ExpectedWallet;
	int32 ExpectedTotal = 0, ExpectedArmies = 0, ArmyEvents = 0, ResourceEvents = 0;
	bool bAllCommitted = true;
	UFUNCTION() void OnArmiesChanged();
	UFUNCTION() void OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts Balance);
private:
	void CheckCommittedAndReentry();
};
