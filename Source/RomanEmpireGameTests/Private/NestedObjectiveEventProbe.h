// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Campaign/CampaignObjectiveState.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "NestedObjectiveEventProbe.generated.h"

class FAutomationTestBase;

/** Editor-test-only observer that commits once inside a real objective notification. */
UCLASS()
class UNestedObjectiveEventProbe : public UObject
{
	GENERATED_BODY()

public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<UObject> FactionManager;
	EFactionID PlayerFaction = EFactionID::None;
	int32 NestedGold = 29;
	int32 NestedTransactions = 0;
	TArray<int32> ObservedGold;

	UFUNCTION()
	void HandleObjectives(FCampaignObjectiveState State);
};
