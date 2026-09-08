// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Faction/FactionData.h"
#include "UObject/Object.h"
#include "PlacementTransactionProbe.generated.h"

class ABuildingBase;
class FAutomationTestBase;

/** Editor-test-only observer issuing adversarial commands from real synchronous events. */
UCLASS()
class UPlacementTransactionProbe : public UObject
{
	GENERATED_BODY()

public:
	FAutomationTestBase* Test = nullptr;
	TWeakObjectPtr<UObject> Placement;
	int32 WalletNotifications = 0;
	int32 PlacedNotifications = 0;
	bool bCancelDuringSpend = false;
	FName BuildingID;

	UFUNCTION()
	void OnResourcesChanged(EFactionID FactionID, FFactionResourceAmounts NewBalance);

	UFUNCTION()
	void OnPlaced(ABuildingBase* Building);
};
