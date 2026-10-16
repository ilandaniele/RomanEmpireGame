// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Battle/TacticalBattleResult.h"
#include "UObject/Object.h"
#include "CampaignReinforcementProbe.generated.h"
class ARomanEmpireGameState;
class FAutomationTestBase;
UCLASS()
class UCampaignReinforcementProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UObject> Manager, Producer;
	FGuid ArmyID, EncounterID;
	int32 TacticalEvents = 0, ArmyEvents = 0, EncounterEvents = 0;
	bool bAllReadsCommitted = true;
	UFUNCTION() void OnTacticalChanged(FTacticalBattleResult Result);
	UFUNCTION() void OnArmiesChanged();
	UFUNCTION() void OnEncounterChanged();
private:
	void CheckCommittedAndReentry();
};
