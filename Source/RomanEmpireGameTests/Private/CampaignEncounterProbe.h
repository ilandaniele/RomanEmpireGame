// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Battle/TacticalBattleResult.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "CampaignEncounterProbe.generated.h"
class ARomanEmpireGameState;
class FAutomationTestBase;
UCLASS()
class UCampaignEncounterProbe : public UObject
{
	GENERATED_BODY()
public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<ARomanEmpireGameState> State;
	TWeakObjectPtr<UObject> Manager;
	TWeakObjectPtr<UObject> Map;
	FGuid EncounterID, ArmyID;
	bool bVictory = true;
	int32 ExpectedSurvivors = 14;
	int32 ArmyEvents = 0, TerritoryEvents = 0, EncounterEvents = 0;
	bool bSawPreparedPlan = false;
	bool bAllReadsCommitted = true;
	UFUNCTION() void OnArmiesChanged();
	UFUNCTION() void OnTerritoryChanged(FName ID, EFactionID Before, EFactionID After);
	UFUNCTION() void OnEncounterChanged();
	UFUNCTION() void OnTacticalChanged(FTacticalBattleResult Result);
private:
	void CheckCommittedAndReentry();
};
